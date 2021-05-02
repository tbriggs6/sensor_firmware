/*
 * @file message-service.c
 * @author Tom Briggs
 * @brief The message delivery utility
 *
 * The message sender encapsulates the ability to act
 * as a client of a remote server.  This code allows the
 * sensor to open a connection to the remote server, send
 * a message (e.g. sensor readings), collect the result,
 * and then close the socket.
 */

#define DEBUG

#include "../../modules/messenger/message-service.h"
#include "../../modules/command/message.h"

#include <contiki.h>
#include <contiki-net.h>

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "SENDER"
#define LOG_LEVEL LOG_LEVEL_INFO

#define DEBUG
#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

#define MAX_ATTEMPTS (8)
#define RETRY_DELAY (CLOCK_CONF_SECOND * 2 )
#define ACK_OK (1)

// events used within the messenger module
process_event_t sender_start_event;
process_event_t sender_fin_event;

// the UDP connection to use / listen to
static struct simple_udp_connection conn;

// the process that is requesting the send
static struct process *requestor;

// a timer used to control resending the data
static struct etimer msg_timer;

// the data buffer,length, and sequence that is currently be sent/resent
static uint8_t current_data[UIP_BUFSIZE];
static uint16_t current_length = 0;
static uint16_t current_sequence = 0;

// how many attempts to deliver this segment?
static int current_attempt = 0;

// the last RSSI
static radio_value_t last_dag_rssi = 0;

// how did the last transaction end?
static int last_ack_ok = 0;
static int send_started = 0;


/**
 * Process for sending / resending data.
 */
PROCESS(messenger_sender, "Messenger Sender");
PROCESS_THREAD(messenger_sender, ev, data)
{
	PROCESS_BEGIN()	;
	etimer_stop(&msg_timer);

	while (1) {
		PROCESS_WAIT_EVENT();

		// a new packet is to be sent
		if (ev == sender_start_event)
		{
			if (send_started == 1) {
				LOG_DBG("Started new send, %d bytes.  Retry interval: %d\n", current_length, RETRY_DELAY);

				current_attempt = 0;
				simple_udp_send(&conn, &current_data, current_length);
				etimer_set(&msg_timer, RETRY_DELAY);
			}
		}


		// a valid response to this packet was receved from server
		else if (ev == sender_fin_event)
		{
			if (send_started == 1) {
				send_started = 0;
				LOG_DBG("Sender: finished seq=%d ok?=%d rssi=%d\n", current_sequence, last_ack_ok, last_dag_rssi);
				etimer_stop(&msg_timer);

				process_post (requestor, PROCESS_EVENT_MSG, NULL);
				etimer_stop(&msg_timer);
			}
			else {
				LOG_DBG("Sender: got duplicate fin event\n");
				etimer_stop(&msg_timer);
			}
		}

		// a timeout while awaiting a response from the server
		else if (etimer_expired(&msg_timer)) {
			if (send_started) {
				// the message was a failure, record that
				if (++current_attempt >= MAX_ATTEMPTS) {
					last_ack_ok = 0;
					send_started = 0;

					LOG_DBG("Sender: seq %d timed out after %d tries\n", current_sequence, current_attempt);
					process_post (requestor, PROCESS_EVENT_MSG, NULL);
					etimer_stop(&msg_timer);
				}

				// try again
				else {
					LOG_DBG("Sender: try again %d\n", current_attempt);
					simple_udp_send(&conn, &current_data, current_length);
					etimer_set(&msg_timer, RETRY_DELAY);
				}
			}
		}


	}
	PROCESS_END()	;
}



void messenger_send (const uip_ipaddr_t *remote_addr, uint16_t sequence, const void *data, int length)
{
	LOG_DBG("message_send ");
	LOG_6ADDR(LOG_LEVEL_DBG, remote_addr);
	LOG_DBG_(" length %d", length);
	LOG_DBG_(" send_started: %d\n", send_started);

	if (send_started != 0) {
		LOG_ERR("Send already in progress...");
		return;
	}

	last_ack_ok = 0;
	send_started = 1;
	current_sequence = sequence;
	requestor = process_current;
	current_attempt = 0;

	memcpy(current_data, data, length);
	current_length = length;

	process_post(&messenger_sender, sender_start_event, NULL);

}


int messenger_recvd_rssi()
{
	return last_dag_rssi;
}




int messenger_last_result_okack( )
{
	return last_ack_ok;
}

void messenger_callback(struct simple_udp_connection *c,
                         const uip_ipaddr_t *src_addr, uint16_t src_port,
                         const uip_ipaddr_t *dest_addr, uint16_t dest_port,
                         const uint8_t *data, uint16_t data_len)
{

	/* only expect an acknowledgement packet.
	 * #define ACK_HEADER (0x90983323)
			typedef struct __attribute__((packed)) {
        uint32_t header;
        uint32_t sequence;
        uint32_t ack_seq;
        uint32_t ack_value;
			} ack_t;
	 */
	ack_t *ack = (ack_t *) data;

	if (data_len != sizeof(ack_t)) goto error;

	if (ack->header != ACK_HEADER) goto error;

	if (ack->ack_value != ACK_OK) goto error;

	if (ack->ack_seq != current_sequence) goto error;


	NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &last_dag_rssi);
	last_ack_ok = 1;

	LOG_DBG("Received ACK for seq %d RSSI=%d\n", current_sequence, (int) last_dag_rssi);

	// found a valid ack
	process_post(&messenger_sender, sender_fin_event, NULL);

error:
	// this packet is not for me.
	LOG_DBG("Received message %d =? %d, header %x =? %x, seq %d =? %d, value %d =? %d\n",
	        (int) data_len, (int) sizeof(ack_t),
	        (unsigned int) ack->header, (unsigned int) ACK_HEADER,
	        (int) ack->ack_seq, (int) current_sequence,
	        (int) ack->ack_value, ACK_OK);
}



static int open_connection( )
{
	uip_ip6addr_t remote_addr;
	uint16_t remote_port;

	config_get_receiver (&remote_addr);
	remote_port = config_get_remote_port();

	int rc = simple_udp_register(&conn,		// the connection to register
	                             0, 			// the local port, 0=ephemeral
	                             &remote_addr,	// remote address
	                             remote_port,  // remote port number
	                             messenger_callback  //  callback function
								);

	if (rc != 1) {
		LOG_ERR("Error - could not register UDP socket %d\n", rc);
	}

	return rc;
}

void messenger_sender_init ()
{
	sender_start_event = process_alloc_event ();
	sender_fin_event = process_alloc_event ();

	process_start(&messenger_sender, NULL);

	open_connection();

}
