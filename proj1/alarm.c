#include "alarm.h"

#include <signal.h>
#include <stdio.h>

int setup_alarm()
{
    numAlarms = 0;
    return signal(SIGALRM, alarm_handler) != NULL;
}

void alarm_handler(int a)
{
    numAlarms++;
    printf("alarm\n");
}