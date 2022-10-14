#include "link.h"
#include "alarm.h"
#include "statemachine.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#define _POSIX_SOURCE 1

int connect(int fd)
{
  if (tcgetattr(fd, &ll->oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    return -1;
  }

  bzero(&ll->newtio, sizeof(ll->newtio));
  ll->newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  ll->newtio.c_iflag = IGNPAR;
  ll->newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  ll->newtio.c_lflag = 0;

  ll->newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  ll->newtio.c_cc[VMIN] = 5;  /* blocking read until 5 chars received */


  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &ll->newtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }

  ll->numTransmissions = 3;
  ll->current_timeout = 0;
  ll->sequenceNumber = 0;
  ll->timeout = 3;

  printf("New termios structure set\n");
  return 0;
}

int connect_transmiter(int fd)
{
  if (connect(fd))
    return 1;

  unsigned char SET[5] = {FLAG, A_E, C_SET, A_E ^ C_SET, FLAG};
  unsigned char UA[5] = {FLAG, A_E, C_UA, A_E ^ C_UA, FLAG};

  if (control_uknowledge_transmiter(fd, SET, UA))
    return 1;
}

int connect_receiver(int fd)
{
  if (connect(fd))
    return 1;

  unsigned char SET[5] = {FLAG, A_E, C_SET, A_E ^ C_SET, FLAG};
  unsigned char UA[5] = {FLAG, A_E, C_UA, A_E ^ C_UA, FLAG};

  if (control_uknowledge_receiver(fd, SET, UA))
    return 1;
}

int process_state_machine5(int fd, unsigned char *expected5)
{
  int state = 0;
  while (state != -2)
  {
    unsigned char data;
    int ret = read_byte(fd, &data);
    if (ret != 0)
    {
      tcflush(fd, TCIOFLUSH);
      return ret;
    }

    state = state_machine5(data, expected5, state);
  }

  printf("\n");

  return -2;
}

int write_packet(int fd, unsigned char *buffer, int length)
{
  printf("Writing: ");
  for (int i = 0; i < length; i++)
  {
    printf("%x ", buffer[i]);
  }
  alarm(ll->timeout);
  printf("\n");
  return write(fd, buffer, sizeof(unsigned char) * length);
}

int read_byte(int fd, unsigned char *buffer)
{
  while (read(fd, buffer, sizeof(unsigned char)) <= 0)
  {
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
      return -1;

    if (ll->current_timeout < numAlarms)
    {
      alarm(ll->timeout);
      ll->current_timeout = numAlarms;
      return numAlarms;
    }
  }
  alarm(0);
  numAlarms = 0;
  ll->current_timeout = 0;
  printf("%x ", *buffer);
  return 0;
}

int control_uknowledge_transmiter(int fd, unsigned char *buffer_control, unsigned char *buffer_uknowledge)
{
  while (true)
  {
    if (write_packet(fd, buffer_control, sizeof(unsigned char) * 5) < 0)
      return 1;

    int proc = process_state_machine5(fd, buffer_uknowledge);

    if (proc == -1)
      return 1;
    else if (proc == ll->numTransmissions)
      return 1;
    else if (proc == -2)
      break;
  }
  return 0;
}

int control_uknowledge_receiver(int fd, unsigned char *buffer_control, unsigned char *buffer_uknowledge)
{
  if (process_state_machine5(fd, buffer_control) == -1)
    return 1;

  if (write_packet(fd, buffer_uknowledge, sizeof(unsigned char) * 5) < 0)
    return 1;

  return 0;
}

int disconnect(int fd)
{
  if (tcsetattr(fd, TCSANOW, &ll->oldtio) == -1)
  {
    perror("tcsetattr");
    return -1;
  }
  return 0;
}

int disconnect_transmiter(int fd)
{
  unsigned char DISC_E[5] = {FLAG, A_E, C_DISC, A_E ^ C_DISC, FLAG};
  unsigned char DISC_R[5] = {FLAG, A_R, C_DISC, A_R ^ C_DISC, FLAG};
  unsigned char UA[5] = {FLAG, A_R, C_UA, A_R ^ C_UA, FLAG};

  if (control_uknowledge_transmiter(fd, DISC_E, DISC_R))
    return -1;

  if (write_packet(fd, UA, 5) < 0)
    return -1;

  if (disconnect(fd))
    return -1;
}

int disconnect_receiver(int fd)
{
  unsigned char DISC_E[5] = {FLAG, A_E, C_DISC, A_E ^ C_DISC, FLAG};
  unsigned char DISC_R[5] = {FLAG, A_R, C_DISC, A_R ^ C_DISC, FLAG};
  unsigned char UA[5] = {FLAG, A_R, C_UA, A_R ^ C_UA, FLAG};

  if (control_uknowledge_receiver(fd, DISC_E, DISC_R))
    return -1;

  if (process_state_machine5(fd, UA) == -1)
    return 1;

  if (disconnect(fd))
    return -1;
}

int llopen(const char *filename, int status)
{
  int fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
  ll->status = status;
  if (fd < 0)
  {
    printf("Failed to open file descriptor link layer\n");
    return -1;
  }
  if (status == TRANSMITER)
  {
    if (connect_transmiter(fd))
      return -1;
  }
  else
  {
    if (connect_receiver(fd))
      return -1;
  }

  return fd;

}


