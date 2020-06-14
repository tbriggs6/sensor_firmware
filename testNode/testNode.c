/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */
#include "../project-conf.h"
#include <contiki.h>
#include <sys/clock.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#include <stdio.h> /* For printf() */
#include <sys/energest.h>
#include "sys/log.h"

#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  energest_init( );

  etimer_set(&timer, CLOCK_SECOND * 60);

  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    printf("6TSCH Stats: \n");
    printf("Routing entries: %u\n", uip_ds6_route_num_routes());
    printf("Routing links: %u\n", uip_sr_num_nodes());

    etimer_reset(&timer);
  }

  PROCESS_END();
}

