/*
 * datahandler.c
 *
 *  Created on: Dec 4, 2017
 *      Author: contiki
 */


#include "message.h"
#include "config.h"
#include "neighbors.h"
#include "message-service.h"
#include <contiki.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEPLOYABLE 1

#if DEPLOYABLE

#include "scif_framework.h"
#include "scif_osal_contiki.h"
#include "scif_scs.h"

#else

#define SCIF_SCS_READ_DATA_OUTPUT_T uint32_t

#endif

#ifndef BV
#define BV(n)	(1 << (n))
#endif

#ifdef PRINTF
#undef PRINTF
#endif

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(sensor_timer, "Sensor Interval Timer");
PROCESS(test_data, "Data Handler");


volatile static int data_seq_acked = -1;

#if DEPLOYABLE

volatile static SCIF_SCS_READ_DATA_OUTPUT_T scData;

#endif // DEPLOYABLE

void data_ack_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{

    data_ack_t *ack = (data_ack_t *) data;
    if (ack->header != DATA_ACK_HEADER) {
        PRINTF("unexpected message - not a data ack\r\n");
        return;
    }

    PRINTF("[DATA ACK HANDLER] Data ack seq: %u\r\n", (unsigned int) ack->ack_seq);
    data_seq_acked = ack->ack_seq;
    process_post(&test_data, PROCESS_EVENT_MSG, data);
}

PROCESS_THREAD(sensor_timer, ev, data)
{
        PROCESS_BEGIN();

        // create timer for sensor_interval seconds to attempt broadcast
        static struct etimer et;

        while(1){
                PRINTF("[SENSOR TIMER PROCESS] Waiting for %d  /  %d\r\n", config_get_sensor_interval(),
                                config_get_sensor_interval() / CLOCK_SECOND);
                etimer_set(&et, config_get_sensor_interval());

                PROCESS_WAIT_EVENT( );

                if (ev == PROCESS_EVENT_TIMER){
                        PRINTF("[SENSOR TIMER PROCESS] Sensor Timer expired. Restarting timer...\r\n");
                        
			#if DEPLOYABLE
			
			PRINTF("[SENSOR TIMER PROCESS] Invoking SCS code...\r\n");
                        if(osalIsReady()){
                                scifSwTriggerExecutionCodeNbl(BV(SCIF_SCS_READ_DATA_TASK_ID));
                                unsetOsalReady();
                        }
			
			#else

			process_poll(&test_data);

			#endif // DEPLOYABLE
                } else if (ev == PROCESS_EVENT_POLL){
                        PRINTF("[SENSOR TIMER PROCESS] Sensor interval changed. Restarting timer with new interval...\r\n");
                } else {
                        PRINTF("[SENSOR TIMER PROCESS] Unknown event generated. Resetting timer...\r\n");
                }
        }

        PROCESS_END();
}


PROCESS_THREAD(test_data, ev, data)
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
    messenger_add_handler(DATA_ACK_HEADER, sizeof(data_ack_t), sizeof(data_ack_t), data_ack_handler);

    PRINTF("[DATA SENDER PROCESS] Initializing...\r\n");
    while(1)
    {
        printf("Waiting for %d  /  %d\r\n", config_get_sensor_interval(), config_get_sensor_interval() / CLOCK_SECOND);
        etimer_set(&et, config_get_sensor_interval());
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

	#if DEPLOYABLE

	// read data from sensors
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

	#else

        // create fake data
        data.header = DATA_HEADER;
        data.sequence = sequence;
        data.battery = 55;
        data.temperature = 33;
        data.adc[0] = 0x1010;
        data.adc[1] = 0x2031;
        data.adc[2] = 0x3043;
        data.I2CError = 0;
        data.colors[0] = 0x2020;
        data.colors[1] = 0x3030;
        data.colors[2] = 0x4040;
        data.colors[3] = 0x5050;
	
	#endif // DEPLOYABLE

        PRINTF("\r\n\n\n[DATA SENDER PROCESS] Data to be broadcast:\r\n");
        PRINTF("[DATA SENDER PROCESS] Header: %X\r\n", (unsigned int) data.header);
        PRINTF("[DATA SENDER PROCESS] Sequence: %u\r\n", (unsigned int) data.sequence);
        PRINTF("[DATA SENDER PROCESS] Ambient Light: %u\r\n", data.adc[0]);
        PRINTF("[DATA SENDER PROCESS] Battery Sensor: %u\r\n", data.battery);
        PRINTF("[DATA SENDER PROCESS] Conductivity: %u\r\n", data.adc[1]);
        PRINTF("[DATA SENDER PROCESS] Hall Sensor: %u\r\n", data.adc[2]);
        PRINTF("[DATA SENDER PROCESS] I2CError: %u\r\n", data.I2CError);
        PRINTF("[DATA SENDER PROCESS] Temperature Sensor: %u\r\n", data.temperature);
        PRINTF("[DATA SENDER PROCESS] colorClear: %u\r\n", data.colors[0]);
        PRINTF("[DATA SENDER PROCESS] colorRed: %u\r\n", data.colors[1]);
        PRINTF("[DATA SENDER PROCESS] colorGreen: %u\r\n", data.colors[2]);
        PRINTF("[DATA SENDER PROCESS] colorBlue: %u\r\n", data.colors[3]);


        count = 0;
	while ((count < 10) && (data_seq_acked < sequence)) {

                // if the message hasn't tried to send yet or the eTimer has expired
            if ((count == 0) || (etimer_expired(&et)))
            {
                // send the current message and set up a timer to control
                // how long it will wait until trying to resend the data
                PRINTF("[DATA SENDER PROCESS] Sending data - attempt %d \r\n", count);
                messenger_send(&server, &data, sizeof(data_t));
                etimer_set(&et, 2 * CLOCK_SECOND /* config_get_bcast_interval()*/ );
                count++;
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
	#if DEPLOYABLE

        scData = data;

	#endif // DEPLOYABLE

        return;
}


void datahandler_init( )
{
	#if DEPLOYABLE

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
	
	#endif //DEPLOYABLE

	process_start(&test_data, NULL);
	process_start(&sensor_timer, NULL);
}
