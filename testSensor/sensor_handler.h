/*
 * datahandler.h
 *
 *  Created on: Dec 4, 2017
 *      Author: contiki
 */

#ifndef TESTSENSOR_SENSOR_HANDLER_H_
#define TESTSENSOR_SENSOR_HANDLER_H_

void datahandler_init2( );
void data_ack_handler2(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length);

//PROCESS_NAME(test_data2);

#endif /* TESTSENSOR_DATAHANDLER_H_ */
