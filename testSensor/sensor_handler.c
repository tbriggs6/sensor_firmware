/*
 *  sensor_handler.c
 *
 *  Created on: Mar 4, 2018
 *      Author: Michael Foreman
 */


#include "message.h"
#include "config.h"
#include "neighbors.h"
#include "message-service.h"
#include "sensor_handler.h"
#include <contiki.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scif_framework.h"
#include "scif_osal_contiki.h"
#include "scif_scs.h"

#ifndef BV
#define BV(n)           (1 << (n))
#endif

PROCESS(sensor_timer, "Sensor Interval Timer");
PROCESS(test_data2, "Data Handler");

volatile static int data_seq_acked = -1;

void data_ack_handler2(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{

    data_ack_t *ack = (data_ack_t *) data;
    if (ack->header != DATA_ACK_HEADER) {
        printf("unexpected message - not a data ack\r\n");
        return;
    }

    printf("Data ack seq: %d\r\n", ack->ack_seq);
    data_seq_acked = ack->ack_seq;
    process_post(&test_data2, PROCESS_EVENT_MSG, data);
}


PROCESS_THREAD(sensor_timer, ev, data)
{
	PROCESS_BEGIN();

	// create timer for sensor_interval seconds to attempt broadcast
	static struct etimer et;

	while(1){
		printf("[SENSOR TIMER PROC] Waiting for %d  /  %d\r\n", config_get_sensor_interval(),
				config_get_sensor_interval() / CLOCK_SECOND);
		etimer_set(&et, config_get_sensor_interval());

		PROCESS_WAIT_EVENT( );

		if (ev == PROCESS_EVENT_TIMER){
			printf("[SENSOR TIMER PROC] Sensor Timer expired. Restarting timer...\r\n");
			printf("[SENSOR TIMER PROC] Invoking SCS code...\r\n");
			//scifSwTriggerExecutionCodeNbl(BV(SCIF_SCS_ANALOG_SENSOR_TASK_ID));
			process_poll(&test_data2);
		}
		else if (ev == PROCESS_EVENT_MSG){
			printf("[SENSOR TIMER PROC] Sensor interval changed. Restarting timer with new interval...\r\n");
		}
		else{
			printf("[SENSOR TIMER PROC] Unknown event generated...\r\n");
		}
	}


	PROCESS_END();
}


PROCESS_THREAD(test_data2, ev, data)
{
    PROCESS_BEGIN( );

    // create timer for sensor_interval seconds to attempt broadcast
    static struct etimer et;

    // create data_t struct for fake data
    // NOTE: Obliterates "data" input variable
    static data_t data;

    // this is compared to the global "data_seq_acked for loop control.
    // when the message is acknowledged, this process will stop trying to send
    // its data to another sensor(airborne or water)?.
    static int sequence = 0;

    // holds IPv6 address for the sensor
    static uip_ipaddr_t server;

    // controls how many times the loop attempts to send the data to another sensor
    static int count = 0;

    // Constructs an IPv6 address... part of Contiki source code
    uip_ip6addr(&server, 0xFD00, 0, 0, 0, 0, 0, 0, 1);
    messenger_add_handler(DATA_ACK_HEADER, sizeof(data_ack_t), sizeof(data_ack_t), data_ack_handler2);

    printf("[DATA SENDER PROC] Initializing...\r\n");
    while(1)
    {
//    	// setting and waiting for the sensor_interval timer to go off
//        printf("Waiting for %d  /  %d\r\n", config_get_sensor_interval(), config_get_sensor_interval() / CLOCK_SECOND);
//        etimer_set(&et, config_get_sensor_interval());
//        PROCESS_WAIT_UNTIL(etimer_expired(&et));

    	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        // create fake data
        data.header = DATA_HEADER;
        data.sequence = sequence;
        data.battery = 55;
        data.temperature = 33;
        data.adc[0] = 0x1010;
        data.adc[1] = 0x2031;
        data.adc[2] = 0x3043;
        data.adc[3] = 0x4343;

        count = 0;
        // while the current message hasn't tried to send count times
        // and less messages have been acknowledged than have been sent
        while ((count < 10) && (data_seq_acked < sequence)) {

        	// if the message hasn't tried to send yet or the eTimer has expired
            if ((count == 0) || (etimer_expired(&et)))
            {
            	// send the current message and set up a timer to control
            	// how long it will wait until trying to resend the data
                printf("Sending data - attempt %d \r\n", count);
                messenger_send(&server, &data, sizeof(data_t));
                etimer_set(&et, 2 * CLOCK_SECOND);
            }

            // wait until the process receives an event
            PROCESS_WAIT_EVENT( );

            // if the event is that the data has been acknowledged (posted by data_ack_handler())
            // and the same number of messages have been acknowledged as have been sent
            if ((ev == PROCESS_EVENT_MSG) && (data_seq_acked == sequence)) {
                printf("OK - data is ACKd\r\n");
                break;
            }

            // if the timer for waiting for an acknowledgment has expired
            else if (etimer_expired(&et)) {
                printf("Timeout waiting for ACK\r\n");
                count++;
            }

            // an event was posted to this process that was not the expiration
            // of the timer or an acknowledgment
            else {
                printf("Seq: %d  Last ack: %d\r\n",sequence , data_seq_acked);
                printf("Unexpected wake-up (%d), resending\r\n", ev);
            }


        }

        printf("\r\n\n\nFinished sending sequence %d\r\n\n\n", sequence);
        sequence++;



    }
    PROCESS_END();
}


void datahandler_init2( )
{
    process_start(&test_data2, NULL);
    process_start(&sensor_timer, NULL);
}
