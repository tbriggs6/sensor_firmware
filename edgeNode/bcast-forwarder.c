#include <contiki.h>
#include <contiki-lib.h>
#include <bcast-service.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/ip/uip-debug.h>

PROCESS(bcast_forwarder, "Broadcast forwarder");

#define DEBUG_BCAST 1

#ifndef NETSTACK_CONF_WITH_IPV6
#error This code requires IPV6
#endif

static struct uip_udp_conn * host_conn;

static int compare_address(const uip_ipaddr_t const *A, const uip_ipaddr_t const *B)
{
    for (int i = 0; i < 8; i++) {
        if (A->u16[i] != B->u16[i]) return A->u16[i] - B->u16[i];
    }
    return 0;
}

PROCESS_THREAD(bcast_forwarder, ev, data)
{
    PROCESS_BEGIN( );
    static char buff[100];

    printf("Forwarder started\n");

    bcast_init(0);
    bcast_add_observer(&bcast_forwarder);

    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
        	if (uip_datalen() >= 100) {
        		printf("Error, data is too large, discarding\n");
        	}
        	else {
        		printf("**** Forwarder received packet: %d ****\n", uip_datalen());
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
	bcast_init(1);

    process_start(&bcast_forwarder, NULL);

}
