/**
 * @file bcast-service.c
 * @author Tom Briggs
 * @brief A broadcast service (source and sink)
 *
 * This is the top-level wrapper for a broad-cast service
 * that can listen (be a sink) for incoming broadcast requests
 * and can send (be a source) for outgoing broadcast messages.
 *
 */
#include "../bcast-service/bcast-service.h"

#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ipv6/multicast/uip-mcast6.h>
#include <net/ip/uip-debug.h>
#include <net/rpl/rpl.h>

#include <sys/process.h>
#include <stdio.h>

#define DEBUG 1

#undef PRINTF

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

/**
 * @brief The process that actually sends the broadcast
 * message.
 */
PROCESS(broadcast_source, "Broadcast source");

/** @breif The mutlicast connection used for sending out packets */
static struct uip_udp_conn * mcast_conn;

/**
 * @brief An internal event used to send the requested
 * broadcast message.
 */
static process_event_t bcast_send_event;

/**
 * @brief a package of data that will be sent to each of the
 * observer nodes.
 *
 * TODO determine if this is safe - the data will ultimately be a pointer
 * to the received data buffer (efficient) but could be overwritten.
 */
typedef struct {
	char *data; // the data that was received
	int length; // the number of bytes that were received
} bcast_send_t;

/**
 * @brief Actual procedure for sending the data to the multi-cast connection
 *
 * The broadcast payload is sent out as a UDP datagram.  This is expected to
 * only be called from the broadcast_source() process.  This must be called
 * only after the bcast_init() function has initialized the #mcast_conn variable.
 */
static void bcast_source(bcast_send_t *payload) {
	PRINTF("Send to: "); PRINT6ADDR(&mcast_conn->ripaddr);
	PRINTF(" Remote Port %u,", uip_ntohs(mcast_conn->rport));
	PRINTF(" %d bytes %s\n", payload->length, payload->data);

	/* uip_udp_packet_send - one of the first steps is to do a "memmove"
	 to copy the data into the global uip_buf[ ] buffer
	 we need to make sure that this is done sending before we
	 send the next packet

	 then, this calls a function "tcpip_output()" which is
	 a pointer toanother function.  I tracked it back to the
	 cooja version, and it starts off doing a packetbuf_copyfrom()
	 to copy the data from the uip_buf[] buffer into the local packet
	 memory.  It then calls NETSTACK_LLSEC.send(NULL, NULL)

	 */
	uip_udp_packet_send(mcast_conn, payload->data, payload->length);

}

/**
 * @brief Prepares the multicast connection
 *
 * The function will setup the IP6 multicast address and create a UDP connection.
 */
static void bcast_prepare() {
	uip_ipaddr_t ipaddr;

	/*
	 * IPHC will use stateless multicast compression for this destination
	 * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
	 */
	uip_ip6addr(&ipaddr, 0xFF1E, 0, 0, 0, 0, 0, 0x89, 0xABCD);
	mcast_conn = udp_new(&ipaddr, UIP_HTONS(BCAST_PORT), NULL);
}

PROCESS_THREAD(broadcast_source, ev, data) {
	PROCESS_BEGIN()	;

	bcast_prepare();

	while (1) {
		PROCESS_YIELD(	);

		if (ev == bcast_send_event) {
			PRINTF("Handling posted bcast event %p data\n", data);
			PRINTF("len: %d string: %s\n", ((bcast_send_t * )data)->length,
					((bcast_send_t * )data)->data);
			bcast_source((bcast_send_t *) data);
		}
	}

PROCESS_END();
}

void bcast_source_init() {
PRINTF("****** Starting broadcast source (should be root) ******* \n");
bcast_send_event = process_alloc_event();
process_start(&broadcast_source, NULL);
}

uip_ipaddr_t *bcast_address() {
return &mcast_conn->ripaddr;
}

void bcast_send(char *data, int len) {
// this needs to be static b/c it will be used in a different
//protothread context

if (len > MAX_PAYLOAD) {
	printf("%s:%d Error - payload is larger than %d bytes\n", __FILE__,
			__LINE__, MAX_PAYLOAD);
	return;
}

static bcast_send_t send_data;
send_data.data = data;
send_data.length = len;

process_post(&broadcast_source, bcast_send_event, &send_data);
}
