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

static unsigned int config_dev_type = 0;
static unsigned int calibration_changed = 0;

process_event_t config_cmd_run;

void config_init (unsigned int dev_type)
{
	// store dev_type
	config_dev_type = dev_type;

	// event for notifying a "hot" config change...
	config_cmd_run = process_alloc_event( );


	// handled by target / platform
	config_read (&config);

	if ((config.magic != CONFIG_MAGIC) || (config.device_type != dev_type)) {
		LOG_INFO("Configuration magic (%-8.8X != %-8.8X) not found, using defaults\r\n", (unsigned int ) config.magic,
							(unsigned int)CONFIG_MAGIC);

		config.magic = CONFIG_MAGIC;
		config.device_type = dev_type;
		config_set_major_version (VERSION_MAJOR);
		config_set_minor_version (VERSION_MINOR);

		config_set_sensor_interval (10); // seconds to wait to send next sensor reading
		config_set_maxfailures (100);  // consecutive failures before reboot
		config_set_retry_interval (15);	// retry sending msgs in seconds

		uip_ip6addr_t server;
		uiplib_ip6addrconv ("fd00::1", &server);
		config_set_receiver (&server);


		config_set_calibration(0, 2); // Si7210 set 20mT, Neodymium magnet
		config_set_calibration(1, 0); // TCS3472 gain of 1
		config_set_calibration(2, 64); //


		printf("Writing config\n");
		config_write(&config);
	}

	// print config
	LOG_INFO("Configuration:\r\n");
	LOG_INFO("Version number: %u.%u\r\n", (unsigned int ) config.major_version, (unsigned int ) config.minor_version);
	LOG_INFO("Magic: %X\r\n", (unsigned int ) config.magic);
	LOG_INFO("Sensor interval: %d\r\n\n", (unsigned int ) config.sensor_interval);
	LOG_INFO("Max failures: %d\r\n\n", (unsigned int ) config.max_failures);
	LOG_INFO("Retry interval: %d\r\n\n", (unsigned int ) config.retry_interval);

	LOG_INFO("Stored destination address: ");
	uip_ip6addr_t addr;
	config_get_receiver (&addr);
	LOG_6ADDR(LOG_LEVEL_INFO, &addr);
	LOG_INFO_("\n");



}

void config_set_magic (uint32_t magic)
{
	config.magic = magic;
}

uint32_t config_get_magic ()
{
	return config.magic;
}

int config_is_magic ()
{
	return config.magic == CONFIG_MAGIC;
}

void config_set_sensor_interval (const int seconds)
{

	config.sensor_interval = (seconds < 5) ? 5 :
													 (seconds > 7200) ? 7200 :
													 seconds;
	return;
}

int config_get_sensor_interval ()
{
	return config.sensor_interval;
}

void config_set_ctime_offset (const int ctime)
{
	//TODO add RTC
	//rtc_settime(ctime);
}

int config_get_ctime_offset ()
{
	//TODO add RTC
	//return rtc_getoffset();
	return 0;
}

void config_set_major_version (uint32_t major_ver)
{
	config.major_version = major_ver;
}

uint32_t config_get_major_version ()
{
	return config.major_version;
}

void config_set_minor_version (uint32_t minor_ver)
{
	config.minor_version = minor_ver;
}

uint32_t config_get_minor_version ()
{
	return config.minor_version;
}

int config_get_devtype ()
{
	return config_dev_type;
}

int config_get (const int configID)
{

	switch (configID)
		{
		case CONFIG_DEVTYPE: return config_get_devtype();
		case CONFIG_CTIME: return config_get_ctime_offset( );
		case CONFIG_SENSOR_INTERVAL: return config_get_sensor_interval( );
		case CONFIG_MAX_FAILURES: return config_get_maxfailures ( );
		case CONFIG_RETRY_INTERVAL: return config_get_retry_interval ( );
		case CONFIG_CAL1:	return config_get_calibration (0);
		case CONFIG_CAL2: return config_get_calibration (1);
		case CONFIG_CAL3: return config_get_calibration (2);
		case CONFIG_CAL4: return config_get_calibration (3);
		case CONFIG_CAL5: return config_get_calibration (4);
		case CONFIG_CAL6: return config_get_calibration (5);
		case CONFIG_CAL7: return config_get_calibration (6);
		case CONFIG_CAL8: return config_get_calibration (7);
		default:
			LOG_DBG("Error - unknown config ID: %d\r\n", configID);
		}
	return -1;
}


void config_set (const int configID, const int value)
{

	switch (configID)
		{
		case CONFIG_CTIME:
			config_set_ctime_offset (value);
			break;

		case CONFIG_SENSOR_INTERVAL:
			config_set_sensor_interval (value);
			break;

		case CONFIG_MAX_FAILURES:
			config_set_maxfailures (value);
			break;

		case CONFIG_RETRY_INTERVAL:
			config_set_retry_interval (value);
			break;

		case CONFIG_CAL1:
			config_set_calibration (0, value);
			break;

		case CONFIG_CAL2:
			config_set_calibration (1, value);
			break;

		case CONFIG_CAL3:
			config_set_calibration (2, value);
			break;

		case CONFIG_CAL4:
			config_set_calibration (3, value);
			break;

		case CONFIG_CAL5:
			config_set_calibration (4, value);
			break;

		case CONFIG_CAL6:
			config_set_calibration (5, value);
			break;

		case CONFIG_CAL7:
			config_set_calibration (6, value);
			break;

		case CONFIG_CAL8:
			config_set_calibration (7, value);
			break;

		default:
			LOG_DBG("Error - unknown config ID: %d\r\n", configID);
		}
}

void config_get_receiver (uip_ipaddr_t *receiver)
{
	int i;

	for (i = 0; i < 8; i++) {
		receiver->u16[i] = config.server[i];
	}

}

void config_set_receiver (const uip_ipaddr_t *receiver)
{
	int i;

	for (i = 0; i < 8; i++) {
		config.server[i] = receiver->u16[i];
	}
}

uint32_t config_get_maxfailures ()
{
	return config.max_failures;
}

void config_set_maxfailures (uint32_t max)
{
	if (max < 10)
		max = 10;

	config.max_failures = max;
}

uint32_t config_get_retry_interval ()
{
	return config.retry_interval;
}

void config_set_retry_interval (uint32_t seconds)
{
	config.retry_interval = (seconds < 5) ? 5 :
			(seconds > 30) ? 30 :
					seconds;
}

void config_clear_calbration_changed( )
{
	calibration_changed = 0;
}

int config_did_calibration_change( )
{
	return calibration_changed;
}

void config_set_calibration (int cal_num, uint16_t value)
{
	if ((cal_num < 0) || (cal_num >= 16))
		return;

	LOG_DBG("Set calibration: %d = %u\n", cal_num, (unsigned int) value);
	config.local_calibration[cal_num] = value;
	calibration_changed = 1;
}

uint16_t config_get_calibration (int cal_num)
{
	if ((cal_num < 0) || (cal_num >= 16))
		return 0xffff;

	return config.local_calibration[cal_num];
}


// weak function, leave this empty, but if
// you want to have the hot configurtion post
// then define this in your main code.
void config_timeout_change( )
{

}

