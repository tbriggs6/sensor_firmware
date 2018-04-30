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

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(sensor_timer, "Sensor Interval Timer");
PROCESS(test_data2, "Data Handler");

volatile static int data_seq_acked = -1;
volatile static SCIF_SCS_READ_DATA_OUTPUT_T scData;

void data_ack_handler2(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{

    data_ack_t *ack = (data_ack_t *) data;
    if (ack->header != DATA_ACK_HEADER) {
        printf("unexpected message - not a data ack\r\n");
        return;
    }

    PRINTF("[DATA ACK HANDLER] Data ack seq: %u\r\n", ack->ack_seq);
    data_seq_acked = ack->ack_seq;
    process_post(&test_data2, PROCESS_EVENT_MSG, data);
}


PROCESS_THREAD(sensor_timer, ev, data)
{
	PROCESS_BEGIN();

	// create timer for sensor_interval seconds to attempt broadcast
	static struct etimer et;

	while(1){
		if ((ev == PROCESS_EVENT_TIMER) || (ev == PROCESS_EVENT_TIMER)){
			if(osalIsReady()){
				scifSwTriggerExecutionCodeNbl(BV(SCIF_SCS_READ_DATA_TASK_ID));
				unsetOsalReady();
			}
		}

		PRINTF("[SENSOR TIMER PROCESS] Waiting for %d  /  %d\r\n", config_get_sensor_interval(),
				config_get_sensor_interval() / CLOCK_SECOND);
		etimer_set(&et, config_get_sensor_interval());

		PROCESS_WAIT_EVENT( );

		if (ev == PROCESS_EVENT_TIMER){
			PRINTF("[SENSOR TIMER PROCESS] Sensor Timer expired. Restarting timer...\r\n");
			PRINTF("[SENSOR TIMER PROCESS] Invoking SCS code...\r\n");
		}
		else if (ev == PROCESS_EVENT_POLL){
			PRINTF("[SENSOR TIMER PROCESS] Sensor interval changed. Restarting timer with new interval...\r\n");
		}
		else{
			PRINTF("[SENSOR TIMER PROCESS] Unknown event generated. Resetting timer...\r\n");
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

    // keeps track of how many times new data has been received since attempting to send data
    static int newDatRcvd = 0;

    // Constructs an IPv6 address... part of Contiki source code
    uip_ip6addr(&server, 0xFD00, 0, 0, 0, 0, 0, 0, 1);
    messenger_add_handler(DATA_ACK_HEADER, sizeof(data_ack_t), sizeof(data_ack_t), data_ack_handler2);

    PRINTF("[DATA SENDER PROCESS] Initializing...\r\n");
    while(1)
    {
    	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    	newDatRcvd = 0;

        data.header = DATA_HEADER;
        data.sequence = sequence;
        data.battery = scData.BatterySensor;
        data.temperature = scData.TemperatureSensor;
        data.adc[0] = scData.AmbLight;
        data.adc[1] = scData.Conductivity;
        data.adc[2] = scData.HallSensor;
        data.I2CError = scData.I2CError;
        data.colors[0] = scData.colorClear;
        data.colors[1] = scData.colorRed;
        data.colors[2] = scData.colorGreen;
        data.colors[3] = scData.colorBlue;

        PRINTF("\r\n\n\n[DATA SENDER PROCESS] Data to be broadcast:\r\n");
        PRINTF("[DATA SENDER PROCESS] Header: %X\r\n", data.header);
        PRINTF("[DATA SENDER PROCESS] Sequence: %u\r\n", data.sequence);
        PRINTF("[DATA SENDER PROCESS] Ambient Light: %u\r\n", data.adc[0]);
        PRINTF("[DATA SENDER PROCESS] Battery Sensor: %u\r\n", data.battery);
        PRINTF("[DATA SENDER PROCESS] Conductivity: %u\r\n", data.adc[1]);
        PRINTF("[DATA SENDER PROCESS] Hall Sensor: %u\r\n", data.adc[2]);
        PRINTF("[DATA SENDER PROCESS] I2CError: %u\r\n", data.I2CError);
        PRINTF("[DATA SENDER PROCESS] Temperature Sensor: %u\r\n", data.temperature);
        PRINTF("[DATA SENDER PROCESS] colorClear: %u\r\n", data.colors[0]);
        PRINTF("[DATA SENDER PROCESS] colorRed: %u\r\n", data.colors[1]);
        PRINTF("[DATA SENDER PROCESS] colorRed: %u\r\n", data.colors[2]);
        PRINTF("[DATA SENDER PROCESS] colorRed: %u\r\n", data.colors[3]);

        count = 0;
        // while the current message hasn't tried to send count times
        // and less messages have been acknowledged than have been sent
        while ((count < 10) && (data_seq_acked < sequence)) {

        	// if the message hasn't tried to send yet or the eTimer has expired
            if ((count == 0) || (etimer_expired(&et)))
            {
            	// send the current message and set up a timer to control
            	// how long it will wait until trying to resend the data
                PRINTF("[DATA SENDER PROCESS] Sending data - attempt %d \r\n", count);
                messenger_send(&server, &data, sizeof(data_t));
                etimer_set(&et, /*2 * CLOCK_SECOND*/ config_get_bcast_interval() );
            }

            // wait until the process receives an event
            PROCESS_WAIT_EVENT( );

            // if the event is that the data has been acknowledged (posted by data_ack_handler())
            // and the same number of messages have been acknowledged as have been sent
            if ((ev == PROCESS_EVENT_MSG) && (data_seq_acked == sequence)) {
                PRINTF("[DATA SENDER PROCESS] OK - data is ACKd\r\n");
                break;
            }

            else if(ev == PROCESS_EVENT_POLL){
            	PRINTF("[DATA SENDER PROCESS] New SCS data available: %d times since data obtained...\r\n", ++newDatRcvd);
            }

            // if the timer for waiting for an acknowledgment has expired
            else if (etimer_expired(&et)) {
                PRINTF("[DATA SENDER PROCESS] Timeout waiting for ACK\r\n");
                count++;
            }

            // an event was posted to this process that was not the expiration
            // of the timer or an acknowledgment
            else {
                PRINTF("[DATA SENDER PROCESS] Seq: %d  Last ack: %d\r\n",sequence , data_seq_acked);
                PRINTF("[DATA SENDER PROCESS] Unexpected wake-up (%d), resending\r\n", ev);
            }


        }

        PRINTF("\r\n\n\n[DATA SENDER PROCESS] Finished sending sequence %d\r\n\n\n", sequence);
        sequence++;



    }
    PROCESS_END();
}

void setSCData(SCIF_SCS_READ_DATA_OUTPUT_T data)
{
	scData = data;
	return;
}

void datahandler_init2( )
{
	scData.AmbLight = 0;
	scData.BatterySensor = 0;
	scData.Conductivity = 0;
	scData.HallSensor = 0;
	scData.I2CError = 0;
	scData.TemperatureSensor = 0;
	scData.colorClear = 0;
	scData.colorRed = 0;
	scData.colorGreen = 0;
	scData.colorBlue = 0;

    process_start(&test_data2, NULL);
    process_start(&sensor_timer, NULL);
}
