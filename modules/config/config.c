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

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "CONFIG"
#define LOG_LEVEL LOG_LEVEL_DBG

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif

#include "../../modules/config/config.h"

config_t config;

#define SECONDS(x) (x * CLOCK_SECOND)


void config_init()
{
	// handled by target / platform
	config_read();

	if (config.magic != CONFIG_MAGIC) {
		LOG_INFO("Configuration magic (%-8.8X != %-8.8X) not found, using defaults\r\n", (unsigned int) config.magic, (unsigned int)CONFIG_MAGIC);

		config.magic = CONFIG_MAGIC;

		config_set_major_version(VERSION_MAJOR);
		config_set_minor_version(VERSION_MINOR);

		config_set_sensor_interval(10); // seconds to wait to send next sensor reading
		config_set_maxfailures(100);  // consecutive failures before reboot
		config_set_retry_interval(15);	// retry sending msgs in seconds

		uip_ip6addr_t server;
		uiplib_ip6addrconv("fd00::1", &server);
		config_set_receiver(&server);

//		printf("Writing config\n");
//		config_write();
//		config_read();
	}

	// print config
	LOG_INFO("Configuration:\r\n");
	LOG_INFO("Version number: %u.%u\r\n", (unsigned int) config.major_version, (unsigned int) config.minor_version);
	LOG_INFO("Magic: %X\r\n", (unsigned int) config.magic);
	LOG_INFO("Sensor interval: %d\r\n\n", (unsigned int) config.sensor_interval);
	LOG_INFO("Max failures: %d\r\n\n", (unsigned int) config.max_failures);
	LOG_INFO("Retry interval: %d\r\n\n", (unsigned int) config.retry_interval);

	LOG_INFO("Stored destination address: ");
	uip_ip6addr_t addr;
	config_get_receiver(&addr);
	LOG_6ADDR(LOG_LEVEL_INFO, &addr);
	LOG_INFO_("\n");
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

void config_set_major_version(uint32_t major_ver)
{
	config.major_version = major_ver;
}

uint32_t config_get_major_version( )
{
	return config.major_version;
}

void config_set_minor_version(uint32_t minor_ver)
{
	config.minor_version = minor_ver;
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

	case CONFIG_SENSOR_INTERVAL:
		config_set_sensor_interval(value);
		break;

	case CONFIG_MAX_FAILURES:
		config_set_maxfailures(value);
		break;

	case CONFIG_RETRY_INTERVAL:
		config_set_retry_interval(value);


	default:
		LOG_DBG("Error - unknown config ID: %d\r\n", configID);
	}
}


void config_get_receiver(uip_ipaddr_t *receiver)
{
	int i;

	for (i = 0; i < 8; i++) {
		receiver->u16[i] = config.server[i];
	}

}

void config_set_receiver(const uip_ipaddr_t *receiver)
{
	int i;

	for (i = 0; i < 8; i++) {
		config.server[i] = receiver->u16[i];
	}
}


uint32_t config_get_maxfailures()
{
	return config.max_failures;
}

void config_set_maxfailures(uint32_t max)
{
	if (max < 10)
		max = 10;

	config.max_failures = max;
}

uint32_t config_get_retry_interval()
{
	return config.retry_interval;
}

void config_set_retry_interval(uint32_t seconds)
{
	if (seconds < 1)
			seconds = 1;

	config.retry_interval = seconds;
}


