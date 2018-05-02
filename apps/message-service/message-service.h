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


#define MESSAGE_SERVER_PORT (5323)
#define COMMAND_SERVER_PORT (5525)

typedef void (*handler_t)(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length);

void message_init( void );
void messenger_send(const uip_ipaddr_t const *remote_addr, const void const *data, int length);
void commander_send(const uip_ipaddr_t const *remote_addr, const void const *data, int length);
void messenger_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len, handler_t handler);
void messenger_remove_handler(handler_t handler);

#endif /* APPS_MESSAGE_SERVICE_MESSAGE_SERVICE_H_ */
