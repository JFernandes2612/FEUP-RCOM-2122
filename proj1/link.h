#ifndef LINK_H_
#define LINK_H_

#define BAUDRATE B38400

#include <termios.h>
#include <stdbool.h>

#define FLAG 0x7E
#define A_E 0x03
#define A_R 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define BASE_RR 0x05
#define BASE_REJ 0x01

#define TRANSMITER 1
#define RECEIVER 0
#define ESCAPE_CHAR 0x7D 
#define FLAG_INDEX 0                             /* Frame header flag index */
#define ADDRESS_INDEX  1					     /* Frame header address index */
#define CONTROL_INDEX  2                        /* Frame header control index */
#define BCC1_INDEX     3                        /* Frame header BCC index */ 

#define BCC1(a, b) (a ^ b)

#define MAXSIZE 200

struct linkLayer
{
    unsigned char sequenceNumber; /*Número de sequência da trama: 0, 1*/
    unsigned int timeout;        /*Valor do temporizador: 1 s*/
    unsigned int current_timeout;
    unsigned int numTransmissions; /*Número de tentativas em caso defalha*/
    int status;
    struct termios oldtio;
    struct termios newtio;
    unsigned char frame[MAXSIZE];

};

struct linkLayer *ll;


int llopen(const char *filename, int status);

int llwrite(int fd, unsigned char *buffer, int length);

int llread(int fd, unsigned char *buffer);

int llclose(int fd);



int process_state_machine5(int fd, unsigned char *expected5);

int connect(int fd);
int connect_transmiter(int fd);
int connect_receiver(int fd);

int disconnect(int fd);
int disconnect_transmiter(int fd);
int disconnect_receiver(int fd);

int write_packet(int fd, unsigned char *buffer, int length);
int read_byte(int fd, unsigned char *buffer);

int control_uknowledge_transmiter(int fd, unsigned char *buffer_control, unsigned char *buffer_uknowledge);
int control_uknowledge_receiver(int fd, unsigned char *buffer_control, unsigned char *buffer_uknowledge);

int byte_stuffing(unsigned char* data_frame, int length);

unsigned char BCC2(unsigned char* data_frame, int length);


/*
Creates an information frame from given parameters (F | A | C | BCC1 | D1..DN | BCC2 | F)
*/

int createIFrame(unsigned char* data_frame, int length);

/*
Resets the linkLayer frame buffer
*/
void emptyFrame();


#endif
