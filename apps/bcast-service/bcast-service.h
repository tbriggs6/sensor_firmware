#ifndef BCAST_SERVICE_H
#define BCAST_SERVICE_H

#include <errno.h>
#include <stdint.h>

#include <contiki.h>
#include <contiki-lib.h>
#include <net/ip/uip.h>
#include <net/ip/simple-udp.h>

/*! \def BCAST_PORT
    \brief The default port used for listening to broadcasts

    The default broadcast port, which can be overridden when initiating the broadcast service.
*/
#define BCAST_PORT (2000)


//! Initialize the broadcast service
/*!
  \param port the port number for the service (0 will use BCAST_PORT).
  \return 0 for an error, 1 on success
*/
int bcast_init( uint16_t port);
void bcast_add_observer(struct process *observer);
void bcast_remove_observer(struct process *observer);

extern  process_event_t bcast_event;



typedef struct {
    struct simple_udp_connection *c;
    const uip_ipaddr_t *sender_addr;
    uint16_t sender_port;
    const uip_ipaddr_t *receiver_addr;
    uint16_t receiver_port;
    const uint8_t *data;
    uint16_t datalen;

} bcast_t;


#endif
