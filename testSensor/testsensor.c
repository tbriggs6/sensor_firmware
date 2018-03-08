/*
 * testsensor.c
 *
 *  Created on: Nov 22, 2017
 *      Author: contiki
 */

#include <contiki.h>
#include <stdio.h>
#include <bcast-service.h>
#include <net/ip/uip-debug.h>
#include <net/ipv6/uip-ds6.h>

#include <message-service.h>
#include "commandhandler.h"

#include "message.h"
#include "config.h"
#include "neighbors.h"

#define DEPLOYABLE

#ifdef  DEPLOYABLE
	#include "sensor_handler.h"
#else
	#include "datahandler.h"
#endif

#include "scif_framework.h"
#include "scif_osal_contiki.h"
#include "scif_scs.h"

#ifndef BV
#define BV(n)           (1 << (n))
#endif

PROCESS(test_bcast_cb, "Test Sensor Broadcast Handler");

AUTOSTART_PROCESSES(&test_bcast_cb);

#define ECHO_REQ 0x323232
#define ECHO_REPL 0x232323
typedef struct {
	uint32_t header;
	char message[32];
} echo_t;

void echo_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{
	echo_t *echoreq = (echo_t *) data;

	printf("echo handler invoked %x - %s from ", echoreq->header, echoreq->message);
	PRINT6ADDR(remote_addr);
	printf(" port %d\n", remote_port);

	if (length != sizeof(echo_t)) return;

	printf("Sending reply\n");

	echo_t echo_rep;
	memcpy(&echo_rep, echoreq, sizeof(echo_t));

	echo_rep.header = ECHO_REPL;
	strcpy((char *)&echo_rep.message, (const char *) "repl");

	messenger_send(remote_addr, &echo_rep, sizeof(echo_rep));
}

PROCESS_THREAD(test_bcast_cb, ev, data)
{
	static struct etimer et;

    PROCESS_BEGIN( );

    printf("Thread Test is begun\r\n()");

    config_init( );

    sensor_aux_init();

    neighbors_init( );

    bcast_init(0);

    bcast_add_observer(&test_bcast_cb);

    message_init( );


    messenger_add_handler(ECHO_REQ, sizeof(echo_t), sizeof(echo_t), echo_handler);
    messenger_add_handler(CMD_SET_HEADER, sizeof(uint32_t) * 4, sizeof(command_set_t), command_handler);

    printf("Before data handler init()\r\n");
	#ifdef DEPLOYABLE
    	datahandler_init2( );
	#else
    	datahandler_init();
	#endif

    printf("Broadcast CB started\r\n");

//    while(1){
//    	etimer_set(&et, config_get_sensor_interval());
//		PROCESS_WAIT_UNTIL(etimer_expired(&et));
//    	//scifExecuteTasksOnceNbl(BV(SCIF_SCS_ANALOG_SENSOR_TASK_ID));
//    	scifSwTriggerExecutionCodeNbl(BV(SCIF_SCS_ANALOG_SENSOR_TASK_ID));
//    }

//    while(1){
//    	;
//    }


    while(1)
    {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            bcast_t *bcast = (bcast_t *) data;
            printf("******************\r\n");
            printf("bcast event: ");
            printf("from: ");
            uip_debug_ipaddr_print(&(bcast->sender_addr));
            printf("  port: %d\r\n", bcast->sender_port);

            printf("len: %d message: %s\r\n", bcast->datalen, bcast->data);
	}
        else {
            printf("Different event: %d\r\n", ev);
        }
    }

    bcast_remove_observer(&test_bcast_cb);

    PROCESS_END();
}
