#ifndef APP_H_
#define APP_H_

#include "link.h"

#define TRANSMITER 1
#define RECEIVER 0

#define MAXSIZE 200
#define PACKSIZE 100
#define FILENAME_MAX_SIZE 256
#define FILESIZE_MAX_DIGITS 8 /* maximum of 256 bytes file */
#define MAX_CONTROL_FRAME (1 + 2 + FILESIZE_MAX_DIGITS + 2 + FILENAME_MAX_SIZE)    
#define DATA_FRAME_SIZE ((PACKSIZE + 4 < MAX_CONTROL_FRAME) ? MAX_CONTROL_FRAME : PACKSIZE + 4)  /* Picks the highest value a data frame will ever possibly need */
#define F_SIZE 0
#define F_NAME 1

#define CONTROL_DATA 0x01
#define CONTROL_START 0x02
#define CONTROL_END 0x03


struct applicationLayer
{
    int fd;     /*Descritor correspondente à porta série*/
    int status; /*TRANSMITTER | RECEIVER*/
    int filesize; /* size of the file */
    int counter; /* pack counter para aquela cena com o remainder de 255 */
    char filename[FILENAME_MAX_SIZE];    /*name of the file*/
    unsigned char data_frame[DATA_FRAME_SIZE]; 
};

struct applicationLayer *appLayer;

int processImage();

void emptyDataFrame();

int llclose();

int createControlPacket(unsigned char control);

int createDataFrame(int fd);

#endif
