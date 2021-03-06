/*
 * config.h
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include <contiki.h>
#include <contiki-net.h>

extern process_event_t config_cmd_run;


// dynamic integer config values handled in config_get( );
typedef enum {
	CONFIG_CTIME = 10,		//0xA
	CONFIG_ROUTER = 11,			//0xB
	CONFIG_DEVTYPE = 12,			//0xC
	CONFIG_UPTIME = 13,			//0xD

	CONFIG_ENERGEST_CPU = 14,		//0xE
	CONFIG_ENERGEST_LPM = 15,		//0xF
	CONFIG_ENERGEST_TRANSMIT = 16,	//0x10
	CONFIG_ENERGEST_LISTEN = 17,		//0x11
	CONFIG_ENERGEST_DEEP_LPM = 18,		//0x12

	CONFIG_SENSOR_INTERVAL = 32,		//0x20
	CONFIG_MAX_FAILURES = 33,					// 0x21
	CONFIG_RETRY_INTERVAL = 34,				// 0x22

	// device specific calibration values
	CONFIG_CAL1 = 64,			// 0x40  --- this is used by Si7210 for selecting compensation
	CONFIG_CAL2 = 65,		  //       --- this is used by TCS3472 for selecting gain (0 = 1x, 1= 4x, 2=16x, 3 = 60x)
	CONFIG_CAL3 = 66,     //       --- this is used by TCS3472 for selecting cycles (0 .. 256)
	CONFIG_CAL4 = 67,
	CONFIG_CAL5 = 68,
	CONFIG_CAL6 = 69,
	CONFIG_CAL7 = 70,
	CONFIG_CAL8 = 71
} configtype_t;

// see wikipedia - https://en.wikipedia.org/wiki/Hexspeak
#define CONFIG_MAGIC (0x0B160000 | (VERSION_MAJOR << 4) | VERSION_MINOR)

typedef struct {
	uint32_t magic;
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t device_type;
	uint32_t sensor_interval;
	uint32_t max_failures;
	uint32_t retry_interval;

	uint16_t server[8];
	uint16_t local_calibration[8];
} config_t;



void config_init( unsigned int dev_type );
int config_read(config_t *config);
int config_write(config_t *config);
void config_list( );

void config_set_magic(uint32_t magic);
uint32_t config_get_magic( );
int config_is_magic( );

void config_set_ctime_offset(const int ctime);
int config_get_ctime_offset( ) ;

void config_set_sensor_interval(const int interval);
int config_get_sensor_interval( );

void config_set_major_version(uint32_t major_ver);
uint32_t config_get_major_version( );
void config_set_minor_version(uint32_t minor_ver);
uint32_t config_get_minor_version( );

int config_get_devtype( );

void config_set(const int configID, const int value);
int config_get (const int configID);

void config_get_receiver(uip_ip6addr_t *receiver);
void config_set_receiver(const uip_ip6addr_t *receiver);
uint16_t config_get_remote_port( );

uint32_t config_get_maxfailures();
void config_set_maxfailures(uint32_t max);

uint32_t config_get_retry_interval();
void config_set_retry_interval(uint32_t seconds);

void config_clear_calbration_changed( );
void config_set_calibration_change( );
int config_did_calibration_change( );

void config_set_calibration (int cal_num, uint16_t value);

void config_set_calibration(int cal_num, uint16_t value);
uint16_t config_get_calibration(int cal_num);

void config_timeout_change( ) __attribute__((weak));

#endif /* CONFIG_H_ */

