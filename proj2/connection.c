#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int connect_to_socket(const char *ip, const int port, const bool check_output)
{
    int sockfd;
    struct sockaddr_in server_addr;

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return -1;
    }

    printf("Connecting...\n");
    //let connection take place before reading
    sleep(1);

    if (check_output)
    {

        if (!verify_output(sockfd, S_RDY_NUSER))
            return -1;
    }

    return sockfd;
}

int login(const int sockfd, const char *user, const char *password)
{
    if (!create_buff_write_verify(sockfd, "user", user, USER_OK_NPASS))
        return -1;

    printf("Username '%s' entered with success.\n", user);

    /////////////////////////////////////////////////////////////////////

    if (!create_buff_write_verify(sockfd, "pass", password, LOGGED_IN))
        return -1;

    printf("Password '%s' entered with success.\n", password);

    return 0;
}

int transfer_file(const int sockfd, const char *path)
{
    if (!create_buff_write_verify(sockfd, "retr", path, FILE_OK))
        return -1;

    printf("Initiated file transfer of '%s'\n", path);

    return verify_output(sockfd, FILE_TRANF_COMP);
}

int receive_file(const int sockfd)
{
    char c;

    int received_fd = -1; 
    if ((received_fd = open("file", O_APPEND | O_CREAT | O_WRONLY, 0777)) == -1)
    {
        perror("open()");
        return -1;
    }
    

    while (read(sockfd, &c, 1) > 0)
    {
        if (write(received_fd, &c, 1) < 0)
        {
            perror("write()");
            return -1;
        }
    }

    close(received_fd);

    return 0;
}

struct pasv_connection_info *enter_pasv(const int sockfd)
{
    if (write(sockfd, "pasv\n", 5) < 0)
    {
        perror("write()");
        return NULL;
    }

    char buff[1024];
    if (read(sockfd, buff, 1024) <= 0)
    {
        perror("read()");
        return NULL;
    }

    //printf("buff - %s\n", buff);

    char code_read[4];
    strncpy(code_read, buff, 3);
    code_read[3] = 0;

    if (strcmp(code_read, ENT_PASV) != 0)
        return NULL;

    char ipdata[30];
    int ipdata_i = 0;
    bool found_ipdata = false;

    for (int i = 0; i < 1000; i++)
    {
        if (buff[i] == '(')
        {
            found_ipdata = true;
            continue;
        }

        if (buff[i] == ')')
            break;

        if (found_ipdata)
        {
            ipdata[ipdata_i] = buff[i];
            ipdata_i++;
        }
    }

    ipdata[ipdata_i] = ',';
    ipdata[ipdata_i + 1] = 0;

    //printf("%s\n", ipdata);

    int num_of_commas = 0;
    char ip[30];
    int ip_i = 0;
    int port_i = 0;
    char port1[4];
    char port2[4];

    for (int i = 0; i < ipdata_i + 1; i++)
    {
        if (num_of_commas < 4)
        {
            if (ipdata[i] == ',')
            {
                ip[ip_i] = '.';
                num_of_commas++;
            }
            else
            {
                ip[ip_i] = ipdata[i];
            }
            ip_i++;
        }
        else if (num_of_commas == 4 || num_of_commas == 5)
        {
            if (ipdata[i] == ',')
            {
                if (num_of_commas == 4)
                    port1[port_i] = 0;
                else if (num_of_commas == 5)
                    port2[port_i] = 0;
                port_i = 0;
                num_of_commas++;
                continue;
            }
            else
            {
                if (num_of_commas == 4)
                    port1[port_i] = ipdata[i];
                else if (num_of_commas == 5)
                    port2[port_i] = ipdata[i];
            }
            port_i++;
        }
        else
        {
            break;
        }
    }

    ip[ip_i - 1] = 0;

    int port = atoi(port1) * 256 + atoi(port2);

    struct pasv_connection_info *ret = malloc(sizeof(struct pasv_connection_info));
    strcpy(ret->ip, ip);
    ret->port = port;

    return ret;
}

void print_pasv_connection_info(struct pasv_connection_info *pasv_connection_info)
{
    printf("Passive mode ip: %s\n", pasv_connection_info->ip);
    printf("Passive mode port: %d\n", pasv_connection_info->port);
}

bool verify_output(const int sockfd, const char *code)
{
    char buff[1024];
    
    if (read(sockfd, buff, 1024) <= 0)
    {
        perror("read()");
        return false;
    }

    ////////////////////////////
    //printf("buff - %s", buff);
    ////////////////////////////

    char code_read[4];
    strncpy(code_read, buff, 3);
    code_read[3] = 0;

    ///////////////////////////////////
    //printf("resp - %s\n", code_read);
    ///////////////////////////////////

    if (strcmp(code_read, code) != 0)
        return false;

    return true;
}

bool write_verify(const int sockfd, const char *buff, const char *code)
{
    //printf("cmd - %s", buff);

    if (write(sockfd, buff, strlen(buff)) < 0)
    {
        perror("write()");
        return false;
    }

    if (!verify_output(sockfd, code))
        return false;

    return true;
}

char *create_buff(const char *command, const char *buff)
{
    char *ret = (char *)malloc(1024);

    sprintf(ret, "%s %s\n", command, buff);

    ret = (char *)realloc(ret, strlen(ret));

    return ret;
}

bool create_buff_write_verify(const int sockfd, const char *command, const char *buff, const char *code)
{
    char *final_buff = create_buff(command, buff);

    if (!write_verify(sockfd, final_buff, code))
    {
        free(final_buff);
        return false;
    }

    free(final_buff);
    return true;
}