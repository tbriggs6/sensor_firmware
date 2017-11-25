#include <contiki.h>
#include <contiki-lib.h>
#include <bcast-service.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PROCESS(bcast_forwarder, "Broadcast forwarder");


PROCESS_THREAD(bcast_forwarder, ev, data)
{
    PROCESS_BEGIN( );

    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            printf("Received a broadcast\n");
        }
    }

    PROCESS_END( );
}

void bcast_forwarder_init( )
{
    process_start(&bcast_forwarder, NULL);
    bcast_init(BCAST_PORT);
    bcast_add_observer(&bcast_forwarder);
}
