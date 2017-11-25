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

    uip_ipaddr_t host;

    uip_ip6addr(&host, 0xAAAA,0,0,0,0,0,0,1);

    host_conn = udp_new(&host, UIP_HTONS(BCAST_PORT), NULL);

    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {

            bcast_t *bcast = (bcast_t *) data;

#ifdef DEBUG_BCAST
            printf("Received a broadcast\n");
            printf("Sender: ");
            uip_debug_ipaddr_print(&bcast->sender_addr);
            printf(" port: %d\n", bcast->receiver_port);

            printf("Receiver: ");
            uip_debug_ipaddr_print(&bcast->receiver_addr);
            printf(" port: %d\n", bcast->receiver_port);

            printf("Length: %d contents: %s\n", bcast->datalen, bcast->data);
#endif

           if (compare_address(&bcast->receiver_addr, bcast_address()) == 0)
           {
               printf("Mesh to Host\n");
               uip_udp_packet_send(host_conn, bcast->data, bcast->datalen);
           }
           else {
               printf("Host to Mesh\n");
               char data[ uip_datalen() ];
               memcpy(data, uip_appdata, uip_datalen());
               int len = uip_datalen();

               bcast_send(data, len);
           }

        }
    }

    PROCESS_END( );
}

void bcast_forwarder_init( )
{
    process_start(&bcast_forwarder, NULL);
    bcast_init();
    bcast_add_observer(&bcast_forwarder);
}
