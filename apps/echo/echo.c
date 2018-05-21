#include <contiki.h>
#include "../apps/messenger/message-service.h"


int echo_handler(const uint8_t *inputdata, int inputlength, uint8_t *outputdata, int *maxoutputlen)
{
    if (inputlength < *maxoutputlen)
        *maxoutputlen = inputlength;

    memcpy(outputdata, inputdata, *maxoutputlen);

    return 1;
}

void echo_init( )
{

    //                    "echo"
    messenger_add_handler(0x6f686365,   4,  99, echo_handler);
}
