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
#include <driverlib/vims.h>
#include <driverlib/flash.h>

#include "dev/cc26xx-uart.h"
#include "dev/serial-line.h"

#include <message-service.h>
#include "commandhandler.h"

#include "message.h"
#include "config.h"
#include "neighbors.h"
#include "datahandler.h"


PROCESS(test_bcast_cb, "Test Sensor Broadcast Handler");

AUTOSTART_PROCESSES(&test_bcast_cb);

#define ECHO_REQ 0x323232
#define ECHO_REPL 0x232323
typedef struct {
	uint32_t header;
	char message[32];
} echo_t;


extern void * _uflash_base;
extern void * _uflash_size;


void check_nvflash(){
	config_t * currentConfig = (config_t *) &_uflash_base;

	printf("Entered Check_nvFlash()\r\n");

	if(currentConfig->magic != CONFIG_MAGIC)
	{
		uint32_t mybase = (uint32_t) &_uflash_base;
		uint32_t mysize = (uint32_t) &_uflash_size;

		config_t myconf = { .magic = CONFIG_MAGIC, .bcast_interval = 100000, .neighbor_interval = 100000, .sensor_interval = 1000};


		FlashPowerModeSet (FLASH_PWR_ACTIVE_MODE, 1024 * 1024, 1024 * 1024);
		uint32_t sector_size = FlashSectorSizeGet ( );

		// Disable and flush the cache so that the old memory isn't there
		VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
		while(VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);

		/* Disable all interrupts when accessing flash, prevents memory from getting hosed */
		CPUcpsid();

		/* Erase the flash -- required in order to write */
		uint32_t resp = FlashSectorErase( mybase );
		resp = FlashProgram((uint8_t *) &myconf, mybase, sizeof(config_t));

		/* Reenable interupts */
		CPUcpsie();

		/* Re-enable the cache */
		VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
		printf("New Magic: %X\r\n", currentConfig->magic);
	}
	else
	{
		printf("Current Magic: %X\r\n", currentConfig->magic);
	}


}


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
    PROCESS_BEGIN( );

    config_init( );

    neighbors_init( );

    bcast_init(0);

    bcast_add_observer(&test_bcast_cb);

    message_init( );

    messenger_add_handler(ECHO_REQ, sizeof(echo_t), sizeof(echo_t), echo_handler);
    messenger_add_handler(CMD_SET_HEADER, sizeof(uint32_t) * 4, sizeof(command_set_t), command_handler);
    printf("Before data handler init()\n");
    datahandler_init( );

    /* FLASH */
    printf("Calling check_nvflash() ... \n");
    check_nvflash();

    /* end Flash */


    printf("Broadcast CB started\n");
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
