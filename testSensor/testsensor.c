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
#include "datahandler.h"
#include <powertrace.h>

#define DEPLOYABLE 0

#if  DEPLOYABLE

#include "scif_framework.h"
#include "scif_osal_contiki.h"
#include "scif_scs.h"

#endif // DEPLOYABLE

#ifndef BV
#define BV(n)           (1 << (n))
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

	PRINTF("echo handler invoked %x - %s from ", (unsigned int) echoreq->header, echoreq->message);
	PRINT6ADDR(remote_addr);
	PRINTF(" port %d\n", remote_port);

	if (length != sizeof(echo_t)) return;

	PRINTF("Sending reply\n");

	echo_t echo_rep;
	memcpy(&echo_rep, echoreq, sizeof(echo_t));

	echo_rep.header = ECHO_REPL;
	strcpy((char *)&echo_rep.message, (const char *) "repl");

	messenger_send(remote_addr, &echo_rep, sizeof(echo_rep));
}

PROCESS_THREAD(test_bcast_cb, ev, data)
{

    PROCESS_BEGIN( );

    PRINTF("\r\n[CONTIKI TEST BROADCAST PROCESS] Starting initializations...\r\n");

    config_init( );

    neighbors_init( );

    #if DEPLOYABLE

    sensor_aux_init();
    PRINTF("Starting power trace, %u ticks per second\n", RTIMER_SECOND);
    energest_init();
    powertrace_start(60 * CLOCK_SECOND);

    #endif

    bcast_init(0);

    bcast_add_observer(&test_bcast_cb);

    message_init( );

    messenger_add_handler(ECHO_REQ, sizeof(echo_t), sizeof(echo_t), echo_handler);

    messenger_add_handler(CMD_RET_HEADER, 0, 1000000, command_handler);
    messenger_add_handler(CMD_SET_HEADER, 0, 1000000, command_handler);

    datahandler_init();

    PRINTF("[CONTIKI TEST BROADCAST PROCESS] Broadcast CB started...\r\n");

    while(1)
    {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            bcast_t *bcast = (bcast_t *) data;
            PRINTF("******************\r\n");
            PRINTF("bcast event: ");
            PRINTF("from: ");
            uip_debug_ipaddr_print(&(bcast->sender_addr));
            PRINTF("  port: %d\r\n", bcast->sender_port);

            PRINTF("len: %d message: %s\r\n", bcast->datalen, bcast->data);
	}
        else {
            PRINTF("Different event: %d\r\n", ev);
        }
    }

    bcast_remove_observer(&test_bcast_cb);

    PROCESS_END();
}
