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

#include <contiki.h>
#include <contiki-lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include "simple-udp.h"

#include "message-service.h"

#undef DEBUG
#undef PRINTF

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

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



static struct simple_udp_connection udp_conn;

void messenger_send(const uip_ipaddr_t const *remote_addr, const void * const data, int length)
{
    simple_udp_sendto(&udp_conn, data, length, remote_addr);
}



static void message_receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);

      uint32_t *header = (uint32_t *) data;
      int length = datalen;

      struct listener *curr;
      printf("Dispatching a received message with %d bytes to %d listeners\n", length, list_length(handlers_list));
      for (curr = list_head(handlers_list); curr != NULL; curr = list_item_next(curr))
      {
          if (curr->handler == NULL) {
              printf("Handler is null\n");
              continue;
          }
          if (curr->header != *header) {
              printf("header %x != %x\n", curr->header, *header);
              continue;
          }

          if ((curr->min_len > 0) && (length < curr->min_len)) {
              printf("length to small: %d < %d\n", length, curr->min_len);
              continue;
          }
          if ((curr->max_len > 0) && (length > curr->max_len)) {
              printf("length is too large: %d > %d\n", length, curr->max_len);
              continue;
          }


          printf("Sending to %p\n", curr->handler);
          curr->handler(sender_addr, sender_port, (char *) uip_appdata, length);
          break;
      }

      printf("Done dispatching message\n");
}



void message_init( )
{
    simple_udp_register( &udp_conn, MESSAGE_SERVER_PORT, NULL, MESSAGE_SERVER_PORT, message_receiver);
}
