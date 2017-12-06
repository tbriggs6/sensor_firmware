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

PROCESS(test_data, "Data Handler");


volatile static int data_seq_acked = -1;

void data_ack_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{

    data_ack_t *ack = (data_ack_t *) data;
    if (ack->header != DATA_ACK_HEADER) {
        printf("unexpected message - not a data ack\n");
        return;
    }

    printf("Data ack seq: %d\n", ack->ack_seq);
    data_seq_acked = ack->ack_seq;
    process_post(&test_data, PROCESS_EVENT_MSG, data);
}




PROCESS_THREAD(test_data, ev, data)
{
    PROCESS_BEGIN( );

    static struct etimer et;
    static data_t data;
    static int sequence = 0;
    static uip_ipaddr_t server;
    static int count = 0;

    uip_ip6addr(&server, 0xFD00, 0, 0, 0, 0, 0, 0, 1);
    messenger_add_handler(DATA_ACK_HEADER, sizeof(data_ack_t), sizeof(data_ack_t), data_ack_handler);

    printf("Data Sender\n");
    while(1)
    {
        printf("Waiting for %d  /  %d\n", config_get_sensor_interval(), config_get_sensor_interval() / CLOCK_SECOND);
        etimer_set(&et, config_get_sensor_interval());
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        data.header = DATA_HEADER;
        data.sequence = sequence;
        data.battery = 55;
        data.temperature = 33;
        data.adc[0] = 0x1010;
        data.adc[1] = 0x2031;
        data.adc[2] = 0x3043;
        data.adc[3] = 0x4343;

        count = 0;
        while ((count < 10) && (data_seq_acked < sequence)) {


            if ((count == 0) || (etimer_expired(&et)))
            {
                printf("Sending data - attempt %d \n", count);
                messenger_send(&server, &data, sizeof(data_t));
                etimer_set(&et, 2 * CLOCK_SECOND);
            }

            PROCESS_WAIT_EVENT( );

            if ((ev == PROCESS_EVENT_MSG) && (data_seq_acked == sequence)) {
                printf("OK - data is ACKd\n");
                break;
            }

            else if (etimer_expired(&et)) {
                printf("Timeout waiting for ACK\n");
                count++;
            }

            else {
                printf("Seq: %d  Last ack: %d\n",sequence , data_seq_acked);
                printf("Unexpected wake-up (%d), resending\n", ev);
            }


        }

        printf("\n\n\nFinished sending sequence %d\n\n\n", sequence);
        sequence++;



    }
    PROCESS_END();
}


void datahandler_init( )
{
    process_start(&test_data, NULL);
}
