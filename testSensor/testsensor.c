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
	strcpy(&echo_rep.message, "repl");

	messenger_send(remote_addr, remote_port, &echo_rep, sizeof(echo_rep));
}


PROCESS_THREAD(test_bcast_cb, ev, data)
{
    PROCESS_BEGIN( );

    bcast_init(0);

    bcast_add_observer(&test_bcast_cb);

    message_init( );

    messenger_add_handler(ECHO_REQ, sizeof(echo_t), sizeof(echo_t), echo_handler);

    printf("Broadcast CB started\n");
    while(1)
    {
        PROCESS_WAIT_EVENT();
        if (ev == bcast_event) {
            bcast_t *bcast = (bcast_t *) data;
            printf("******************\n");
            printf("bcast event: ");
            printf("from: ");
            uip_debug_ipaddr_print(&(bcast->sender_addr));
            printf("  port: %d\n", bcast->sender_port);

            printf("len: %d message: %s\n", bcast->datalen, bcast->data);
         }
        else {
            printf("Different event: %d\n", ev);
        }
    }

    bcast_remove_observer(&test_bcast_cb);

    PROCESS_END();
}
