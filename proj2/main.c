#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "connection.h"

static void print_usage()
{
    printf("Usage: ./download <FTP_URL>\n");
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    if (argc != 2)
    {
        print_usage();
        return -1;
    }

    struct UrlData* url_data = parse_input(argv[1]);

    if (url_data == NULL)
    {
        printf("Error parcing input url.\n");
        print_usage();
        return -1;
    }

    print_urldata(url_data);

    char* ip = get_IP_from_host_name(url_data->host);
    if (ip == NULL)
    {
        printf("Host name does not exist.\n");
        return -1;
    }

    printf("Host name: %s\nIP: %s\n\n", url_data->host, ip);

    int sock_ftp_command_fd = -1;

    if ((sock_ftp_command_fd = connect_to_socket(ip, FTP_COMMAND_PORT, true)) == -1)
    {
        printf("Error connecting to command socket.\n");
        return -1;
    }

    printf("Connected to ftp command server (%s) on socket port 21 with file descriptor %d.\n\n", ip, sock_ftp_command_fd);

    printf("Logging in...\n");
    if (login(sock_ftp_command_fd, url_data->user, url_data->password) == -1)
    {
        printf("Error logging in\n\n");
        return -1;
    }
    printf("Logged In!\n\n");

    printf("Entering passive mode...\n");
    struct pasv_connection_info* pasv_connection_info = NULL;
    if ((pasv_connection_info = enter_pasv(sock_ftp_command_fd)) == NULL)
    {
        printf("Error entering passive mode\n\n");
        return -1;
    }
    print_pasv_connection_info(pasv_connection_info);
    printf("Passive mode entered!\n\n");

    int sock_ftp_server_fd = -1;

    if ((sock_ftp_server_fd = connect_to_socket(pasv_connection_info->ip, pasv_connection_info->port, false)) == -1)
    {
        printf("Error connecting to server socket.\n");
        return -1;
    }

    printf("Connected to ftp response server (%s) on socket port %d with file descriptor %d.\n\n", pasv_connection_info->ip, pasv_connection_info->port, sock_ftp_server_fd);

    printf("Tranfering file...\n");

    if (transfer_file(sock_ftp_command_fd, url_data->url_path) == -1)
    {
        printf("Error transfering file.\n");
        return -1;
    }

    printf("File tranfered!\n\n");

    printf("Reading file...\n");

    if (receive_file(sock_ftp_server_fd) == -1)
    {
        printf("Error receiving file.\n");
        return -1;
    }

    printf("File read and saved!\n\n");

    if (close(sock_ftp_command_fd)<0) {
        perror("command close()");
        return -1;
    }

    printf("Closed ftp command server socket with file descriptor %d.\n", sock_ftp_command_fd);

    if (close(sock_ftp_server_fd)<0) {
        perror("server close()");
        return -1;
    }

    printf("Closed ftp response server socket with file descriptor %d.\n", sock_ftp_server_fd);

    return 0;
}