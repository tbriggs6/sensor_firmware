/*
 * message-service.h
 *
 *  Created on: Dec 2, 2017
 *      Author: contiki
 */

#ifndef APPS_MESSAGE_SERVICE_MESSAGE_SERVICE_H_
#define APPS_MESSAGE_SERVICE_MESSAGE_SERVICE_H_

#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>


#define MESSAGE_CLIENT_PORT (0)	// listen to any port
#define MESSAGE_SERVER_PORT (5353) // only listen to this one

typedef void (*handler_t)(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length);

void message_init( void );
void messenger_send(uip_ipaddr_t *remote_addr, int remote_port, void *data, int length);
void messenger_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len, handler_t handler);
void messenger_remove_handler(handler_t handler);

#endif /* APPS_MESSAGE_SERVICE_MESSAGE_SERVICE_H_ */
