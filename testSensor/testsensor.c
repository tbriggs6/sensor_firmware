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
PROCESS(test_sensor_process, "Test Sensor Process");

AUTOSTART_PROCESSES(&test_sensor_process, &test_bcast_cb);



/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_sensor_process, ev, data)
{
  PROCESS_BEGIN();

  // initialize the broadcast receiver
  bcast_init( );

  static struct etimer timeout;
  static int count = 0;
  static char buff[64];

  static uip_ipaddr_t ipaddr;

  // loads the default prefix (upper 64-bit bits) with the default prefix
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);

  // sets the lower 64-bit of the ipaddr to the MAC ipaddr
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);


  etimer_set(&timeout, 30 * CLOCK_SECOND);

  while(1) {

      PROCESS_YIELD( );
      if (etimer_expired(&timeout)) {
          memset(buff,0,64);
          sprintf(buff,"%-2.2x:%-2.2x:%-2.2x:%-2.2x:%-2.2x:%-2.2x:%-2.2x:%-2.2x msg %d",
                  ipaddr.u8[8],ipaddr.u8[9],ipaddr.u8[10],ipaddr.u8[11],
                  ipaddr.u8[12],ipaddr.u8[13],ipaddr.u8[14],ipaddr.u8[15],
                  count++);

          printf("Sending message: %s\n", buff);
          bcast_send(buff, strlen(buff));
          etimer_set(&timeout, 30 * CLOCK_SECOND);
      }

  }


  PROCESS_END();
}


PROCESS_THREAD(test_bcast_cb, ev, data)
{
    PROCESS_BEGIN( );
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
