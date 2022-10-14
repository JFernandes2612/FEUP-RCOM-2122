#include "statemachine.h"

#include <stdio.h>

int state_machine5(unsigned char received_value, unsigned char *expected5, unsigned char state)
{
    switch (state)
    {
    case 0:
        if (received_value == expected5[0])
            state++;
        break;
    case 1:
        if (received_value == expected5[1])
            state++;
        else if (received_value == expected5[0])
            break;
        else
            state = 0;
        break;
    case 2:
        if (received_value == expected5[2])
            state++;
        else if (received_value == expected5[0])
            state = 1;
        else
            state = 0;
        break;
    case 3:
        if (received_value == expected5[3])
            state++;
        else if (received_value == expected5[0])
            state = 1;
        else
            state = 0;
        break;
    case 4:
        if (received_value == expected5[4])
            return -2;
        else
            state = 0;
        break;
    }
    return state;
}
