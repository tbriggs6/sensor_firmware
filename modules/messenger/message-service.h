/*
 * message-service.h
 *
 *  Created on: Dec 2, 2017
 *      Author: contiki
 */

#ifndef APPS_MESSAGE_SERVICE_UDP_MESSAGE_SERVICE_H_
#define APPS_MESSAGE_SERVICE_UDP_MESSAGE_SERVICE_H_

#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>

#include <stdint.h>

// the server port to *listen* for incoming conns
#define COMMAND_SERVER_PORT (5323)

// the remote server port to *connect* to sent messages
#define MESSAGE_SERVER_PORT (5555)

extern process_event_t sender_start_event;
extern process_event_t sender_fin_event;

/*
 * The call-back template for the messenger service
 * * inputdata - the pointer to the received data
 * * inputlength - the length of the received buffer
 * * outputdata - the output data to be sent back to the remote
 * * maxoutputlenth - initially the maximum size of the buffer, the
 *       handler must set this to the number of bytes to actually return.
 */
typedef int (*handler_t)(const uint8_t *inputdata, int inputlength, uint8_t *outputdata, int *maxoutputlen);

// initialize the messenger framework
void messenger_init( void );

// send a message to the given address
void messenger_send (const uip_ipaddr_t *remote_addr,  uint16_t sequence,  const void *data, int length);

// get result of the last send (including any data received from the remote
void messenger_get_last_result(int *sendlen, int *recvlen, int maxlen, void *dest);

// was the last result a valid ACK from the remote?
int messenger_last_result_okack( );

// add a callback handler to the list
void messenger_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len, handler_t handler);

// remove a callback handler from the list
void messenger_remove_handler(handler_t handler);

int messenger_recvd_rssi( );

#endif /* APPS_MESSAGE_SERVICE_UDP_MESSAGE_SERVICE_H_ */
