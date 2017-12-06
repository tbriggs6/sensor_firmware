/*
 * confighandler.h
 *
 *  Created on: Dec 4, 2017
 *      Author: contiki
 */

#ifndef TESTSENSOR_COMMANDHANDLER_H_
#define TESTSENSOR_COMMANDHANDLER_H_

#include <contiki.h>
#include <net/ip/uip.h>

void command_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length);

#endif /* TESTSENSOR_COMMANDHANDLER_H_ */