/*
Calculates the BCC corresponding to a data frame
*/
unsigned char BCC2(unsigned char* data_frame, int length) {

  unsigned char bcc = data_frame[0];

  for(int i = 1; i < length; i++) {
      bcc ^= data_frame[i];
  }

  return bcc;
}

int byte_stuffing(unsigned char* data_frame, int length){
  unsigned char after_stuffing_buffer[MAXSIZE];
  int add_length = 0;

  for (int i = 0; i < length; i++)
  {
    if (data_frame[i] == FLAG)
    {
      after_stuffing_buffer[i + add_length] = ESCAPE_CHAR;
      after_stuffing_buffer[i + 1 + add_length] = FLAG ^ 0x20;
      add_length++;
    }
    else if (data_frame[i] == ESCAPE_CHAR)
    {
      after_stuffing_buffer[i + add_length] = ESCAPE_CHAR;
      after_stuffing_buffer[i + 1 + add_length] = ESCAPE_CHAR ^ 0x20;
      add_length++;
    }
    else
    {
      after_stuffing_buffer[i + add_length] = data_frame[i];
    }
  }

<<<<<<< HEAD
  bzero(data_frame, length); //empty data_frame
  memcpy(data_frame, after_stuffing_buffer, length + add_length); //copy the now stuffed bytes into our original buffer
=======
  {
    length++;
}

int createIFrame(unsigned char* data_frame, int length) {

  ll->frame[FLAG_INDEX] = FLAG;
  ll->frame[ADDRESS_INDEX] = A_E;
  ll->frame[CONTROL_INDEX] = (ll->sequenceNumber << 6);
  ll->frame[BCC1_INDEX] = BCC1(ll->frame[ADDRESS_INDEX] , ll->frame[CONTROL_INDEX]);
  
  memcpy(ll->frame + 4, data_frame, length);

  ll->frame[length + 4] = FLAG;
  return length + 5;
}


void emptyFrame() {

  bzero(&ll->frame, sizeof(ll->frame));  

}


int llwrite(int fd, unsigned char *buffer, int length)
{
  
  // add the bcc2 to the data_buffer so that it is also bytestuffed!
  buffer[length] = BCC2(buffer, length);
  int stuffed_length = byte_stuffing(buffer, length + 1);

  emptyFrame();
  stuffed_length = createIFrame(buffer, stuffed_length);
   
  //ll->frame has our trama at this point all ready to send out
  int ret;
  if ((ret = write_packet(fd, ll->frame, stuffed_length)) < 0)
  {
    return -1;
  }

  unsigned char response[5] = {FLAG, A_E, BASE_RR | (ll->sequenceNumber << 7), A_E ^ (BASE_RR | (ll->sequenceNumber << 7)), FLAG};
<<<<<<< HEAD
  
=======
>>>>>>> 70fbc24ecaeff0da275aa60f82fc9cf1740e2932
  if (process_state_machine5(fd, response) == -1)
  {
    return -1;
  }
  
  
  ll->sequenceNumber ^= 0x01; //change the sequence number for the next packet
  return ret;
}

int llread(int fd, unsigned char *buffer)
{
  int counter = 0;
  unsigned char *data_buffer = (unsigned char *)malloc(MAXSIZE);
  unsigned char c;
  printf("Reading: ");
  while (true)
  {
    if (read_byte(fd, &c) == -1)
      return -1;

    
    if (c == ESCAPE_CHAR)
    {
<<<<<<< HEAD
      if (read_byte(fd, &c)) //read the next byte and destuff it
=======
      if (read_byte(fd, &c) == -1)
>>>>>>> 70fbc24ecaeff0da275aa60f82fc9cf1740e2932
        return -1;
      c |= 0x20;
      data_buffer[counter] = c;
      counter++;
      continue;
    }
    data_buffer[counter] = c;
    counter++;
    if (c == FLAG && counter != 1)
      break;
  }
  
  printf("counter : %d \n", counter);
  printf("\n");
<<<<<<< HEAD
  
  //buffer now holds the 104 data bytes
  buffer = memcpy(buffer, &data_buffer[4], counter - 6);

  unsigned char response[5] = {FLAG, A_E, BASE_RR | (ll->sequenceNumber << 7), A_E ^ (BASE_RR | (ll->sequenceNumber << 7)), FLAG};
  
  if (write_packet(fd, response, 5) < 0)
=======

  memcpy(buffer, &data_buffer[4], counter - 6);
  free(data_buffer);

  unsigned char response[5] = {FLAG, A_E, BASE_RR | (ll->sequenceNumber << 7), A_E ^ (BASE_RR | (ll->sequenceNumber << 7)), FLAG};

  if (write_packet(fd, response, 5) <= 0)
>>>>>>> 70fbc24ecaeff0da275aa60f82fc9cf1740e2932
  {
    return -1;
  }

  ll->sequenceNumber ^= 0x01;

  return counter;
}

int llclose(int fd)
{

  sleep(1);

  printf("Disconnecting\n\n");

  if (ll->status == TRANSMITER)
  {
    if (disconnect_transmiter(fd))
      return -1;
  }
  else
  {
    if (disconnect_receiver(fd))
      return -1;
  }

  printf("Disconnected\n");

  close(fd);

  return 1;
}
