#include <contiki.h>
#include <contiki-lib.h>
#include <bcast-service.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/ip/uip-debug.h>

PROCESS(bcast_forwarder, "Broadcast forwarder");

#define DEBUG 1

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#ifndef NETSTACK_CONF_WITH_IPV6
#error This code requires IPV6
#endif


/**
 * @brief The callback from the bcast-service, this is called whenever the
 * node receives a packet on the broadcast port.  if this is the root node,
 * then any broadcast targeted directly at it will be forwarded to the other
 * nodes in the network.
 */
PROCESS_THREAD(bcast_forwarder, ev, data)
{
    PROCESS_BEGIN( );
    static char buff[100];

    printf("Forwarder started\n");

    // become a sink
    bcast_init(0);
    bcast_add_observer(&bcast_forwarder);

    // become the root node of the broadcast network
    bcast_init(1);


    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
        	if (uip_datalen() >= 100) {
        		printf("Error, data is too large, discarding\n");
        	}
        	else {
        		PRINTF("**** Forwarder received packet: %d ****\n", uip_datalen());
        		memset(buff, 0, sizeof(buff));
				memcpy(buff, uip_appdata, uip_datalen());
				int len = uip_datalen();

				bcast_send(buff, len);
        	}
        }
    }

    PROCESS_END( );
}

void bcast_forwarder_init( )
{
	process_start(&bcast_forwarder, NULL);
}
