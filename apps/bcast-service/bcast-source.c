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
static void bcast_source(bcast_send_t *payload)
{
    PRINTF("Send to: ");
    PRINT6ADDR(&mcast_conn->ripaddr);
    PRINTF(" Remote Port %u,", uip_ntohs(mcast_conn->rport));
    PRINTF(" %d bytes %s\n", payload->length, payload->data);

    uip_udp_packet_send(mcast_conn, payload->data, payload->length);

}

/**
 * @brief Prepares the multicast connection
 *
 * The function will setup the IP6 multicast address and create a UDP connection.
 */
static void bcast_prepare( )
{
   uip_ipaddr_t ipaddr;

  /*
   * IPHC will use stateless multicast compression for this destination
   * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
   */
  uip_ip6addr(&ipaddr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
  mcast_conn = udp_new(&ipaddr, UIP_HTONS(BCAST_PORT), NULL);
}

/**
 * @brief Prints all known addresses
 */
static void bcast_print_addresses( )
{
     int i;
     uint8_t state;

     printf("Our IPv6 addresses:\n");
     for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
       state = uip_ds6_if.addr_list[i].state;
       if(uip_ds6_if.addr_list[i].isused &&
               (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
         printf("  ");
         uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
         printf("\n");
       }
     }
}

/**
 * @brief Update the state of the address lists from TENTATIVE to PREFFERED
 */
static void bcast_update_address_states( )
{
   int i;
   uint8_t state;

    for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
      state = uip_ds6_if.addr_list[i].state;
      if(uip_ds6_if.addr_list[i].isused &&
              (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {

          if(state == ADDR_TENTATIVE) {
          uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
        }
      }
    }
}


/**
 * @brief Sets the address and RPL routing information for the broadcast
 *
 * The default address is set for one of the default multi-cast addresses.
 */
static void bcast_set_addresses( )
{
     rpl_dag_t *dag;
     uip_ipaddr_t ipaddr;

     // loads the default prefix (upper 64-bit bits) with the default prefix
     uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);

     // sets the lower 64-bit of the address to the MAC address
     uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);

     // add the address to the list of addresses used by this device
     uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

     // update the broadcast states from tentative to preferred
     bcast_update_address_states( );

     /* Become root of a new DODAG with ID our global v6 address */
     dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
     if(dag != NULL) {
         rpl_set_prefix(dag, &ipaddr, 64);
         PRINTF("Created a new RPL dag with ID: ");
         PRINT6ADDR(&dag->dag_id);
         PRINTF("\n");
     }

}


 PROCESS_THREAD(broadcast_source, ev, data)
 {
   PROCESS_BEGIN();


     PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);

     NETSTACK_MAC.off(1);

     bcast_set_addresses();

     bcast_prepare();


     while(1) {
         PROCESS_YIELD();
         if (ev == bcast_send_event) {
             PRINTF("Handling posted bcast event %p data\n");
             PRINTF("len: %d string: %s\n", ((bcast_send_t *)data)->length,
                    ((bcast_send_t *)data)->data);
             bcast_source((bcast_send_t *) data);
         }
     }

   PROCESS_END();
 }

void bcast_source_init( )
{
     bcast_send_event = process_alloc_event();
     process_start(&broadcast_source, NULL);
}


uip_ipaddr_t *bcast_address( )
{
    return &mcast_conn->ripaddr;
}

void bcast_send(char *data, int len)
{
    // this needs to be static b/c it will be used in a different
    //protothread context
    static bcast_send_t send_data;
    send_data.data = data;
    send_data.length = len;

    process_post(&broadcast_source, bcast_send_event, &send_data);
}
