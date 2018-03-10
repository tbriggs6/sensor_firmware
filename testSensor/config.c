/*
 * config.c
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#include <contiki.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <contiki-net.h>
#include <net/ip/uip-debug.h>
#include <sys/compower.h>

#include <driverlib/flash.h>
#include <driverlib/vims.h>

#include "config.h"
#include "sensor_handler.h"

extern void * _uflash_base;
extern void * _uflash_size;

static config_t config;
static dynamic_config_t dynconfig;

#define SECONDS(x) (x * CLOCK_SECOND)

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static void config_read()
{
	config_t *statconfig = (config_t *) &_uflash_base;
	config.magic = statconfig->magic;
	config.bcast_interval = statconfig->bcast_interval;
	config.sensor_interval = statconfig->sensor_interval;
	config.neighbor_interval = statconfig->neighbor_interval;
}

static void config_write()
{
	uint32_t mybase = (uint32_t) &_uflash_base;
	uint32_t mysize = (uint32_t) &_uflash_size;

	if(FlashPowerModeGet() != FLASH_PWR_ACTIVE_MODE){
		FlashPowerModeSet (FLASH_PWR_ACTIVE_MODE, 1024 * 1024, 1024 * 1024);
	}

	// Disable and flush the cache so that the old memory isn't there
	VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
	while(VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);
	while(FlashCheckFsmForError() != FAPI_STATUS_SUCCESS);
	while(FlashCheckFsmForReady() != FAPI_STATUS_FSM_READY);

	/* Disable all interrupts when accessing flash, prevents memory from getting hosed */
	CPUcpsid();

	/* Erase the flash -- required in order to write */
	uint32_t resp = FlashSectorErase(mybase);
	if(resp == FAPI_STATUS_SUCCESS){
		PRINTF("[CONFIG WRITE] Flash erasure successful...\r\n");
	} else {
		PRINTF("[CONFIG WRITE] Flash erasure unsuccessful...\r\n");
	}

	resp = FlashProgram((uint8_t *) &config, mybase, sizeof(config_t));
	if(resp == FAPI_STATUS_SUCCESS){
		PRINTF("[CONFIG WRITE] Flash program successful...\r\n");
	} else {
		PRINTF("[CONFIG WRITE] Flash program unsuccessful...\r\n");
	}

	/* Reenable interupts */
	CPUcpsie();

	/* Re-enable the cache */
	VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
}

void config_init()
{
	config_read();
	// print config
	PRINTF("[CONFIG INIT] Configuration:\r\n");
	PRINTF("[CONFIG INIT] Magic: %X\r\n", config.magic);
	PRINTF("[CONFIG INIT] Broadcast interval: %d\r\n", config.bcast_interval);
	PRINTF("[CONFIG INIT] Neighbor interval: %d\r\n", config.neighbor_interval);
	PRINTF("[CONFIG INIT] Sensor interval: %d\r\n\n", config.sensor_interval);

	if (config.magic != CONFIG_MAGIC) {
		PRINTF("[CONFIG INIT] Configuration magic (%-8.8X != %-8.8X) not found, using defaults\r\n", config.magic, CONFIG_MAGIC);
		config.magic = CONFIG_MAGIC;
		config.bcast_interval = SECONDS(30);
		config.neighbor_interval = SECONDS(30);
		config.sensor_interval = SECONDS(5);

		config_write();
		config_read();

		// print config
		PRINTF("[CONFIG INIT] Configuration:\r\n");
		PRINTF("[CONFIG INIT] Magic: %X\r\n", config.magic);
		PRINTF("[CONFIG INIT] Broadcast interval: %d\r\n", config.bcast_interval);
		PRINTF("[CONFIG INIT] Neighbor interval: %d\r\n", config.neighbor_interval);
		PRINTF("[CONFIG INIT] Sensor interval: %d\r\n\n", config.sensor_interval);
	}

	

	memset(&dynconfig, 0, sizeof(dynconfig));

}


void config_set_sensor_interval(const int interval)
{
	if (interval < SECONDS(5)) {
		PRINTF("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		PRINTF("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.sensor_interval = interval;
	config_write();

	process_poll(&sensor_timer);

	return;
}

int config_get_sensor_interval()
{
	if (config.sensor_interval < (SECONDS(5)))
		return SECONDS(5);

	return config.sensor_interval;
}

void config_set_ctime_offset(const int ctime)
{
	//rtc_settime(ctime);
}

int config_get_ctime_offset()
{
	//return rtc_getoffset();
	return 0;
}

void config_set_bcast_interval(const int interval)
{
	if (interval < SECONDS(5)) {
		PRINTF("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		PRINTF("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.bcast_interval = interval;
	config_write();
}

int config_get_bcast_interval()
{

	if (config.bcast_interval < (SECONDS(30)))
		return SECONDS(30);

	return config.bcast_interval;
}


void config_set_neighbor_interval(const int interval)
{
	if (interval < (SECONDS(5))) {
		PRINTF("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		PRINTF("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.neighbor_interval = interval;
	config_write();
}

int config_get_neighbor_interval()
{
	if (config.neighbor_interval < (SECONDS(10)))
		return (SECONDS(10));

	return config.neighbor_interval;
}
int config_get_devtype( )
{
	return CONFIG_DEVTYPE;
}

void config_set(const int configID, const int value)
{

	switch (configID) {
	case CONFIG_CTIME:
		config_set_ctime_offset(value);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		config_set_bcast_interval(value);
		break;

	case CONFIG_SENSOR_INTERVAL:
		config_set_sensor_interval(value);
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		config_set_neighbor_interval(value);
		break;

	default:
		PRINTF("Error - unknown config ID: %d\r\n", configID);
	}
}


void config_get_receiver(uip_ipaddr_t *receiver)
{
	memcpy(receiver, &dynconfig.receiver, sizeof(dynconfig.receiver));
}

void config_set_receiver(const uip_ipaddr_t *receiver)
{
	PRINTF("Setting receiver address to ");
	uip_debug_ipaddr_print(receiver);

	memcpy(&(dynconfig.receiver), receiver, sizeof(dynconfig.receiver));
}
