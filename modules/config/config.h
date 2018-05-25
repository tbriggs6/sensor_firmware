/*
 * config.h
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define VERSION_MAJOR 3
#define VERSION_MINOR 0

#include <contiki.h>
#include <contiki-net.h>

// dynamic integer config values handled in config_get( );
typedef enum {
	CONFIG_CTIME = 10,		//0xA
	CONFIG_ROUTER ,			//0xB
	CONFIG_DEVTYPE,			//0xC
	CONFIG_UPTIME,			//0xD

	CONFIG_ENERGEST_CPU,		//0xE
	CONFIG_ENERGEST_LPM,		//0xF
	CONFIG_ENERGEST_TRANSMIT,	//0x10
	CONFIG_ENERGEST_LISTEN,		//0x11
	CONFIG_ENERGEST_DEEP_LPM,		//0x12

	CONFIG_BROADCAST_INTERVAL,	//0x13
	CONFIG_SENSOR_INTERVAL,		//0x14
	CONFIG_NEIGHBOR_INTERVAL,	//0x15

	CONFIG_NEIGHBOR_NUM,		//0x16
	CONFIG_NEIGHBOR_POS,		//0x17
	CONFIG_NEIGHBOR_RSSI,		//0x18
	CONFIG_NEIGHBOR_ROUTER,		//0x19

	CONFIG_RTIMER_SECOND		//0x1A

} configtype_t;

// see wikipedia - https://en.wikipedia.org/wiki/Hexspeak
#define CONFIG_MAGIC (0xB16B00B5)

typedef struct {
	uint32_t major_version;
	uint32_t minor_version;
	int magic;
	int bcast_interval;
	int sensor_interval;
	int neighbor_interval;
	uint16_t server[8];
} config_t;

typedef struct {
	int ctime;
	uip_ipaddr_t receiver;
} dynamic_config_t;



void config_init( );
void config_read( );
void config_write( );

void config_set_magic(uint32_t magic);
uint32_t config_get_magic( );
int config_is_magic( );

void config_set_ctime_offset(const int ctime);
int config_get_ctime_offset( ) ;

void config_set_bcast_interval(const int interval);
int config_get_bcast_interval( );

void config_set_sensor_interval(const int interval);
int config_get_sensor_interval( );

void config_set_neighbor_interval(const int interval);
int config_get_neighbor_interval( );

void config_set_major_version(uint32_t major_ver);
uint32_t config_get_major_version( );
void config_set_minor_version(uint32_t minor_ver);
uint32_t config_get_minor_version( );

int config_get_devtype( );

void config_set(const int configID, const int value);

void config_get_receiver(uip_ip6addr_t *receiver);
void config_set_receiver(const uip_ip6addr_t *receiver);


#endif /* CONFIG_H_ */
