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

#include <contiki.h>
#include <sys/clock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <contiki-net.h>
#include <net/ipv6/tcpip.h>

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "SENDER"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif


#define MAX_SEND_BUFFER 128
#define MAX_RECV_BUFFER 128
static uint8_t send_buffer[MAX_SEND_BUFFER];
static uint8_t rcv_buffer[MAX_RECV_BUFFER];
static int send_length, recv_length;
static volatile int send_started = 0;
static struct tcp_socket snd_socket;
static uip_ip6addr_t message_addr;
static struct process *requestor;

process_event_t sender_start_event;
process_event_t sender_fin_event;

PROCESS(messenger_sender, "Messenger Sender");

int messenger_send_result(void * data, int *max_length)
{
    if (recv_length < 0) {
        *max_length = 0;
        return 0;
    }

    if (*max_length < recv_length)
        recv_length = *max_length;

    memcpy(data, rcv_buffer, recv_length);
    return 1;
}

void messenger_send(const uip_ipaddr_t *remote_addr, const void * data, int length)
{
	LOG_DBG("message_send ");
	LOG_6ADDR(LOG_LEVEL_DBG,remote_addr);
	LOG_DBG_(" length %d", length);
	LOG_DBG_(" send_started: %d\n", send_started);

    if (send_started != 0) {
        LOG_ERR("Send already in progress...");
        return;
    }

    send_started = 1;
    if (length > MAX_SEND_BUFFER) {
        LOG_ERR("Request exceeds maximum transfer (%d > %d)", length, MAX_SEND_BUFFER);
    }

    requestor = process_current;

    memcpy(&message_addr, remote_addr, sizeof(uip_ipaddr_t));
    memcpy(send_buffer, data, length);
    send_length = length;

    LOG_DBG("Notifying message sender...\n");
    process_post_synch(&messenger_sender, sender_start_event, NULL);
}


void messenger_get_last_result(int *sendlen, int *recvlen, int maxlen, void *dest)
{
	if (maxlen > recv_length)
		maxlen = recv_length;

	memcpy(dest, rcv_buffer,maxlen);
	*sendlen = send_length;
	*recvlen = recv_length;
}


/* ****************************************************************
 *
 * TCP Sending agent
 *
 * **************************************************************** */
static int sender_recv_bytes(struct tcp_socket *s, void *ptr,
                             const uint8_t *inputptr, int inputdatalen)
{
	if (inputdatalen > 0) {
		LOG_DBG("Sender got %d bytes back from remote\n", inputdatalen);
		recv_length = inputdatalen;
		tcp_socket_close(s);
	}

    return 0;
}

static void sender_recv_event(struct tcp_socket *s, void *ptr, tcp_socket_event_t ev)
{
	LOG_DBG("Sender got event ev=%d ", ev);
    switch(ev) {
    // start of connection
    case TCP_SOCKET_CONNECTED:		// 0
        LOG_DBG_("Socket connected event, sending data\n");
        tcp_socket_send(s, send_buffer, send_length);
        break;

    // during connection
    case TCP_SOCKET_DATA_SENT:		// 4
          LOG_DBG_("Data sent\n");
          break;

    // closing the connection (normally)
    case TCP_SOCKET_CLOSED:			// 1
        LOG_DBG_("Socket closed event\n");
        send_started = 0;
        break;

    // closing the connect (abnormally)
    case TCP_SOCKET_TIMEDOUT:		// 2
        LOG_DBG_("Socket timed out\n");
        send_started = 0;
        send_length = -1;
        break;

    case TCP_SOCKET_ABORTED:		// 3
        LOG_DBG_("Socket aborted\n");
        send_started = 0;
        send_length = -1;
        break;
    }

    if (send_started == 0) {
        process_post(&messenger_sender, sender_fin_event, NULL);
    }
}


PROCESS_THREAD(messenger_sender, ev, data)
{
	static struct etimer timer;
	int rc;

	PROCESS_BEGIN();

    rc = tcp_socket_register(&snd_socket, NULL,
                     send_buffer, sizeof(send_buffer),
                     rcv_buffer, sizeof(rcv_buffer),
                     sender_recv_bytes, sender_recv_event);

    if (rc < 0) {
    	LOG_DBG("Error - could not register TCP socket! Aborting process");
    	goto error;
    }
    LOG_DBG("Message sender starting on %d\n", MESSAGE_SERVER_PORT);
    while(1) {

        PROCESS_WAIT_EVENT_UNTIL(ev == sender_start_event);
        LOG_DBG("Message sender was awoken... connecting... to ");
        LOG_6ADDR(LOG_LEVEL_DBG, &message_addr);
        LOG_DBG_(" port %d ", MESSAGE_SERVER_PORT);
        LOG_DBG_(" sending %d bytes\n", send_length);

        etimer_reset(&timer);
        etimer_set(&timer, CLOCK_SECOND * 15);

        rc = tcp_socket_connect(&snd_socket, &message_addr, MESSAGE_SERVER_PORT);
        if (rc < 0) {
        	LOG_ERR("Error - socket could not connect: %d\n", rc);
        	send_started = 0;
            send_length = -1;
        }
        else
        {
			while(1) {
				PROCESS_WAIT_EVENT();

				if (ev == PROCESS_EVENT_TIMER) {
					LOG_INFO("Error - connect timed out\n");
					send_started = 0;
					send_length = -1;
					break;
				}
				if (ev == sender_fin_event) {
					LOG_INFO("Finished sending, sent %d bytes\n", send_length);
					break;
				}
			}
        }
        process_post(requestor, PROCESS_EVENT_MSG, NULL);


    }
error:
	LOG_DBG("Message sender process exiting\n");

    PROCESS_END();
}


void messenger_sender_init( )
{
	sender_start_event = process_alloc_event( );
	sender_fin_event = process_alloc_event( );

	process_start(&messenger_sender, NULL);
}
