/*
 * sensor.c
 *
 * Simple sensor firmware.
 *
 *  Created on: Jul 5, 2016
 *      Author: Tom Briggs
 */

#include <contiki.h>
#include <random.h>

#include <sys/clock.h>
#include <dev/button-sensor.h>
#include <dev/batmon-sensor.h>
#include <board-peripherals.h>
#include <dev/leds.h>

#include <stdio.h>
#include <stdbool.h>

#include "../config.h"
#include "../rtc.h"
#include "../neighbors.h"

#include "scif_scs.h"
#include "scif_framework.h"
#include "scif_osal_contiki.h"

#include <powertrace.h>
#include "sender.h"
#include "command.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


PROCESS(sensor_process, "Sensor Process");
AUTOSTART_PROCESSES(&sensor_process);

PROCESS_THREAD(sensor_process, ev, data)
{
	PROCESS_BEGIN( );

	config_init();


	SENSORS_ACTIVATE(batmon_sensor);

	scifScsTaskData.analogSensor.cfg.scheduleDelay = config_get_sensor_interval();

	uip_ipaddr_t default_receiver;
	uip_ip6addr(&default_receiver, 0xfd00, 0,0,0,0,0,0,1);

	config_set_receiver(&default_receiver);

	sensor_aux_init();

	sender_init();
	command_init();

	neighbors_init();

	scifScsTaskData.analogSensor.cfg.scheduleDelay = config_get_sensor_interval();

	PRINTF("Starting power trace, %u ticks per second\n", RTIMER_SECOND);
	energest_init();
	powertrace_start(60 * CLOCK_SECOND);

	PROCESS_END();
	return 0;
}


void sensors_send()
{


//	int i = 0;
//	for (i = 0; i < 4; i++) {
//		printf("%d - %d\n", i, scifScsTaskData.analogSensor.output.anaValues[i]);
//	}
//
//	printf("Distance: %d? %d\n", 	scifScsTaskData.analogSensor.output.distanceValid,
//			((scifScsTaskData.analogSensor.output.distanceHigh << 16)
//			| scifScsTaskData.analogSensor.output.distanceLow));
//
//	printf("Battery: %d %d\n",
//			batmon_sensor.value(BATMON_SENSOR_TYPE_TEMP),
//			(batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5);
//
//	printf("Time: %d\n", rtc_gettime( ));


	sender_data_t data;
	data.battery_mon = (batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5;
	data.ctime = rtc_gettime();
	data.adc[0] = 10;
	data.adc[1] = 20;
	data.adc[2] = 30;
	data.adc[3] = 40;

	sender_send_data(&data);


}


void sensor_reset_timer( )
{
	// update the schedule
	scifScsTaskData.analogSensor.cfg.scheduleDelay = config_get_sensor_interval();
	sensor_aux_init();
	scifScsTaskData.analogSensor.cfg.scheduleDelay = config_get_sensor_interval();
}
