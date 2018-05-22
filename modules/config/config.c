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
#include <sys/compower.h>

#define DEBUG

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "CONFIG"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#include "../../modules/config/config.h"

config_t config;
dynamic_config_t dynconfig;

#define SECONDS(x) (x * CLOCK_SECOND)






void config_init()
{
	// handled by target / platform
	config_read();

	// print config
	LOG_DBG("Configuration:\r\n");
	LOG_DBG("Version number: %u.%u\r\n", (unsigned int) config.major_version, (unsigned int) config.minor_version);
	LOG_DBG("Magic: %X\r\n", config.magic);
	LOG_DBG("Broadcast interval: %d\r\n", config.bcast_interval);
	LOG_DBG("Neighbor interval: %d\r\n", config.neighbor_interval);
	LOG_DBG("Sensor interval: %d\r\n\n", config.sensor_interval);

	if (config.magic != CONFIG_MAGIC) {
		LOG_INFO("Configuration magic (%-8.8X != %-8.8X) not found, using defaults\r\n", config.magic, CONFIG_MAGIC);

		config.magic = CONFIG_MAGIC;

		config_set_major_version(VERSION_MAJOR);
		config_set_minor_version(VERSION_MINOR);
		config_set_bcast_interval(20);
		config_set_neighbor_interval(20);
		config_set_sensor_interval(10);
		uip_ip6addr_t server;
		uiplib_ip6addrconv("fd00::1", &server);
		config_set_receiver(&server);

		config_write();
		config_read();

		// print config
		LOG_INFO("Configuration:\r\n");
		LOG_INFO("Version number: %u.%u\r\n", (unsigned int) config.major_version, (unsigned int) config.minor_version);
		LOG_INFO("Magic: %X\r\n", config.magic);
		LOG_INFO("Broadcast interval: %d\r\n", config.bcast_interval);
		LOG_INFO("Neighbor interval: %d\r\n", config.neighbor_interval);
		LOG_INFO("Sensor interval: %d\r\n\n", config.sensor_interval);
	}


	memset(&dynconfig, 0, sizeof(dynconfig));

}


void config_set_magic(uint32_t magic)
{
	config.magic = magic;
}

uint32_t config_get_magic( )
{
	return config.magic;
}

int config_is_magic( )
{
	return config.magic == CONFIG_MAGIC;
}

void config_set_sensor_interval(const int seconds)
{
	int interval = SECONDS(seconds);

	if (interval < SECONDS(5)) {
		LOG_INFO("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		LOG_INFO("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.sensor_interval = interval;
	config_write();

	//TODO
//	#if DEPLOYABLE
//
//	process_poll(&sensor_timer);
//
//	#endif // DEPLOYABLE

	return;
}

int config_get_sensor_interval()
{
	if (config.sensor_interval < (SECONDS(5)))
		return SECONDS(5);

	return config.sensor_interval  / CLOCK_SECOND;
}

void config_set_ctime_offset(const int ctime)
{
	//TODO add RTC
	//rtc_settime(ctime);
}

int config_get_ctime_offset()
{
	//TODO add RTC
	//return rtc_getoffset();
	return 0;
}

void config_set_bcast_interval(const int  seconds)
{
	int interval = SECONDS(seconds);

	if (interval < SECONDS(5)) {
		LOG_DBG("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		LOG_DBG("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.bcast_interval = interval;
	config_write();
}

int config_get_bcast_interval()
{
	// convert interval back to seconds
	return config.bcast_interval / CLOCK_SECOND;
}


void config_set_neighbor_interval(const int seconds)
{
	int interval = SECONDS(seconds);

	if (interval < (SECONDS(5))) {
		LOG_DBG("Error - interval too small (5...3600) seconds\r\n");
	}
	else if (interval > SECONDS(3600)) {
		LOG_DBG("Error - interval too large (5...3600) seconds\r\n");
		return;
	}

	config.neighbor_interval = interval;
	config_write();
}

int config_get_neighbor_interval()
{
	if (config.neighbor_interval < (SECONDS(10)))
		return (SECONDS(10));

	return config.neighbor_interval  / CLOCK_SECOND;
}


void config_set_major_version(uint32_t major_ver)
{
	config.major_version = major_ver;
	config_write();
}

uint32_t config_get_major_version( )
{
	return config.major_version;
}

void config_set_minor_version(uint32_t minor_ver)
{
	config.minor_version = minor_ver;
	config_write();
}

uint32_t config_get_minor_version( )
{
	return config.minor_version;
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
		LOG_DBG("Error - unknown config ID: %d\r\n", configID);
	}
}


void config_get_receiver(uip_ipaddr_t *receiver)
{
	memcpy(receiver, &dynconfig.receiver, sizeof(dynconfig.receiver));
}

void config_set_receiver(const uip_ipaddr_t *receiver)
{
	LOG_DBG("Setting receiver address to ");
	LOG_6ADDR(LOG_LEVEL_DBG, receiver);
	LOG_DBG_("\n");

	memcpy(&(dynconfig.receiver), receiver, sizeof(dynconfig.receiver));
}
