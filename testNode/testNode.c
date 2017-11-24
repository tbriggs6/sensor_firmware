/*
 * testNode.c
 *
 *  Created on: Nov 15, 2017
 *      Author: tbriggs
 */

#include <contiki.h>
#include <random.h>

#include <sys/clock.h>

#include <dev/leds.h>


#include "contiki-net.h"
#include "contiki-lib.h"
#include "net/ip/uip.h"



#include "dev/serial-line.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "sender_udp.h"
#include "../utils.h"

#ifdef CFGPERIPH
#include <board-peripherals.h>
#include <dev/uart1.h>
#include "ieee-addr.h"
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif



void send(uip_ipaddr_t *dest, const char *message)
{
  printf("Sending message to: ");
  uip_debug_ipaddr_print(dest);
  printf("\n");

  printf("Message: %s\n", message);

  send_string(dest, message, strlen(message));

  uip_debug_ipaddr_print(dest);



} // end function

void parse_bcast(char *str)
{
  int i;

  for (i = 0; str[i] != 0; i++) {
      if (str[i] != ' ') break;
  }


  printf("Message: %s\n", &str[i]);

  char *ptr = &str[i];
  broadcast_string(ptr, strlen(ptr));

} // end function


void parse_addr(char *str, uip_ipaddr_t *addr)
{
  int i;

  for (i = 0; str[i] != 0; i++) {
      if (str[i] != ' ') break;
  }

  printf("address: %s\n", &str[i]);
  uip_from_string(&str[i], addr);

  printf("****************************\n");
  printf("Address parsed: ");
  uip_debug_ipaddr_print(addr);
  printf("\n");
  printf("****************************\n");

} // end function



static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}

void show_neighbors( )
{
  static uip_ds6_route_t *r;
  static uip_ds6_nbr_t *nbr;

  printf("*************** Neighbors ******************\n");
  for (nbr = nbr_table_head(ds6_neighbors);
       nbr != NULL;
       nbr = nbr_table_next(ds6_neighbors, nbr))
  {
      printf("addr: ");
      uip_debug_ipaddr_print(&(nbr->ipaddr));
      printf("  state: ");
      switch (nbr->state) {
          case NBR_INCOMPLETE: printf(" INCOMPLETE");break;
          case NBR_REACHABLE: printf(" REACHABLE");break;
          case NBR_STALE: printf(" STALE");break;
          case NBR_DELAY: printf(" DELAY");break;
          case NBR_PROBE: printf(" NBR_PROBE");break;
          }
      printf("\n");
  }

  printf("**************** Routes ******************\n");
  for (r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r))
    {
      printf("addr: ");
      uip_debug_ipaddr_print(&r->ipaddr);
      printf("/%u via(", r->length);
      uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
      printf(" %lus\n", (unsigned long) r->state.lifetime);
    }

}


PROCESS(test_process, "Test Process");
AUTOSTART_PROCESSES(&test_process);

PROCESS_THREAD(test_process, ev, data)
{
	PROCESS_BEGIN( );

	// start off with receiver pointing here
	static uip_ipaddr_t receiver;
	memcpy(&receiver, &uip_ds6_if.addr_list[0].ipaddr, sizeof(receiver));

#ifdef	CFGPERIPH
	uart1_set_input(serial_line_input_byte);
#endif

	serial_line_init( );

	PRINTF("Starting power trace, %u ticks per second\n", RTIMER_SECOND);
	energest_init();
	powertrace_start(600 * CLOCK_SECOND);


	printf("Local addresses: \n");
	print_local_addresses();

	printf("Test Node v0.1  --- Minicom users: use ctrl-J for new-line\n");

	static int done = 0;
	static int count = 0;
	while(!done) {
	    printf("Address is set to: ");
	    uip_debug_ipaddr_print(&receiver);
	    printf("\n");

	    printf(" %d> ", count++);

	    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);

	    char *ptr = (char *) data;
	    printf("(%s)\n", ptr);

	    if (!strcasecmp(ptr,"HELP")) {
		printf("*****************\n");
		printf("ADDR ipv6_addr - sets the destination address\n");
		printf("SEND message - sends message to address ");
		uip_debug_ipaddr_print(&receiver);
		printf("\n");
		printf("BCAST message - broadcast message to all hosts\n");
		printf("STAT - show network status\n");
		printf("SERVER - become server\n");
		printf("ROOT - become RPL root\n");
		printf("RPL - repair RPL\n");
		printf("\n");
		printf("CLIENT - become client\n");
	    }

	    else if (!strncasecmp(ptr, "SERVER", 6)) {
		start_server();
	    }
	    else if (!strcasecmp(ptr,"ROOT")) {
		become_root( );
	    }
	    else if (!strcasecmp(ptr,"RPL")) {
		repair_root();
	    }
	    else if (!strncasecmp(ptr, "CLIENT", 5)) {
		start_client();
	    }
	    else if (!strncasecmp(ptr, "ADDR ",5)) {
		parse_addr(ptr+5, &receiver);
		printf("Address is set to: ");
		uip_debug_ipaddr_print(&receiver);
		printf("\n");
	    }
	    else if (!strncasecmp(ptr, "ROOT", 4)) {
		become_root();
	    }
	    else if (!strncasecmp(ptr, "STAT", 4)) {
		show_neighbors();
	    }
	    else if (!strncasecmp(ptr,"SEND ",5)) {
		send(&receiver, ptr+5);
		PROCESS_WAIT_UNTIL(ev == send_fin_event);
		int *status = (int *) data;

		printf("Send finished: %d\n", *status);

	    }
	    else if (!strncasecmp(ptr,"BCAST ",6)) {
		printf("Broadcasting\n");
		parse_bcast(ptr+6);
	    }
	    else {
		printf("Unknown command\n");
	    }

	}

	PROCESS_END();
	return 0;
}



