/**
 * @file bcast-sink.c
 * @author Tom Briggs
 * @brief A broadcast sink service - will listen for and notify observers.
 *
 * A broadcast sink service.  This will listen for incoming UDP broadcast
 * messages and notify a list of observers. Each observer will be notified
 * by posting a broadcast_sink_event to the process.  The data for the
 * event will be received message.
 *
 */


#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ipv6/multicast/uip-mcast6.h>
#include <net/ip/uip-debug.h>

#include <stdio.h>

#include <bcast-service.h>

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif

#define UIP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

/**
 * @brief the broadcast_sink process that handles tcpip_events and notifies observers.
 */
PROCESS(broadcast_sink, "Broadcast Sink");

/**
 * @brief the IP address to listen to (expected to be a multi-cast address)
 */
static uip_ipaddr_t sink_addr;

/**
 * @brief the established UDP connection that will generate those events
 */
static struct uip_udp_conn *sink_conn;

/**
 * @brief the event that will be delivered to a process to indicate an event
 */
process_event_t bcast_event;

/**
 * @brief A structure to hold the observers of the sink
 *
 * Each entry contains the process that should be notified,
 * and in typical singly-linked list fashion, a pointer to the
 * next node in the list.  The list structure itself is managed
 * by the Contiki list classes.
 *
 * NOTE: the "struct observer *next" must be the first entry in
 * the struct.(to allow Contiki to work).
 */
struct observer {
    struct observer *next;
    struct process *process;
};

/**
 * @brief the maximum number of observers -
 *
 * This should be tailored
 * to the particular application's needs.  The more allocated
 * than is needed, the more memory is wasted.
 */
#define MAX_OBSERVERS 8

/**
 * @brief the memory block to allocate observer nodes
 */
MEMB(observers_memb, struct observer, MAX_OBSERVERS);

/**
 * @brief the linked list of observers
 */
LIST(observers_list);




/**
 * @brief delivers the received data to the observers
 *
 * When the sink receives a message, this function will be called
 * to notify the observers, one by one, that the data has been
 * received. The update takes place by iterating over each of the
 * processes in the notify list.
 *
 * TODO verify that there cannot be a race condition on the list by
 * receiving a packge while an observer is being added or removed
 */
static void bcast_receiver( const char *data, int length)
 {

    static struct observer *curr;
    static bcast_t bcast;

    memset(&bcast, 0, sizeof(bcast));

    //TODO fill in the proper data here.
    memcpy(&bcast.sender_addr,  &(UIP_IP_BUF->srcipaddr), sizeof(uip_ipaddr_t));
    bcast.sender_port = UIP_IP_BUF->srcport;

    memcpy(&bcast.receiver_addr, &(UIP_IP_BUF->destipaddr), sizeof(uip_ipaddr_t));
    bcast.receiver_port = UIP_IP_BUF->destport;

    if (length > MAX_PAYLOAD) {
        printf("Error - only first %d bytes will be sent", MAX_PAYLOAD);
        length = MAX_PAYLOAD;
    }
    memcpy(&bcast.data, data, length);
    bcast.datalen = length;


    printf("Notifying observers %d %s\n", length, data);
    for (curr = list_head(observers_list); curr != NULL; curr = list_item_next(curr))
    {
        process_post_synch(curr->process, bcast_event, &bcast);
    }
 }


 PROCESS_THREAD(broadcast_sink, ev, data)
 {
   PROCESS_BEGIN();

   // create the UDP connections to listen for incoming messages
   sink_conn = udp_new(NULL, UIP_HTONS(0), NULL);   // creates an unassigned connection
   udp_bind(sink_conn, UIP_HTONS(BCAST_PORT));     // binds to connection to local port (2000)

   printf("Listening for incoming multicast messages ");
   uip_debug_ipaddr_print(&sink_addr);
   printf(" port: %u/%u\n", UIP_HTONS(sink_conn->lport), UIP_HTONS(sink_conn->rport));

   while(1) {
     PROCESS_YIELD();
     if ((ev == tcpip_event) && (uip_newdata())) {
         bcast_receiver(uip_appdata, uip_len );
     }
   }

   PROCESS_END();
 }

 void bcast_sink_init( void  )
 {
	 printf("****** Starting broadcast sink (should not be root) ******* \n");

     bcast_event = process_alloc_event();

     process_start(&broadcast_sink, NULL);
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

