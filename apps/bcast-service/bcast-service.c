#include "../bcast-service/bcast-service.h"

#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ipv6/multicast/uip-mcast6.h>
#include <net/ip/uip-debug.h>

#include <stdio.h>


 PROCESS(broadcast_process, "Broadcast process");

static struct uip_udp_conn *mcast_conn;
static uint16_t bcast_port = BCAST_PORT;

process_event_t bcast_event;

struct observer {
    struct observer *next;

    struct process *process;
};

#define MAX_OBSERVERS 16
MEMB(observers_memb, struct observer, MAX_OBSERVERS);
LIST(observers_list);


static uip_ds6_maddr_t * bcast_join_mcast_group(void)
{
    uip_ipaddr_t addr;
    uip_ds6_maddr_t *rv;

    /* First, set our v6 global */
    uip_ip6addr(&addr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&addr, &uip_lladdr);
    uip_ds6_addr_add(&addr, 0, ADDR_AUTOCONF);

    /*
     * IPHC will use stateless multicast compression for this destination
     * (M=1, DAC=0), with 32 inline bits (1E 89 AB CD)
     */
    uip_ip6addr(&addr, 0xFF1E,0,0,0,0,0,0x89,0xABCD);
    rv = uip_ds6_maddr_add(&addr);

    if (rv) {
        printf("Joined multicast group ");
        uip_debug_ipaddr_print(&uip_ds6_maddr_lookup(&addr)->ipaddr);
        printf("\n");
    }

    return rv;

}


static void bcast_receiver( )
 {

    static struct observer *curr;
    static bcast_t bcast;

    bcast.sender_addr = 0;
    bcast.sender_port = 0;
    bcast.receiver_addr = 0;
    bcast.receiver_port = 0;
    bcast.data = "foo";
    bcast.datalen = 4;


    for (curr = list_head(observers_list); curr != NULL; curr = list_item_next(curr))
    {
        process_post_synch(curr->process, bcast_event, &bcast);
    }
 }


 PROCESS_THREAD(broadcast_process, ev, data)
 {
   PROCESS_BEGIN();

   if (bcast_join_mcast_group() == NULL) {
       printf("Error joining multicast group\n");
       PROCESS_EXIT();
   }

   mcast_conn = udp_new(NULL, UIP_HTONS(0), NULL);
   udp_bind(mcast_conn, UIP_HTONS(BCAST_PORT));

   while(1) {
     PROCESS_WAIT_EVENT();
     if (ev == tcpip_event) {
         bcast_receiver( );
     }
   }

   PROCESS_END();
 }

void bcast_add_observer(struct process *observer)
{
    static struct observer *curr;

    if (list_length(observers_list) >= MAX_OBSERVERS)
        return;

    for (curr = list_head(observers_list); curr != NULL; curr = list_item_next(curr))
    {
       if (curr->process == observer)
           return;
    }

    struct observer *n = memb_alloc(&observers_memb);
    n->process = observer;
    list_add(observers_list, n);
}

void bcast_remove_observer(struct process *observer)
{
    static struct observer *curr;

    for (curr = list_head(observers_list); curr != NULL; curr = list_item_next(curr))
    {
       if (curr->process == observer)
           break;
    }

    if (curr == NULL) return;

    list_remove(observers_list, curr);
    memb_free(&observers_memb, curr);
}

void bcast_getaddr(uip_ipaddr_t *addr)
{
    uip_create_linklocal_allnodes_mcast(addr);
}


int bcast_init( uint16_t port)
{
    if (port != 0) bcast_port  = port;


//    static uip_ipaddr_t mcast_addr;
//    bcast_getaddr(&mcast_addr);
//

    bcast_event = process_alloc_event();
    process_start(&broadcast_process, NULL);
    return 1;
}
