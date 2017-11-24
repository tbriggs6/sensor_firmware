/*
 * config.c
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#include <contiki.h>
#include <ti-lib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <contiki-net.h>
#include <net/ip/uip-debug.h>

#include <ti-lib.h>
#include <driverlib/flash.h>
#include <driverlib/vims.h>
#include <sys/compower.h>

#include "rtc.h"
#include "config.h"

static volatile config_t flash_config __attribute__ ((section (".config")));
static config_t config;
static dynamic_config_t dynconfig;


static void config_read( )
{

	//TODO - there is still a caching issue here....
	bool state = IntMasterDisable();
	uint32_t save_mode = VIMSModeGet(VIMS_BASE);
	VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
	while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);

	memset(&config, 0xff, sizeof(config_t));

    memcpy(&config, (void *) &flash_config, sizeof(config_t));

    VIMSModeSet(VIMS_BASE, save_mode);
    while (VIMSModeGet(VIMS_BASE) != save_mode);

    if (state) {
    	IntMasterEnable();
    }

	return;
}

static void config_write( )
{
	int rce = 0, rcp = 0;

	printf("Config - writing\n");

	bool state = IntMasterDisable();
	uint32_t save_mode = VIMSModeGet(VIMS_BASE);
	VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
	while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);

	rce = FlashSectorErase((uint32_t) &flash_config);
	if (rce == FAPI_STATUS_SUCCESS)
		rcp = FlashProgram((uint8_t *) &config, (uint32_t) &flash_config, sizeof(config_t));

    VIMSModeSet(VIMS_BASE, save_mode);
    while (VIMSModeGet(VIMS_BASE) != save_mode) {
    	;
    }

	if (state) {
		IntMasterEnable();
	}

	if (rce != FAPI_STATUS_SUCCESS) {
		if (rce == FAPI_STATUS_INCORRECT_DATABUFFER_LENGTH)
			printf("Config - error incorrect databuffer length during erase\n");
		else if (rce == FAPI_STATUS_FSM_ERROR)
			printf("Config - error FSM error during erase\n");
		else
			printf("Config - error during erase: %d\n", rce);
	}
	else if (rcp != FAPI_STATUS_SUCCESS) {
		if (rcp == FAPI_STATUS_INCORRECT_DATABUFFER_LENGTH)
			printf("Config - error incorrect databuffer length during program\n");
		else if (rcp == FAPI_STATUS_FSM_ERROR)
			printf("Config - error FSM error during program\n");
		else
			printf("Config - error during program: %d\n", rce);
	}

	printf("Config - done\n");
}

void config_init()
{
	config_read( );
	if (config.magic != CONFIG_MAGIC) {
		printf("Configuration magic (%-8.8x != %-8.8x) not found, using defaults\n", config.magic, CONFIG_MAGIC);
		config.magic = CONFIG_MAGIC;
		config_set_bcast_interval(30 * CLOCK_SECOND);
		config_set_sensor_interval(30 * CLOCK_SECOND);
		config_set_neighbor_interval(30 * CLOCK_SECOND);
		config_write( );
		config_read();
	}

	memset(&dynconfig, 0, sizeof(dynconfig));

}


void config_set_sensor_interval(const int interval)
{
	if (interval < 5) {
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > 3600) {
		printf("Error - interval too large (5...3600) seconds\n");
		return;
	}

	config.sensor_interval = interval;
}

int config_get_sensor_interval()
{
	if (config.sensor_interval < (CLOCK_SECOND))
		return 1 * CLOCK_SECOND;

	return config.sensor_interval;
}

void config_set_ctime_offset(const int ctime)
{
	rtc_settime(ctime);
}

int config_get_ctime_offset()
{
	return rtc_getoffset();
}

void config_set_bcast_interval(const int interval)
{
	if (interval < 5) {
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > 3600) {
		printf("Error - interval too large (5...3600) seconds\n");
		return;
	}

	config.bcast_interval = interval;
	config_write();
}

int config_get_bcast_interval()
{

	if (config.bcast_interval < (CLOCK_SECOND))
		return 30 * CLOCK_SECOND;

	return config.bcast_interval;
}


void config_set_neighbor_interval(const int interval)
{
	if (interval < 5) {
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > 3600) {
		printf("Error - interval too large (5...3600) seconds\n");
		return;
	}

	config.neighbor_interval = interval;
	config_write();
}

int config_get_neighbor_interval()
{
	if (config.neighbor_interval < (CLOCK_SECOND))
		return 10 * CLOCK_SECOND;

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
//
//	case CONFIG_ROUTER:
//		config_set_router(address);
//		break;

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
		printf("Error - unknown config ID: %d", configID);
	}
}


void config_get_receiver(uip_ipaddr_t *receiver)
{
	memcpy(receiver, &dynconfig.receiver, sizeof(dynconfig.receiver));
}

void config_set_receiver(const uip_ipaddr_t *receiver)
{
	printf("Setting receiver address to ");
	uip_debug_ipaddr_print(receiver);

	memcpy(&(dynconfig.receiver), receiver, sizeof(dynconfig.receiver));
}
