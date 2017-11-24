/*
 * testnode.c
 *
 *  Created on: Nov 15, 2017
 *      Author: tbriggs
 */



#include <contiki.h>
#include <random.h>

#include <button-sensor.h>
#include <batmon-sensor.h>
#include <board-peripherals.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <powertrace.h>


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#include <core/net/ip/uip.h>

PROCESS(test_process, "Test Process");
AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data)
{
	PROCESS_BEGIN( );


	PRINTF("Starting power trace, %u ticks per second\n", RTIMER_SECOND);
	energest_init();
	powertrace_start(300 * CLOCK_SECOND);


	// get the link address
	//linkaddr_copy(&message.header.from, &linkaddr_node_addr);
	uip_ipaddr_t hostaddr;

	uip_gethostaddr(&hostaddr);

	int count = 0;
	int done = 0;
	while (!done) {

		char buff[80];
		memset(buff,0,80);


		printf("%x:%x %d> ", hostaddr.u8[1], hostaddr.u8[0], count);

		//fgets(buff, 79, stdin);

		printf("Command: (%s)\r\n", buff);

		count++;
	}


	PROCESS_END();
	return 0;
}
