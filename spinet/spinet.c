//#include "contiki.h"
//#include "spi-iface.h"
//#include "rpl-iface.h"
//#include "sys/log.h"
//
//#define LOG_MODULE "SPINet"
//#define LOG_LEVEL LOG_LEVEL_DBG
//
//
///*---------------------------------------------------------------------------*/
///* Declare and auto-start this file's process */
//PROCESS(spinet, "SPI Net");
//AUTOSTART_PROCESSES(&spinet);
//PROCESS_THREAD(spinet, ev, data)
//{
//  PROCESS_BEGIN();
//
//  LOG_INFO("Contiki-NG SPI NET Starting\n");
//  LOG_INFO("Compile date %s %s\n", __DATE__, __TIME__);
//
//  spi_init ();
//  rpl_init();
//
//  /*/\/\/\/ network routing /\/\/\/ */
//  process_start(&spi_rdcmd, NULL);
//  // enter main loop
//  while (1)
//    {
//  		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//    }
//PROCESS_END();
//}
//

/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */
#include <contiki.h>
#include <sys/clock.h>
#
#include <stdio.h> /* For printf() */
#include <sys/energest.h>
#include "sys/log.h"
#include "spi-iface.h"
#include "rpl-iface.h"


#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#define LOG_MODULE "spinet"
#define LOG_LEVEL LOG_LEVEL_ERR
//
///*---------------------------------------------------------------------------*/
//PROCESS(spinet, "spinet");
//AUTOSTART_PROCESSES(&spinet);
///*---------------------------------------------------------------------------*/
//PROCESS_THREAD(spinet, ev, data)
//{
//  static struct etimer timer;
//
//  PROCESS_BEGIN();
//
//  NETSTACK_MAC.off( );
//
//  spi_init();
//  tsch_set_coordinator(1);
//
//  NETSTACK_ROUTING.root_start();
//  NETSTACK_MAC.on();
//
//
//  etimer_set(&timer, CLOCK_SECOND * 30);
//  while(1) {
//
//    /* Wait for the periodic timer to expire and then restart the timer. */
//    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
//    printf("6TSCH Stats: \n");
//    printf("Routing entries: %u\n", uip_ds6_route_num_routes());
//    printf("Routing links: %u\n", uip_sr_num_nodes());
//
//    etimer_reset(&timer);
//  }
//
//  PROCESS_END();
//}
/*---------------------------------------------------------------------------*/
PROCESS(spinet, "spinet");
AUTOSTART_PROCESSES(&spinet);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(spinet, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t prefix;
  PROCESS_BEGIN();
  //tsch_set_coordinator(0);


  energest_init( );
  NETSTACK_MAC.on();
  spi_init();

  etimer_set(&timer, 5 * CLOCK_SECOND);

  // wait a few seconds
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
//  tsch_set_pan_secured(0);
//  tsch_set_coordinator(1);

  etimer_set(&timer, 1 * CLOCK_SECOND);

	// wait a few seconds
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  const uip_ipaddr_t *default_prefix = uip_ds6_default_prefix();
  uip_ip6addr_copy(&prefix, default_prefix);

  printf("Setting as DAG root\n");
  NETSTACK_ROUTING.root_set_prefix(&prefix, NULL);
  NETSTACK_ROUTING.root_start();

  etimer_set(&timer, 60 * CLOCK_SECOND);
  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    etimer_reset(&timer);
  }

  PROCESS_END();
}
