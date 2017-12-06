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

#include "config.h"

static config_t config;
static dynamic_config_t dynconfig;

#define SECONDS(x) (x * CLOCK_SECOND)

static void config_read( )
{
	//TODO Read the configuration from the flash
	memset(&config, 0, sizeof(config));
}

static void config_write( )
{
	//TODO write the configuration to the flash
}

void config_init()
{
	config_read( );
	if (config.magic != CONFIG_MAGIC) {
		printf("Configuration magic (%-8.8x != %-8.8x) not found, using defaults\n", config.magic, CONFIG_MAGIC);
		config.magic = CONFIG_MAGIC;
		config_set_bcast_interval( SECONDS(30) );
		config_set_sensor_interval( SECONDS(30) );
		config_set_neighbor_interval( SECONDS(30) );
		config_write( );
		config_read();
	}

	memset(&dynconfig, 0, sizeof(dynconfig));

}


void config_set_sensor_interval(const int interval)
{
	if (interval < SECONDS(5)) {
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > SECONDS(3600)) {
		printf("Error - interval too large (5...3600) seconds\n");
		return;
	}

	config.sensor_interval = interval;
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
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > SECONDS(3600)) {
		printf("Error - interval too large (5...3600) seconds\n");
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
		printf("Error - interval too small (5...3600) seconds\n");
	}
	else if (interval > SECONDS(3600)) {
		printf("Error - interval too large (5...3600) seconds\n");
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
