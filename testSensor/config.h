/*
 * config.h
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <contiki.h>
#include <net/ip/uip.h>


// dynamic integer config values handled in config_get( );
typedef enum {
	CONFIG_CTIME = 10,
	CONFIG_ROUTER ,
	CONFIG_DEVTYPE,
	CONFIG_UPTIME,

	CONFIG_ENERGEST_CPU,
	CONFIG_ENERGEST_LPM,
	CONFIG_ENERGEST_TRANSMIT,
	CONFIG_ENERGEST_LISTEN,
	CONFIG_ENERGEST_MAX,

	CONFIG_BROADCAST_INTERVAL,
	CONFIG_SENSOR_INTERVAL,
	CONFIG_NEIGHBOR_INTERVAL,

	CONFIG_NEIGHBOR_NUM,
	CONFIG_NEIGHBOR_POS,
	CONFIG_NEIGHBOR_RSSI,
	CONFIG_NEIGHBOR_ROUTER,

	CONFIG_RTIMER_SECOND,
	CONFIG_FORCE_32_BIT = 0x7FFFFFFF

} configtype_t;

// see wikipedia - https://en.wikipedia.org/wiki/Hexspeak
#define CONFIG_MAGIC (0xB16B00B5)

typedef struct {
	int magic;
	int bcast_interval;
	int sensor_interval;
	int neighbor_interval;
} config_t;

typedef struct {
	int ctime;
	uip_ipaddr_t receiver;
} dynamic_config_t;

void config_init( );

void config_set_ctime_offset(const int ctime);
int config_get_ctime_offset( ) ;
void config_set_bcast_interval(const int interval);
int config_get_bcast_interval( );
void config_set_sensor_interval(const int interval);
int config_get_sensor_interval();
void config_set_neighbor_interval(const int interval);
int config_get_neighbor_interval();

int config_get_devtype( );

void config_set(const int configID, const int value);

void config_get_receiver(uip_ipaddr_t *receiver);
void config_set_receiver(const uip_ipaddr_t *receiver);


#endif /* CONFIG_H_ */
