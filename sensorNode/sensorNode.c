/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */


#include <contiki.h>
#include <sys/clock.h>
#include <stdio.h> /* For printf() */
#include <message-service.h>
#include <echo.h>
#include <config.h>

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

  config_init( );
  messenger_init();
  echo_init();

  LOG_DBG("Size of addr: %lu\n", sizeof(uip_ipaddr_t));

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);
  uip_ip6addr_t addr;

  uiplib_ip6addrconv("fd00::1", &addr);

  int i;
  for (i = 0; i < 8; i++) {
	  LOG_DBG("%d - %x\n", i, addr.u16[i]);
  }
  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);

    LOG_INFO("Sending message: hello world");
    const char *msg = "Hello";

    uiplib_ip6addrconv("fd00::1", &addr);
    messenger_send(&addr, msg, strlen(msg));

  }

  PROCESS_END();
}
