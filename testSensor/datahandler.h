/*
 * datahandler.h
 *
 *  Created on: Dec 4, 2017
 *      Author: contiki
 */

#ifndef TESTSENSOR_DATAHANDLER_H_
#define TESTSENSOR_DATAHANDLER_H_

void datahandler_init( );
void data_ack_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length);

#endif /* TESTSENSOR_DATAHANDLER_H_ */
