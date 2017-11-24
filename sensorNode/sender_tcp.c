/*
 * sender_tcp.c
 *
 *  Created on: Jan 3, 2017
 *      Author: tbriggs
 */

/*
 * sender.c
 *
 *  Created on: Jan 2, 2017
 *      Author: tbriggs
 */

#include <contiki.h>
#include <contiki-net.h>
#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include <simple-udp.h>
#include <sys/node-id.h>

#include "../config.h"
#include "../message.h"

#include "sensors.h"
#include "sender.h"

#define TCP_PORT 5323

static	uip_ipaddr_t dest;
static struct psock ps;


typedef struct {
	char *buffer;
	int num_bytes;
} process_post_t;

static process_event_t sender_event;
static uint8_t psock_buff[256];

PROCESS(sender_process, "Sender process");
PROCESS(receiver_process, "Receiver process");

static int check_address(const uip_ipaddr_t *addr)
{
	uint8_t *ptr = (uint8_t *) addr;
	int i;

	for (i = 0; i < sizeof(uip_ipaddr_t); i++) {
		if (ptr[i] != 0)
			return 1;
	}

	return 0;
}

void sender_send_data(const sender_data_t *data)
{
	static int sequence = 0;
	int i;

	static char send_buffer[256] = {0};
	static int send_bytes = 0;


	data_t message;
	message.header = DATA_HEADER;
	message.battery = data->battery_mon;
	message.sequence = sequence++;

	//TODO need to implement this
	message.temperature = 0;

	for (i = 0; i < DATA_NUM_ADC; i++)
		message.adc[i] = data->adc[i];

	memcpy(&send_buffer, &message, sizeof(message));
	send_bytes = sizeof(message);

	process_post_t post;
	post.buffer = &message;
	post.num_bytes = sizeof(message);

	process_post_synch(&sender_process, sender_event, &post);
}



PROCESS_THREAD(sender_process, ev, data)
{
	PROCESS_BEGIN();



	while (1) {
		printf("Sender waiting for event : %d\n", ev);
		PROCESS_WAIT_EVENT_UNTIL((ev == sender_event) || (ev == tcpip_event));

		if (ev == tcpip_event) {
			printf("Warning - tcp/ip event detected\r\n");
			continue;
		}

		else if (ev == sender_event) {
			if (! uip_connected()) {
				printf("Error - not connected, discarding\r\n");
				continue;
			}

			process_post_t *post_data = (process_post_t *) data;

			printf("Sending posted sensor data to ");
			uip_debug_ipaddr_print(&dest);
			printf("\n");

			message_print_data((data_t *) post_data->buffer);

			PSOCK_BEGIN(&ps);
			PSOCK_SEND(&ps, post_data->buffer, post_data->num_bytes );
			PSOCK_END(&ps);
		}
	}

	PROCESS_END();

}



PROCESS_THREAD(receiver_process, ev, data)
{


	PROCESS_BEGIN();
	static struct etimer et;
	int connected = 0;

	/* Delay 15 seconds */
	etimer_set(&et, 15 * CLOCK_SECOND);

	PSOCK_INIT(&ps, psock_buff, sizeof(psock_buff));


	while(1) {

		// check if the receiver's destination is known
		config_get_receiver(&dest);
		if (check_address(&dest) == 0) {
			printf("Error - no receiver address, discarding.\n");
			 etimer_reset(&et);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
			continue;
		}

		if (!connected) {
			printf("Calling tcp_connect\n");
			tcp_connect(&dest, UIP_HTONS(TCP_PORT), NULL);
			PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

			if (uip_aborted() || uip_timedout() || uip_closed()) {
				printf("Could not establish connection\n");
				etimer_reset(&et);
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
			}
			else {
				printf("Connected %d!\n", uip_connected());
				connected = 1;
			} // end else connected
		}
		else {
			PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
			if (uip_aborted() || uip_timedout() || uip_closed()) {
				printf("Lost connection\r\n");
				connected = 0;
				etimer_reset(&et);
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
			}
		}
	}

	PROCESS_END();

	printf("Error - sender process exiting\n");
}


void sender_init( )
{
	process_start(&receiver_process, NULL);
	process_start(&sender_process, NULL);
}
