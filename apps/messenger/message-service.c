/**
 * @file message-service.c
 * @author Tom Briggs
 * @brief The message service manager
 *
 * The message service comprises two functions:
 * * A server socket that listens for incoming connections
 * * A list of call-back handlers that will be invoked when
 *   a matching message is received.
 *
 * At initialization (through a call to message_init()), the
 * service creates a TCP socket and listens for incoming
 * connections on port (COMMAND_SERVER_PORT in message-service.h).
 *
 * Applications register call-backs through calls to messenger_add_handler()
 * and messenger_remove_handler().  When a message is received, the service
 * will iterate through the list of registered handlers and compare the
 * matching criteria.  The first handler to match the criteria will be
 * invoked.  At this time, only the first matching handler will be called.
 *
 * The matching criteria are:
 * * Pre-amble of the message (the first 32-bits must match)
 * * minimum message length
 * * maximum message length
 *
 * As an example, the "echo" service registers itself with this module:
 *      messenger_add_handler(0x6f686365,   4,  99, echo_handler);
 * Where the pre-able is the ASCII characters "echo", and the message
 * must be between 4 and 99 bytes (inclusively).  When a TCP datagram
 * matches this, the function echo_handler() will be called with arguments
 * containing a pointer to the datagram and the number of received bytes.
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

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif



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


static struct tcp_socket rcv_socket;

#define INPUTBUFSIZE 128
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 128
static uint8_t outputbuf[OUTPUTBUFSIZE];



static int message_recv_bytes(struct tcp_socket *s, void *ptr,
                             const uint8_t *inputptr, int inputdatalen)
{
    LOG_DBG("Rcvd %d bytes\n", inputdatalen);

    uint32_t *header = (uint32_t *) inputptr;
    struct listener *curr;
    for (curr = list_head(handlers_list); curr != NULL; curr = list_item_next(curr))
    {
        if (inputdatalen > curr->max_len) continue;
        if (inputdatalen < curr->min_len) continue;

        if (inputptr == NULL) continue;
        if (*header != curr->header) continue;

        LOG_DBG("Matched handler %p (%d <= %d) (%d >= %d) (%x == %x)\n",
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
    case TCP_SOCKET_CONNECTED: LOG_DBG("Socket connected event\n"); break;
    case TCP_SOCKET_CLOSED: LOG_DBG("Socket closed event\n"); break;
    case TCP_SOCKET_TIMEDOUT: LOG_DBG("Socket timed out\n"); break;
    case TCP_SOCKET_ABORTED: LOG_DBG("Socket aborted\n"); break;
    case TCP_SOCKET_DATA_SENT: LOG_DBG("Data sent\n"); break;
    }
}


PROCESS(messenger_receiver, "Messenger Receiver");
PROCESS_THREAD(messenger_receiver, ev, data)
{
    PROCESS_BEGIN();

    LOG_DBG("Command receiver starting on %d\n", COMMAND_SERVER_PORT);
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

void messenger_sender_init( );

void messenger_init()
{
	messenger_sender_init( );
	process_start(&messenger_receiver, NULL);

}
