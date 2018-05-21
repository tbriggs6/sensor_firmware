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

#include "../message-service-tcp/message-service.h"

#include <contiki.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "sys/log.h"

#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_INFO

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


/* ****************************************************************
 * TCP Sender Process
 * **************************************************************** */
static process_event_t message_send_event = process_alloc_event();

typedef struct {
    const uip_ipaddr_t *remote_addr;
    const void *data;
    int port;
    size_t length;
} message_t;

static enum states { IDLE, CONNECTING, SENDING } sender_state = IDLE;
static struct psock message_sock;

PROCESS(message_sender,"Sends messages to collector.");
PROCESS_THREAD(message_sender, ev, data)
{
    static message_t *message;

    PROCESS_BEGIN( );

    while (1) {

        PROCESS_WAIT_EVENT();
        LOG_DBG("Event %d", ev);

        switch(sender_state) {
        case IDLE:
            sender_state = IDLE;
            if (ev == message_send_event) {
                message = (message_t *) data;
                if (message->length == 0) {
                    LOG_ERR("message_length is 0");
                }
                else if (message->data == NULL) {
                    LOG_ERR("message_data is a NULL pointer");
                }
                else {
                    tcp_connect(message->remote_addr);
                    sender_state = CONNECTING;
                }
            }
            else {
                LOG_ERR("ERR - Unknown event %d", ev);
            }
            break;

        case CONNECTING:
            sender_state = IDLE;
            if (ev == tcpip_event)
            {
                if (uip_aborted()) {
                    LOG_ERR("ERR - Connection aborted");
                }
                else if (uip_timedout()) {
                    LOG_ERR("ERR - Connection timedout");
                }
                else if (uip_closed()) {
                    LOG_ERR("ERR - Connection closed");
                }
                else if (uip_connected()) {
                    LOG_INFO("Connection established... sending."));
                    PSOCK_INIT(&message_sock);
                    PSOCK_SEND(&message_sock, message->data, message->length);
                    PSOCK_END();
                    sender_state = SENDING;
                }
                else {
                    LOG_ERR("ERR - Unknown TCP state\n");
                }
            }
            else {
                LOG_ERR("ERR - Cannot handle other events while waiting for TCP");
                sender_state = CONNECTING;
            }
            break;

        case SENDING:
            sender_state = IDLE;
            if (ev == tcpip_event) {
                if (uip_closed()) {
                    LOG_INFO("Connection closed.");
                }
                else if (uip_aborted()) {
                    LOG_ERR("Error - connection aborted");
                }
                else if (uip_timedout()) {
                    LOG_ERR("Error - connection timed out");
                }
            }
            else {
                LOG_ERR("Error - unexpected event %d\n", ev);
                sender_state = SENDING;
            }

            break;
        }
    }

    PROCESS_END();
}




/* **************************************************************** */


void messenger_send(const uip_ipaddr_t const *remote_addr, const void * const data, int length)
{
    if (sender_state != IDLE) {
        LOG_ERR("Error - aborted, send in progress.");
        return;
    }
    message_t message;
    message.remote_addr = remote_addr;
    message.port = MESSAGE_SERVER_PORT;
    message.data = data;
    message.length = length;

    process_post_synch(&message_sender, message_send_event, &message)


}

void commander_send(const uip_ipaddr_t const *remote_addr, const void * const data, int length)
{
        if (sender_state != IDLE) {
            LOG_ERR("Error - aborted, send in progress.");
            return;
        }
        message_t message;
        message.remote_addr = remote_addr;
        message.port = COMMAND_SERVER_PORT;
        message.data = data;
        message.length = length;

        process_post_synch(&message_sender, message_send_event, &message)

}



static void message_receiver(struct simple_msg_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
    LOG_INFO("[RECEIVER] Data received on port %d from port %d with length %d bytes\r\n",
         receiver_port, sender_port, datalen);

      uint32_t *header = (uint32_t *) data;
      int length = datalen;

      struct listener *curr;
      LOG_INFO("[RECEIVER] Dispatching a received message with %d bytes to %d listeners\r\n", length, list_length(handlers_list));
      for (curr = list_head(handlers_list); curr != NULL; curr = list_item_next(curr))
      {
          if (curr->handler == NULL) {
              LOG_DBG("[RECEIVER] Handler is null\r\n");
              continue;
          }
          if (curr->header != *header) {
              LOG_DBG("[RECEIVER] header %x != %x\r\n", curr->header, *header);
              continue;
          }

          if ((curr->min_len > 0) && (length < curr->min_len)) {
              LOG_DBG("[RECEIVER] length to small: %d < %d\r\n", length, curr->min_len);
              continue;
          }
          if ((curr->max_len > 0) && (length > curr->max_len)) {
              LOG_DBG("[RECEIVER] length is too large: %d > %d\r\n", length, curr->max_len);
              continue;
          }


          LOG_DBG("[RECEIVER] Sending to %p\r\n", curr->handler);
          curr->handler(sender_addr, sender_port, (char *) uip_appdata, length);
          break;
      }

     LOG_DBG("[RECEIVER] Done dispatching message\r\n");
     }



void message_init()
{
    process_start(&message_sender);
}
