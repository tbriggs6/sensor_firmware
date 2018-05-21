/**
 * @file message-service.c
 * @author Tom Briggs
 * @brief The message service manager
 *
 * This service manages the exchanging messages.  Other
 * services register themselves by submitting a handler
 * type and length, and this process will dispatch
 * matching datagrams.
 *
 */

#include "../messenger/message-service.h"

#include <contiki.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <contiki-net.h>
#include <net/ipv6/tcpip.h>
#include "sys/log.h"

#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

#define UIP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

/**
 * @brief Listener services
 */
struct listener {
	struct listener *next;		// needed for list
	uint32_t header;			// header for message
	uint32_t min_len;			// minimum acceptable length
	uint32_t max_len;			// maximum acceptable length
	handler_t handler;
};

/**
 * @brief maximum number of observers in the list
 */
#define MAX_HANDLERS 8

/**
 * Memory pool for handlers
 */
MEMB(handlers_memb, struct listener, MAX_HANDLERS);

/**
 * @brief the list of handlers
 */
LIST(handlers_list);


void messenger_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len, handler_t handler)
{
	if (list_length(handlers_list) >= MAX_HANDLERS)
		return;

	struct listener *n = memb_alloc(&handlers_memb);
	n->header = header;
	n->min_len = min_len;
	n->max_len = max_len;
	n->handler = handler;

	list_add(handlers_list, n);
}


void messenger_remove_handler(handler_t handler)
{
	struct listener *curr;

	if (list_length(handlers_list) >= MAX_HANDLERS)
		return;

	for (curr = list_head(handlers_list); curr != NULL; curr = list_item_next(curr))
	{
		if (curr->handler == handler)
			break;
	}

	if (curr == NULL) return;

	list_remove(handlers_list, curr);
	memb_free(&handlers_memb, curr);
}


#define MAX_SEND_BUFFER 128
#define MAX_RECV_BUFFER 32
static uint8_t send_buffer[MAX_SEND_BUFFER];
static uint8_t rcv_buffer[MAX_RECV_BUFFER];
static int send_length, recv_length;
static volatile int send_started = 0;
static struct tcp_socket snd_socket;
static uip_ipaddr_t message_addr;
static struct process *requestor;

static process_event_t sender_start_event;
static process_event_t sender_fin_event;

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

void messenger_send(const uip_ipaddr_t const *remote_addr, const void * const data, int length)
{
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

    process_post_synch(&messenger_sender, sender_start_event, NULL);
}



/* ****************************************************************
 *
 * TCP Sending agent
 *
 * **************************************************************** */
static int sender_recv_bytes(struct tcp_socket *s, void *ptr,
                             const uint8_t *inputptr, int inputdatalen)
{

    recv_length = inputdatalen;
    tcp_socket_close(s);

    return 0;
}

static void sender_recv_event(struct tcp_socket *s, void *ptr, tcp_socket_event_t ev)
{
    switch(ev) {
    case TCP_SOCKET_CONNECTED:
        LOG_DBG("Socket connected event");
        tcp_socket_send(s, send_buffer, send_length);
        break;

    case TCP_SOCKET_DATA_SENT:
          LOG_DBG("Data sent");
          break;

    case TCP_SOCKET_CLOSED:
        LOG_DBG("Socket closed event");
        send_started = 0;
        break;

    case TCP_SOCKET_TIMEDOUT:
        LOG_DBG("Socket timed out");
        send_started = 0;
        send_length = -1;
        break;

    case TCP_SOCKET_ABORTED:
        LOG_DBG("Socket aborted");
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
    PROCESS_BEGIN();

    tcp_socket_register(&snd_socket, NULL,
                     send_buffer, sizeof(send_buffer),
                     rcv_buffer, sizeof(rcv_buffer),
                     sender_recv_bytes, sender_recv_event);

    while(1) {

        PROCESS_WAIT_EVENT_UNTIL(ev == sender_start_event);
        LOG_DBG("Message receiver starting on %d\n", COMMAND_SERVER_PORT);

        tcp_connect(&message_addr, MESSAGE_SERVER_PORT, NULL);

        PROCESS_WAIT_EVENT_UNTIL(ev == sender_fin_event);
        process_post(requestor, PROCESS_EVENT_MSG, NULL);

    }
    PROCESS_END();
}


static struct tcp_socket rcv_socket;

#define INPUTBUFSIZE 128
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 128
static uint8_t outputbuf[OUTPUTBUFSIZE];



static int message_recv_bytes(struct tcp_socket *s, void *ptr,
                             const uint8_t *inputptr, int inputdatalen)
{
    LOG_DBG("Rcvd %d bytes", inputdatalen);

    uint32_t *header = (uint32_t *) inputptr;
    struct listener *curr;
    for (curr = list_head(handlers_list); curr != NULL; curr = list_item_next(curr))
    {
        if (inputdatalen > curr->max_len) continue;
        if (inputdatalen < curr->min_len) continue;

        if (inputptr == NULL) continue;
        if (*header != curr->header) continue;

        LOG_DBG("Matched handler %p (%d <= %d) (%d >= %d) (%x == %x)",
                curr->handler,
                inputdatalen, curr->max_len,
                inputdatalen, curr->min_len,
                *header, curr->header);

        int outputlen = sizeof(outputbuf);
        int rc = curr->handler(inputptr, inputdatalen, (uint8_t *) &outputbuf, &outputlen);

        LOG_DBG("Handler reported RC=%d and %d bytes\n", rc, outputlen);

        if ((rc == 1) && (outputlen > 0)) {
            LOG_DBG("Sending response to remote\n");
            tcp_socket_send(s, (uint8_t *) &outputbuf, outputlen);
            break;
        }
    }

    if (curr == NULL) {
        LOG_DBG("No handlers for the message %x %d\n",*header,inputdatalen);
    }

    tcp_socket_close(s);

    // consume all bytes
    return 0;
}

static void message_recv_event(struct tcp_socket *s, void *ptr, tcp_socket_event_t ev)
{
    switch(ev) {
    case TCP_SOCKET_CONNECTED: LOG_DBG("Socket connected event"); break;
    case TCP_SOCKET_CLOSED: LOG_DBG("Socket closed event"); break;
    case TCP_SOCKET_TIMEDOUT: LOG_DBG("Socket timed out"); break;
    case TCP_SOCKET_ABORTED: LOG_DBG("Socket aborted"); break;
    case TCP_SOCKET_DATA_SENT: LOG_DBG("Data sent"); break;
    }
}


PROCESS(messenger_receiver, "Messenger Receiver");
PROCESS_THREAD(messenger_receiver, ev, data)
{
    PROCESS_BEGIN();

    LOG_DBG("Message receiver starting on %d\n", COMMAND_SERVER_PORT);
    tcp_socket_register(&rcv_socket, NULL,
                  inputbuf, sizeof(inputbuf),
                  outputbuf, sizeof(outputbuf),
                  message_recv_bytes, message_recv_event);
     tcp_socket_listen(&rcv_socket, COMMAND_SERVER_PORT);

     while(1) {
         PROCESS_PAUSE();
     }


    PROCESS_END();
}


void message_init()
{
    sender_start_event = process_alloc_event( );
    sender_fin_event = process_alloc_event( );

    process_start(&messenger_sender, NULL);
    process_start(&messenger_receiver, NULL);
}
