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
#include <contiki.h>
#include <contiki-lib.h>

#include <bcast-service.h>
#include <bcast-sink.h>
#include <bcast-source.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include <net/ipv6/multicast/uip-mcast6.h>
#include <net/ipv6/multicast/uip-mcast6-engines.h>

#undef DEBUG

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
 * @brief joins the multicast group
 *
 * This code is largely taken from the contiki/examples/ipv6/multicast/sink.c
 */
static uip_ds6_maddr_t * bcast_join_mcast_group(void)
{
    uip_ipaddr_t addr;
    uip_ds6_maddr_t *rv;

    /*
     * IPHC will use stateless multicast compression for this destination
     * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
     */
    uip_ip6addr(&addr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
    rv = uip_ds6_maddr_add(&addr);

    if (rv != NULL) {
        PRINTF("Joined multicast group ");
        PRINT6ADDR(&uip_ds6_maddr_lookup(&addr)->ipaddr);
        PRINTF("\n");
    }
    else {
    	printf("Error - did not join multicast group\n");
    }

    return rv;
}


/**
 * @brief initialize the broadcast layer
 *
 * Initialize either the source or the sink.  Only the root (border router)
 * should call the source initialization, while all of the other nodes should
 * be configured as a sink.
 *
 * The border node should also be configured as a sink so that it can listen
 * to remote requests.
 */
void bcast_init( int root_node )
{

    PRINTF("Multicast Engine: '%s'\n", UIP_MCAST6.name);

    if (root_node)
    	bcast_source_init( );
    else
    	bcast_sink_init();

    bcast_join_mcast_group( );
}


