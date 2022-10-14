#include "app.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <math.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define _POSIX_SOURCE 1

int countDigit(long n)
{
    int count = 0;
    while (n != 0)
    {
        n = n / 10;
        ++count;
    }
    return count;
}

int createControlPacket(unsigned char control){
  size_t index = 0;
  appLayer->data_frame[index] = control;

  index++;
  /* 1st parameter, filesize */
  appLayer->data_frame[index] = F_SIZE;
  index++;
  int filesize_length = countDigit(appLayer->filesize);
  appLayer->data_frame[index] = filesize_length;
  index++; //index is now 3
  char filesize[FILESIZE_MAX_DIGITS];
  
  snprintf(filesize, filesize_length + 1, "%d", appLayer->filesize);
  
  for(int i = 0; i < filesize_length; i++) {
    appLayer->data_frame[index] = filesize[i];
    index++;
  }


  /* 2nd parameter, filename */
  appLayer->data_frame[index] = F_NAME;
  index++;
  int filename_length =  strlen(appLayer->filename) + 1;
  appLayer->data_frame[index] = filename_length;
  index++;
  for(int i = 0; i < filename_length; i++) {

    appLayer->data_frame[index] = appLayer->filename[i];
    index++;
  }
  return index;

}

void emptyDataFrame(){
  bzero(&appLayer->data_frame, sizeof(appLayer->data_frame));
}

/*
Creates a data packet with data read from file, returns its size
*/

int createDataFrame(int fd) {
   //read the next PACKSIZE bytes from the file
  int bytes_read = read(fd,appLayer->data_frame + 4, PACKSIZE);
  printf("num_of_bytes_read_from_file: %d \n", bytes_read);

  if(bytes_read == 0){
    return -1;
  }

  appLayer->data_frame[0] = CONTROL_DATA;
  appLayer->data_frame[1] = appLayer->counter % 255;

 
  /*
    K = 256 * L2 + L1
    L2 = (K - L1) / 256
    L1 = K - 256*L2
  */
  appLayer->data_frame[2] = (bytes_read >> 8) & 0xFF; // L2
  appLayer->data_frame[3] = bytes_read & 0xFF;   //L1

  return (bytes_read + 4);
}

int power(int base, int exponent)
{
int result=1;
for (exponent; exponent>0; exponent--)
{
result = result * base;
}
return result;
}

int processImage()
{
  if (appLayer->status == TRANSMITER)
  {
    char filename[256] = "pinguim.gif";
    int sending_file_fd = open(filename, O_RDONLY);

    if (sending_file_fd == -1)
    {
      printf("Error opening sending file\n");
      return -1;
    }

    struct stat s;
    if (fstat(sending_file_fd, &s))
    {
      printf("Error getting sending file stat information\n");
      return -1;
    }

    int file_size = (int)s.st_size;
    appLayer->filesize = file_size;
    strcpy(appLayer->filename, filename);

    //create the start data_frame
    emptyDataFrame();

    int packet_size = createControlPacket(0x02);

    unsigned char* frame = (unsigned char*) malloc(packet_size * 2); //Allocate double amount to acount for byte stuffing
    memcpy(frame, appLayer->data_frame, packet_size);


    if (llwrite(appLayer->fd, frame, packet_size) < 0)
    {
      printf("Error writing package start info\n");
      return -1;
    }
  
    appLayer->counter++;

    int file_size = (int)s.st_size;
    int current_size = 0;
    int sequence_number = 0;

    while (file_size != current_size)
    {

      emptyDataFrame();
      packet_size = createDataFrame(sending_file_fd);

      if(packet_size == -1){
        break; //we have read all of the file
      }
      printf("packet_size: %d\n", packet_size);
      printf("counter: %d \n", appLayer->counter);

      appLayer->counter++;
      frame = (unsigned char*) malloc(packet_size * 2); //byte stuffing
      memcpy(frame, appLayer->data_frame, packet_size);

      if (llwrite(appLayer->fd, frame, packet_size) <= 0)
      {
        printf("Error writing data_packet number %d info\n",appLayer->counter);
        return -1;
      }

      free(frame);

    }

    emptyDataFrame();
    packet_size = createControlPacket(0x03);
    frame = (unsigned char*) malloc(packet_size * 2);
    memcpy(frame, appLayer->data_frame, packet_size);
    
    if (llwrite(appLayer->fd, frame, packet_size) < 0)
    {
      printf("Error writing package end info\n");
      return -1;
    }
    free(frame);
    close(sending_file_fd);
  }
  else
  {
    int receiving_file = -1;

    while (true)
    {
      unsigned char *package_info = (unsigned char *)malloc(MAXSIZE);

      if (llread(appLayer->fd, package_info) < 0)
      {
        printf("Error reading from link layer\n");
        return -1;
      }

      if (package_info[0] == CONTROL_START) //receiving start packet
      {
        unsigned char *filename;
        int counter = 1;
        if (package_info[counter] == 0x00) //receiving the file_size
        {
          counter++;
          int len = package_info[counter]; //size in bytes of the next field
          int maxlen = len;
          printf("length of filesize: %d \n", len);
          unsigned file_size = 0;
          counter++;
          for (counter; counter < len + 3; counter++)
          {
            printf("%c ", package_info[counter]);
            file_size = file_size +  (int)(package_info[counter] - '0') * power(10, maxlen-1);
            maxlen--;
          }
          printf("\n");
          printf("%d\n", file_size);
        }
        if (package_info[counter] == 0x01)
        {
          counter++;
          
          filename = malloc(package_info[counter]); //has stored how many bytes long the filename is

          counter++;

          filename = memcpy(filename, &package_info[counter], package_info[counter - 1]);
          
        }
    
        unsigned char pre[5] = "pre-";
        unsigned char *actual_filename = malloc(strlen(filename) + 4);

        actual_filename = strcat(pre, filename);
       
        receiving_file = open(actual_filename, O_WRONLY | O_CREAT | O_APPEND, 0666);

        if (receiving_file == -1)
        {
          printf("Error opening receiving file\n");
          return -1;
        }
      }
      else if (package_info[0] == CONTROL_END)
      {
        break;
      }
      else if (package_info[0] == CONTROL_DATA) //data package
      {
        int data_size = package_info[3] + package_info[2] * 256;
        printf("data_size: %d \n", data_size);
        unsigned char *actual_buffer = malloc(data_size);
        memcpy(actual_buffer, &package_info[4], data_size);

        if (write(receiving_file, actual_buffer, data_size) < 0)
        {
          printf("Error writing data to new file\n");
          return -1;
        }

        free(actual_buffer);
      }

      free(package_info);
    }

    close(receiving_file);
  }

  return 0;
}