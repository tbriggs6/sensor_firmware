/*
 * sender.h
 *
 *  Created on: Jan 2, 2017
 *      Author: tbriggs
 */

#ifndef SENSORNODE_SENDER_H_
#define SENSORNODE_SENDER_H_

#include <contiki.h>

typedef struct {
	int battery_mon;
	int ctime;
	int adc[16];
} sender_data_t;



extern process_event_t sender_event;
extern process_event_t ack_event;
extern process_event_t sender_finish_event;

void sender_init( );
void sender_send_data(const sender_data_t *data);

#endif /* SENSORNODE_SENDER_H_ */
