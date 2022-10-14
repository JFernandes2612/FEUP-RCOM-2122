#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

struct UrlData *parse_input(const char *input)
{
    size_t sz = strlen(input);

    struct UrlData *ret = (struct UrlData *)malloc(sizeof(struct UrlData));
    memset(ret, 0, sizeof(struct UrlData));

    char *buff = malloc(sz);

    buff = memset(buff, 0, sz);
    buff = strncpy(buff, input, 6);

    if (strcmp(buff, "ftp://"))
    {
        free(ret);
        return NULL;
    }
    buff = memset(buff, 0, sz);

    size_t buff_i = 0;

    bool has_user = false;
    bool has_password = false;
    bool has_host = false;

    for (size_t i = 6; i < sz; i++)
    {
        if (isalnum(input[i]) || input[i] == '.' || input[i] == '_' || input[i] == '-')
        {
            buff[buff_i] = input[i];
        }
        else if (input[i] == ':')
        {
            if (has_user)
            {
                free(ret);
                return NULL;
            }
            else if (!has_user && !has_password && !has_host)
            {
                strcpy(ret->user, buff);
                buff = memset(buff, 0, sz);
                buff_i = 0;
                has_user = true;
                continue;
            }
        }
        else if (input[i] == '@')
        {
            if (has_password)
            {
                free(ret);
                return NULL;
            }
            else if (has_user)
            {
                strcpy(ret->password, buff);
                buff = memset(buff, 0, sz);
                buff_i = 0;
                has_password = true;
                continue;
            }
            else
            {
                free(ret);
                return NULL;
            }
        }
        else if (input[i] == '/')
        {
            if (has_user && !has_password)
            {
                free(ret);
                return NULL;
            }
            else if (!has_host)
            {
                strcpy(ret->host, buff);
                buff = memset(buff, 0, sz);
                buff_i = 0;
                has_host = true;
                continue;
            }
            else
            {
                buff[buff_i] = input[i];
            }
        }
        else
        {
            free(ret);
            return NULL;
        }

        buff_i++;
    }

    if (buff[0] == 0 || buff[0] == '/')
    {
        free(ret);
        return NULL;
    }
    strcpy(ret->url_path, buff);

    if ((has_user && !has_password) || !has_host)
    {
        free(ret);
        return NULL;
    }

    if (!has_user || ret->user[0] == 0 || ret->password[0] == 0)
    {
        strcpy(ret->user, "anonymous");
        strcpy(ret->password, "password");
    }

    return ret;
}

void print_urldata(const struct UrlData *urldata)
{
    printf("User: %s\n", urldata->user);
    printf("Password: %s\n", urldata->password);
    printf("Host: %s\n", urldata->host);
    printf("Url Path: %s\n\n", urldata->url_path);
}

char *get_IP_from_host_name(const char *host_name)
{
    struct hostent *h;

    if ((h = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname()");
        return NULL;
    }

    return inet_ntoa(*((struct in_addr *)h->h_addr));
}