/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */


#include <contiki.h>
#include <sys/clock.h>
#include <stdio.h> /* For printf() */
#include <sys/energest.h>

#include "../modules/config/config.h"
#include "../modules/echo/echo.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/command.h"
#include "sys/log.h"
#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG
#include "../modules/command/message.h"

#include "sensor.h"

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
  command_init( );
  energest_init( );
  sensor_init( );

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 60);
  uip_ip6addr_t addr;

  uiplib_ip6addrconv("fd00::1", &addr);
  config_set_receiver(&addr);

  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}

