#ifndef UTILS_H_
#define UTILS_H_

struct UrlData
{
    char user[100];
    char password[100];
    char host[100];
    char url_path[100];
};

char* get_IP_from_host_name(const char* host_name);

struct UrlData* parse_input(const char* input);

void print_urldata(const struct UrlData* urldata);

#endif