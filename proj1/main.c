#include "app.h"
#include "alarm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _POSIX_SOURCE 1

void print_usage()
{
  printf("Usage: rcom SerialPort <R | T>\n");
}

int parse_args(int argc, char **argv)
{
  if ((argc < 3) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    print_usage();
    return 1;
  }
  if (strcmp("R", argv[2]) == 0)
    appLayer->status = RECEIVER;
  else if (strcmp("T", argv[2]) == 0)
    appLayer->status = TRANSMITER;
  else
  {
    print_usage();
    return 1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  setbuf(stdout, NULL);

  if (setup_alarm())
    return 1;

  appLayer = (struct applicationLayer *)malloc(sizeof(struct applicationLayer));
  ll = (struct linkLayer *)malloc(sizeof(struct linkLayer));

  if (parse_args(argc, argv))
  {
    free(appLayer);
    free(ll);
    return 1;
  }

  if ((appLayer->fd = llopen(argv[1], appLayer->status)) == -1)
  {
    free(appLayer);
    free(ll);
    return 1;
  }

  if (processImage())
  {
    free(appLayer);
    free(ll);
    return 1;
  }

  if (llclose(appLayer->fd))
  {
    free(appLayer);
    free(ll);
    return 1;
  }

  free(appLayer);
  free(ll);
}
