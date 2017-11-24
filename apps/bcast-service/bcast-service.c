#include "../bcast-service/bcast-service.h"

#include <contiki.h>
#include <contiki-lib.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>
#include <net/ipv6/uip-ds6.h>
#include <stdio.h>


 PROCESS(broadcast_process, "Broadcast process");

static struct simple_udp_connection bcast_connection;
static uint16_t bcast_port = BCAST_PORT;

process_event_t bcast_event;

struct observer {
    struct observer *next;

    struct process *process;
};

#define MAX_OBSERVERS 16
MEMB(observers_memb, struct observer, MAX_OBSERVERS);
LIST(observers_list);


static void bcast_receiver(
         struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
 {

    static struct observer *curr;
    static bcast_t bcast;

    bcast.c = c;
    bcast.sender_addr = sender_addr;
    bcast.sender_port = sender_port;
    bcast.receiver_addr = receiver_addr;
    bcast.receiver_port = receiver_port;
    bcast.data = data;
    bcast.datalen = datalen;


    for (curr = list_head(observers_list); curr != NULL; curr = list_item_next(curr))
    {
        process_post_synch(curr->process, bcast_event, &bcast);
    }
 }


 PROCESS_THREAD(broadcast_process, ev, data)
 {
   PROCESS_BEGIN();

   simple_udp_register(&bcast_connection, bcast_port,
                        NULL, bcast_port,
                        bcast_receiver);

   while(1) {
     PROCESS_WAIT_EVENT();
     printf("Got event number %d\n", ev);
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


    static uip_ipaddr_t mcast_addr;
    bcast_getaddr(&mcast_addr);

    printf("Broadcast address: ");
    uip_debug_ipaddr_print(&mcast_addr);
    printf("\n");

    bcast_event = process_alloc_event();
    process_start(&broadcast_process, NULL);
    return 1;
}
