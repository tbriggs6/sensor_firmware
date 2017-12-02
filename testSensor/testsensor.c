/*
 * testsensor.c
 *
 *  Created on: Nov 22, 2017
 *      Author: contiki
 */

#include <contiki.h>
#include <stdio.h>
#include <bcast-service.h>
#include <net/ip/uip-debug.h>
#include <net/ipv6/uip-ds6.h>

PROCESS(test_bcast_cb, "Test Sensor Broadcast Handler");

AUTOSTART_PROCESSES(&test_bcast_cb);



PROCESS_THREAD(test_bcast_cb, ev, data)
{
    PROCESS_BEGIN( );

    bcast_init(0);

    bcast_add_observer(&test_bcast_cb);

    printf("Broadcast CB started\n");
    while(1)
    {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            bcast_t *bcast = (bcast_t *) data;
            printf("******************\n");
            printf("bcast event: ");
            printf("from: ");
            uip_debug_ipaddr_print(&(bcast->sender_addr));
            printf("  port: %d\n", bcast->sender_port);

            printf("len: %d message: %s\n", bcast->datalen, bcast->data);
         }
        else {
            printf("Different event: %d\n", ev);
        }
    }

    bcast_remove_observer(&test_bcast_cb);

    PROCESS_END();
}
