/*
 * sender.c
 *
 *  Created on: Jan 2, 2017
 *      Author: tbriggs
 */

#include <contiki.h>
#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include <simple-udp.h>
#include <sys/node-id.h>
#include <random.h>
#include <dev/leds.h>
#include "scif_osal_contiki.h"

#include "../config.h"
#include "../message.h"
#include "sensors.h"
#include "sender.h"

// one of the net debug functions has this in it...
#ifdef PRINTF
#undef PRINTF
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define UDP_PORT (5323)
#define MAX_RETRIES (50)

#define RETRY_MAX (60 * CLOCK_SECOND)

#define RETRY_DELAY (random_rand() % RETRY_MAX)

static struct simple_udp_connection sender_connection;

process_event_t sender_event;
process_event_t ack_event;
process_event_t sender_finish_event;

static volatile data_ack_t last_ack;

/**
 * Checks the given address to see if it is non-zero,
 * and presumably valid.
 */
static int check_address(const uip_ipaddr_t *addr)
{
	uint8_t *ptr = (uint8_t *) addr;
	int i = 0;

	for (i = 0; i < sizeof(uip_ipaddr_t); i++) {
		if (ptr[i] != 0)
			return 1;
	}

	return 0;
}

/**
 * Process to handle exchanging packets with the remote server.
  */

PROCESS(sender_process, "Data Transceiver process");
PROCESS_THREAD(sender_process, ev, data)
{
	static struct etimer et;		// retry timer
	static int retry_count = 0;		// retry count
	static data_t sensor_data;		// sensor data
	static uip_ipaddr_t receiver;		// the destination addr
	static int done = 0;			// done or not

	// begin process
	PROCESS_BEGIN();

	while (1) {

		memset(&sensor_data, 0, sizeof(sensor_data));
		memset(&receiver, 0, sizeof(receiver));

		PRINTF("\n***********************\n");
		PRINTF("Waiting for a data message\n");

		PROCESS_WAIT_EVENT_UNTIL(ev == sender_event);
		leds_off(LEDS_ALL);

		if (data == NULL) {
			PRINTF("Error - data was NULL\n");
			leds_on(LEDS_RED);
			continue;
		}

		memcpy(&sensor_data, data, sizeof(sensor_data));
		if (sensor_data.header != DATA_HEADER) {
			PRINTF("Error - posted data has header %X, is not a sensor packet\n", (int ) sensor_data.header);
			leds_on(LEDS_RED);
			continue;
		}

		config_get_receiver(&receiver);
#if (DEBUG > 0)
		PRINTF("ROUTER ADDRESS: ");
		uip_debug_ipaddr_print(&receiver);
		printf("\n");
#endif

		if (check_address(&receiver) == 0) {
			PRINTF("Error - no receiver address, discarding.\n");
			leds_on(LEDS_RED);
			continue;
		}


		PRINTF("**********\n");
		PRINTF("Sending packet to router\n");

		done = 0;
		retry_count = 0;
		while (!done && (retry_count < MAX_RETRIES)) {
			simple_udp_sendto_port(&sender_connection, (char *) &sensor_data, sizeof(data_t), &receiver, UDP_PORT);


			etimer_set(&et, RETRY_DELAY);

			while(1) {
				PROCESS_WAIT_EVENT();
				PRINTF("Got event ev=%d etimer=%d\n", ev, etimer_expired(&et));
				if (etimer_expired(&et)) break;
				if (ev == PROCESS_EVENT_TIMER) break;
				if (ev == ack_event) break;
			}

			PRINTF(">> Done waiting after send ev=%d\n", ev);

			if (etimer_expired(&et)) {
				PRINTF("Sequence %d Timed out waiting for ACK... count=%d\n", (int ) sensor_data.sequence, retry_count);
				retry_count++;
				done = 0;
			}

			else if (ev == ack_event) {
				if (last_ack.ack_seq == sensor_data.sequence) {
					PRINTF("Received ACK %d\n", *((int * ) data));
					leds_on(LEDS_GREEN);
					// success
					done = 1;
				}
				else if (last_ack.ack_seq > sensor_data.sequence) {
					PRINTF("Received ACK (%lu) > than sequence(%d), aborting this send.\n", last_ack.ack_seq,
							(int ) sensor_data.sequence);
					leds_on(LEDS_RED);
					// abort
					done = 1;
				}
				else {
					PRINTF("Received ACK (%lu) < than sequence (%d), retrying\n", last_ack.ack_seq,
							(int ) sensor_data.sequence);
					retry_count++;
					done = 0;
				}
			}
			else {
				PRINTF("HMMMM ---- not sure why we exited the retry loop\n");
				retry_count++;
				done = 0;
			}
			etimer_stop(&et);

		} // end while retry loop



		// send finished
		if (!done) {
			PRINTF("Send was unsuccessful\n");
			leds_on(LEDS_RED);
		}
		else {
			PRINTF("Send was successful\n");
			leds_on(LEDS_GREEN);
		}

		// wake up the alert interrupt
		process_post(&alert_interrupt, sender_finish_event, &done);

	} // end main while loop

PROCESS_END( );

}

static data_t send_data_message;
void sender_send_data(const sender_data_t *sensor_data)
{
int i;
static int sequence = 0;

send_data_message.header = DATA_HEADER;
send_data_message.battery = sensor_data->battery_mon;
send_data_message.sequence = sequence++;

//TODO need to implement this
send_data_message.temperature = 0;

for (i = 0; i < DATA_NUM_ADC; i++)
	send_data_message.adc[i] = sensor_data->adc[i];

#if (DEBUG > 0)
	PRINTF("Posting message: ");
	message_print_data(&send_data_message);
#endif

	process_post(&sender_process, sender_event, &send_data_message);
}

/*
 * sender_receiver - this is the call back from the reception of a UDP
 * packet.
 */
static void sender_receiver(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
	const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t length)
{
if (length == sizeof(data_ack_t)) {
	data_ack_t *ack = (data_ack_t *) data;
	if (ack->header != DATA_ACK_HEADER)
		return;

	memcpy((void *) &last_ack, data, sizeof(data_ack_t));

	PRINTF("SEND: sender_receiver posting receipt of ACK packet, value: %lu\n", ack->ack_value);
	process_post(&sender_process, ack_event, &(ack->ack_value));
	PRINTF("Back from posting ACK\n");

}
else {
	PRINTF("Received a non-ACK packet, ignoring?\n");
}
}

void sender_init()
{

	PRINTF("Initializing SENDER\n");

	simple_udp_register(&sender_connection, 0, NULL, UDP_PORT, sender_receiver);

	sender_event = process_alloc_event();
	ack_event = process_alloc_event();
	sender_finish_event = process_alloc_event();

	PRINTF("   ** Sender Event: %d  Ack Event: %d **\n", sender_event, ack_event);
	memset((void *) &last_ack, 0, sizeof(last_ack));

	leds_on(LEDS_ALL);

	process_start(&sender_process, NULL);
}

