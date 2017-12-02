#ifndef BCAST_SERVICE_H
#define BCAST_SERVICE_H

#include <errno.h>
#include <stdint.h>

#include <contiki.h>
#include <contiki-lib.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>
#include <net/ipv6/uip-ds6.h>

#include <sys/process.h>

/*! \def BCAST_PORT
    \brief The default port used for listening to broadcasts

    The default broadcast port, which can be overridden when initiating the broadcast service.
*/
#define BCAST_PORT (2000)


//! Initialize the broadcast service
/*!
  \param root_node indicate whether this is a root node or not
  \return 0 for an error, 1 on success
*/
void bcast_init( int root_node );



void bcast_add_observer(struct process *observer);
void bcast_remove_observer(struct process *observer);
void bcast_send(char *data, int len);
uip_ipaddr_t *bcast_address( );

#define MAX_PAYLOAD (32)

/**
 * @brief The broadcast details
 *
 * This is used to pass the broadcast details around the program
 * Note that it makes a copy of the received data - and restricts
 * the size to MAX_PAYLOAD
 */
typedef struct {
    struct simple_udp_connection *c;
    uip_ipaddr_t sender_addr;
    uint16_t sender_port;
    uip_ipaddr_t receiver_addr;
    uint16_t receiver_port;
    uint16_t datalen;
    char data[MAX_PAYLOAD];
    char zero;
} bcast_t;

//! Event is posted when a broadcast mesg is received
extern process_event_t bcast_event;

#endif
