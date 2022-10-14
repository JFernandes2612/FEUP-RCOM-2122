#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdbool.h>

#define FTP_COMMAND_PORT 21

#define S_RDY_NUSER "220"
#define USER_OK_NPASS "331"
#define LOGGED_IN "230"
#define ENT_PASV "227"
#define FILE_OK "150"
#define FILE_TRANF_COMP "226"

struct pasv_connection_info
{
    char ip[30];
    int port;
};

int connect_to_socket(const char* ip, const int port, const bool check_output);

int login(const int sockfd, const char* user, const char* password);

int transfer_file(const int sockfd, const char* path);

int receive_file(const int sockfd);

struct pasv_connection_info* enter_pasv(const int sockfd);

void print_pasv_connection_info(struct pasv_connection_info* pasv_connection_info);

bool verify_output(const int sockfd, const char* code);

bool write_verify(const int sockfd, const char* buff, const char* code);

char* create_buff(const char* command, const char* buff);

bool create_buff_write_verify(const int sockfd, const char* command, const char* buff, const char* code);

#endif