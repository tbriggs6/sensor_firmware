/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */
#include <contiki.h>
#include <sys/clock.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#include <stdio.h> /* For printf() */
#include <sys/energest.h>
#include "sys/log.h"

#include "../modules/config/config.h"
#include "../modules/echo/echo.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/command.h"
#include "sys/log.h"
#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG
#include "../modules/command/message.h"


#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG

static int sequence = 0;
void send_fake_cal( )
{
	static uip_ip6addr_t addr;
	static airborne_cal_t cal_data;

	config_get_receiver (&addr);

	memset(&cal_data, 0, sizeof(cal_data));
	cal_data.sequence = sequence++;
	cal_data.header = AIRBORNE_CAL_HEADER;

	// send data
	messenger_send (&addr, (void *) &cal_data, sizeof(cal_data));
}

void send_fake_data( )
{
	static uip_ip6addr_t addr;
	static airborne_t data;

	config_get_receiver (&addr);

	memset(&data, 0, sizeof(data));
	data.sequence = sequence++;
	data.header = AIRBORNE_HEADER;

	// send data
	messenger_send (&addr, (void *) &data, sizeof(data));

}


/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  static struct etimer timer;
  static uip_ip6addr_t addr;
  static int send_cal_data = 1;

  int send_len, recv_len, status;

  PROCESS_BEGIN();
  tsch_set_coordinator(0);

  energest_init( );
  NETSTACK_MAC.on();

  // let things settle....
  etimer_set(&timer, CLOCK_SECOND );

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  config_init( );
  messenger_init();
  echo_init();
  command_init( );


    uiplib_ip6addrconv("fd00::1", &addr);
    config_set_receiver(&addr);

  LOG_INFO("Stored destination address: ");
  config_get_receiver (&addr);
  LOG_6ADDR(LOG_LEVEL_INFO, &addr);
  LOG_INFO_("\n");

  etimer_set(&timer, 60 * CLOCK_SECOND );
  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    if (send_cal_data == 1)
    	send_fake_cal( );
    else
    	send_fake_data( );

    // start the wait loop
    etimer_set(&timer, 30*CLOCK_SECOND);

    while(1) {
    	PROCESS_WAIT_EVENT();
    	printf("Event received: %d\n", ev);

    	if (etimer_expired(&timer)) {
    		printf("Timeout waiting for response\n");
    		break;
    	}
    	else if (ev == PROCESS_EVENT_MSG) {
    		printf("Received message send event\n");


    		messenger_get_last_result(&send_len, &recv_len, sizeof(status), &status);
    		printf("Result: send: %d, recv: %d, status: %d\n", send_len, recv_len, status);
    		if (status >= 0) send_cal_data = 0;
    		break;
    	}
    }

    // wait for the next timer.
    etimer_set(&timer, 60 * CLOCK_SECOND );
  }

  PROCESS_END();
}

