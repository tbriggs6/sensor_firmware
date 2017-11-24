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

PROCESS(test_bcast_cb, "Test Sensor Broadcast Handler");
PROCESS(test_sensor_process, "Test Sensor Process");

AUTOSTART_PROCESSES(&test_sensor_process, &test_bcast_cb);



/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // initialize the broadcast receiver
  bcast_init(0);

  PROCESS_END();
}


PROCESS_THREAD(test_bcast_cb, ev, data)
{
    PROCESS_BEGIN( );
    bcast_add_observer(&test_bcast_cb);

    while(1)
    {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            bcast_t *bcast = (bcast_t *) data;
            printf("******************\n");
            printf("bcast event: ");
            printf("from: ");
            uip_debug_ipaddr_print(bcast->sender_addr);
            printf("  port: %d\n", bcast->sender_port);

            printf("len: %d message: %s\n", bcast->datalen, bcast->data);
         }
    }

    bcast_remove_observer(&test_bcast_cb);

    PROCESS_END();
}
