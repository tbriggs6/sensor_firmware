/*
 * sender_udp.h
 *
 *  Created on: Nov 15, 2017
 *      Author: tbriggs
 */

#ifndef TESTNODE_SENDER_UDP_H_
#define TESTNODE_SENDER_UDP_H_

#include <contiki.h>

void start_server( );
void start_client( );
void become_root( );
void repair_root( );

void send_string(uip_ipaddr_t *receiver, const const char *str, int len);
void broadcast_string(const char *str, int len);

extern process_event_t send_fin_event;

#endif /* TESTNODE_SENDER_UDP_H_ */
