/*
 *
 * Copyright (c) 2020, Thomas Briggs
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \addtogroup sensorNodes
 * @{
 *
 * \file
 * Implement main logic loop for the water sensors for the
 * water quality sensor project (variously known as ICON, ANCHOR, ...).
 *
 * This is the version implemented around the new CC1352-based modules / carrier
 * boards.  This is also the first production version to eliminate the need for
 * sensor controller studio.  Finally, it is also the first production version using
 * the new Contiki-NG + TSCH radio scheduling.
 */
#include <contiki.h>
#include <string.h>

//#include <net/mac/tsch/tsch.h>
#include <dev/leds.h>
#include <sys/energest.h>

#include "../modules/config/config.h"
#include "../modules/echo/echo.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/message.h"
#include "../modules/command/command.h"
#include "../modules/sensors/analog.h"
#include "../modules/sensors/daylight.h"
#include "../modules/sensors/ms5637.h"
#include "../modules/sensors/pic32drvr.h"
#include "../modules/sensors/si7210.h"
#include "../modules/sensors/tcs3472.h"
#include "../modules/sensors/vaux.h"
#include "../modules/sensors/sensors.h"
#include "devtype.h"

#include "config_nvs.h"
#include "config.h"

#define LOG_MODULE "H20"
#define LOG_LEVEL LOG_LEVEL_DBG

static int sequence = 0;
static int failure_counter = 0;

static int sensor_done_evt = 0;
PROCESS_NAME(sensor_process);


static volatile int red = 1;
static volatile int green = 0;

PROCESS(sysmon, "System Monitor");
PROCESS_THREAD(sysmon, ev, data)
{
	static struct etimer et = { 0 };

	PROCESS_BEGIN( );

	etimer_set(&et, CLOCK_SECOND / 2);
	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		if (red == 1) {
			leds_single_toggle(LEDS_RED);
		}
		else {
			leds_single_off(LEDS_RED);
		}


		if (green == 1) {
			leds_single_toggle(LEDS_GREEN);
		}
		else {
			leds_single_off(LEDS_GREEN);
		}


		if (failure_counter >= config_get_maxfailures()) {
			watchdog_reboot();
			leds_single_on(LEDS_RED);
		}

		etimer_set(&et, CLOCK_SECOND / 2);
	}

	PROCESS_END();
}


/**
 * \brief sends calibration data to the server
 *
 * Each device contains unique calibration constants that are either
 * stored in the individual sensors are part of the non-volatile configuration
 * memory.  Whenever the device starts up it will send the current calibration
 * values.
 *
 * The server stores the calibration data and will connect the new data messages
 * to the most recent calibration check in.  Thus, we *should* send calibration
 * data whenever there is a change in the calibration data.  For instance, if
 * the non-volatile configuration parameters are changed.
 *
 *TODO implement sending calibration data when configuration changes
 */

PROCESS(send_cal_proc, "Send Data");
PROCESS_THREAD(send_cal_proc, ev, data)
{
	// calibration message to send to server
	static water_cal_t message = { 0 };

	// MS5637 pressure sensor data
	static ms5637_caldata_t mcal =	{ 0 };

	// Si7210 calibration data
	static si7210_calibration_t scal = { 0 };

	// event timer for delays
	static struct etimer et = { 0 };

	// result
	static bool rc = false;

	PROCESS_BEGIN( );

	vaux_enable ();

	// delay to let everything settle.
	etimer_set(&et, 1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	if (ms5637_readcalibration_data (&mcal) == 0) {
		LOG_ERR("Error - ms5637 could not read cal data, aborting.\n");
	}

	if (si7210_read_cal(&scal) == 0) {
		LOG_ERR("Error - si7210 could not read Si7210 cal data\n");
	}

	vaux_disable ();

	memset (&message, 0, sizeof(message));
	message.header = WATER_CAL_HEADER;
	message.sequence = sequence++;
	message.rssi = messenger_recvd_rssi();

	message.caldata[0] = mcal.sens;
	message.caldata[1] = mcal.off;
	message.caldata[2] = mcal.tcs;
	message.caldata[3] = mcal.tco;
	message.caldata[4] = mcal.tref;
	message.caldata[5] = mcal.temp;

	message.resistorVals[0] = config_get_calibration (0);  // si7210 compensation range
	message.resistorVals[1] = config_get_calibration (1);
	message.resistorVals[2] = config_get_calibration (2);
	message.resistorVals[3] = config_get_calibration (3);
	message.resistorVals[4] = config_get_calibration (4);
	message.resistorVals[5] = config_get_calibration (5);
	message.resistorVals[6] = config_get_calibration (6);
	message.resistorVals[7] = config_get_calibration (7);


		message.si7210_gain = scal.config_range;
		message.si7210_offset = scal.config_range;



	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_INFO("**************************\n");
		LOG_INFO("* Cal Data - %6.4u      *\n", (unsigned int ) message.sequence);
		LOG_INFO("* Sens: %-6.4u          *\n", (unsigned int ) mcal.sens);
		LOG_INFO("* Off: %-6.4u           *\n", (unsigned int ) mcal.off);
		LOG_INFO("* TCO: %-6.4u           *\n", (unsigned int ) mcal.tco);
		LOG_INFO("* TCS: %-6.4u           *\n", (unsigned int ) mcal.tcs);
		LOG_INFO("* Tref: %-6.4u          *\n", (unsigned int ) mcal.tref);
		LOG_INFO("* Temp: %-6.4u          *\n", (unsigned int ) mcal.temp);
		LOG_INFO("**************************\n");
	}

	// dispatch the message to the message queue
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	messenger_send (&addr, message.sequence, (void*) &message, sizeof(message));
	etimer_set(&et, config_get_retry_interval() * CLOCK_SECOND);


	while(1) {
		PROCESS_WAIT_EVENT();



		// messenger responded...
		if (ev == PROCESS_EVENT_MSG) {

			LOG_DBG("Found event message\n");

			if (messenger_last_result_okack ()) {
				LOG_INFO("calibration data sent OK\n");

				failure_counter = 0;
				config_clear_calbration_changed( );
				rc = true;
			}
			// calibration did not send
			else {
				LOG_INFO("calibration data sent, NAK\n");
				failure_counter++;
			}

			break;
		}

		else if (etimer_expired(&et)) {
			failure_counter++;
			LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);
			break;
		}

		else {
			LOG_DBG("Unknown event, ignoring\n");
			continue;
		}

	} // end while ... event processing loop

	LOG_DBG("Cal process is done.\n");
	process_post(&sensor_process, sensor_done_evt, &rc);

	PROCESS_END( );
}


/*---------------------------------------------------------------------------*/


/**
 * \brief read sensors and send data to server
 *
 * Read from the integrated sensors and report their values to the server.
 * This function should be called periodically to report sensor values.
 */
/*---------------------------------------------------------------------------*/
PROCESS(send_data_proc, "Send Data");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_data_proc, ev, data)
{
	PROCESS_BEGIN( );

	// the message to send to the server
	static struct etimer et = { 0 };
  static unsigned int completions = 0;
  static volatile ms5637_data_t mdata = { 0 };
	static volatile si7210_data_t sdata = { 0 };
	static volatile tcs3472_data_t cdata = { 0 };
	static volatile conductivity_t pdata = { 0 } ;
	static water_data_t message = { 0 };

	static clock_t start = 0;
	static clock_t now = 0;

	static uip_ip6addr_t addr;
	static bool rc = false;


	// start preparing the message to send
	memset (&message, 0, sizeof(message));
	message.header = WATER_DATA_HEADER;
	message.sequence = sequence++;
	message.rssi = messenger_recvd_rssi();

	// turn on the auxillary bus
	vaux_enable ();
	daylight_enable();

	// 1 period = 1/32 of a second
	etimer_set(&et, 1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  start = clock_time( );

  completions = 0;

  // start the sensor readings
  process_start(&ms5637_proc, (void *)&mdata);
  completions |= (1 << 0);

  process_start(&si7210_proc, (void *)&sdata);
  completions |= (1 << 1);

  process_start(&tcs3472_proc, (void *)&cdata);
  completions |= (1 << 2);

  process_start(&pic32_proc, (void *) &pdata);
  completions |= (1 << 3);

  etimer_set(&et, CLOCK_SECOND);

  while (completions != 0) {
  	PROCESS_YIELD( );
  	if ((completions & (1 << 0)) && !process_is_running(&ms5637_proc)) {
  		completions &= ~(1 << 0);
  		now = clock_time();

  		message.pressure = mdata.pressure;
  		message.temppressure = mdata.temperature;

  		LOG_DBG("Pressure: %d, Temp: %d\n", (int) message.pressure, (int) message.temppressure);

  	  LOG_DBG("ms5637 %lu ticks, still running: %x\n", (now-start), completions);
  	}

  	if ((completions & (1 << 1)) && !process_is_running(&si7210_proc)) {
  		completions &= ~(1 << 1);
  		now = clock_time();

  		message.hall = sdata.magfield;

  		LOG_DBG("si7210 %lu ticks, still running: %x\n", (now-start), completions);
  	}
  	if ((completions & (1 << 2)) && !process_is_running(&tcs3472_proc)) {
  		completions &= ~(1 << 2);
  		now = clock_time();

  		message.color_red = cdata.red;
  		message.color_green = cdata.green;
  		message.color_blue = cdata.blue;
  		message.color_clear = cdata.clear;

  		LOG_DBG("tcs3472 %lu ticks, still running: %x\n", (now-start), completions);
  	}

  	if ((completions & (1 << 3)) && !process_is_running(&pic32_proc)) {
    	completions &= ~(1 << 3);
    	now = clock_time();

    	message.range1 = pdata.range[0];
    	message.range2 = pdata.range[1];
    	message.range3 = pdata.range[2];
    	message.range4 = pdata.range[3];
    	message.range5 = pdata.range[4];

    	LOG_DBG("pic32 %lu ticks, still running: %x\n", (now-start), completions);
    }

  	if (etimer_expired(&et)) {
  		etimer_set(&et, CLOCK_SECOND);
  		LOG_WARN("Sensor timeout...\n");
  		if (completions & (1 << 0)) LOG_WARN("ms5637 still running: %d\n", process_is_running(&ms5637_proc));
  		if (completions & (1 << 1)) LOG_WARN("si7210 still running: %d\n", process_is_running(&si7210_proc));
  		if (completions & (1 << 2)) LOG_WARN("tcs3472 still running: %d\n", process_is_running(&tcs3472_proc));
  		if (completions & (1 << 3)) LOG_WARN("pic32 still running: %d\n", process_is_running(&pic32_proc));
  	}
  }

  now = clock_time( );


	daylight_disable ();

	// There are pending events -- the previous loop could end in the main exec thread
	// and if the sending process hasn't finished cleaning up.... race conditions abound.

	etimer_set(&et, 3);
	while(! etimer_expired(&et)) {
		PROCESS_WAIT_EVENT();
	}

	// capture ambient light
	memset((void *) &cdata, 0, sizeof(cdata));
	process_start(&tcs3472_proc, (void *)&cdata);

	message.battery = vbat_read( );
	message.temperature = thermistor_read( );

	while(process_is_running(&tcs3472_proc)) {
		PROCESS_YIELD();
	}

	message.ambient = cdata.clear;

	vaux_disable ();

	// this is the end, display stuff
	LOG_INFO("***********  WATER SENSOR *********\n");
	LOG_INFO("* Data Pkt   seq: %10u      *\n", (unsigned int ) message.sequence);
	LOG_INFO("* Battery       : %10u      *\n", (unsigned int ) message.battery);

	LOG_INFO("* Ranges                          *\n");
	LOG_INFO("*            1] : %10u      *\n", (unsigned int ) message.range1);
	LOG_INFO("*            2] : %10u      *\n", (unsigned int ) message.range2);
	LOG_INFO("*            3] : %10u      *\n", (unsigned int ) message.range3);
	LOG_INFO("*            4] : %10u      *\n", (unsigned int ) message.range4);
	LOG_INFO("*            5] : %10u      *\n", (unsigned int ) message.range5);

	LOG_INFO("* Thermistor    : %10u      *\n", (unsigned int ) message.temperature);
	LOG_INFO("* Hall          : %10u      *\n", (unsigned int) message.hall);
	LOG_INFO("* Color                           *\n");
	LOG_INFO("*           Red : %10u      *\n", (unsigned int ) message.color_red);
	LOG_INFO("*         Green : %10u      *\n", (unsigned int ) message.color_green);
	LOG_INFO("*          Blue : %10u      *\n", (unsigned int ) message.color_blue);
	LOG_INFO("*         Clear : %10u      *\n", (unsigned int ) message.color_clear);
	LOG_INFO("* Ambient       : %10u      *\n", (unsigned int ) message.ambient);
	LOG_INFO("* Pressure      : %10u      *\n", (unsigned int ) message.pressure);
	LOG_INFO("* Internal Temp : %10u      *\n", (unsigned int ) message.temppressure);
	LOG_INFO("***********************************\n");

	config_get_receiver (&addr);

	// dispatch the message to the messenger service for delivery
	green = 1;
	messenger_send (&addr, message.sequence, (void*) &message, sizeof(message));


	while(1) {
		PROCESS_WAIT_EVENT();

		// messenger responded...
		if (ev == PROCESS_EVENT_MSG) {


			if (messenger_last_result_okack ()) {
				LOG_INFO("sensor data sent OK\n");
				failure_counter = 0;
				rc = true;
				config_clear_calbration_changed( );
			}
			// calibration did not send
			else {
				LOG_INFO("sensor data sent, NAK\n");
				failure_counter++;
				rc = false;
			}

			break;
		}

		else if (etimer_expired(&et)) {
			failure_counter++;
			LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);
			rc = false;
			break;
		}

		else {
			LOG_DBG("Unknown event, ignoring\n");
			continue;
		}

	} // end while ... event processing loop

	// report our success
	process_post(&sensor_process, sensor_done_evt, &rc);

PROCESS_END( );
}



/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Water process");
AUTOSTART_PROCESSES(&sensor_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_process, ev, data)
{
	// event timer - used to configure data intervals and retransmits
	static struct etimer timer = { 0 };

	// result from the previous send
	static bool result = false;

	PROCESS_BEGIN()	;

	// this node is NOT a coordinator (only the spinet / router is)
	//tsch_set_coordinator (0);

	// initialize the energest module
	energest_init ();

	// enable the radio MAC
	NETSTACK_MAC.on ();

	// let things settle for 1 second.
	etimer_set (&timer, CLOCK_SECOND);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired (&timer));

	sensor_done_evt = process_alloc_event();

	sensors_init();

	// radio should have begun initialization in the background

	// initialize the configuration module - used for non-volatile config
	nvs_init( );

	config_init ( WATER_SENSOR_DEVTYPE);

	// initialize the messenger service - used for bidirectional comms with server
	messenger_init ();

	// enable the "echo" service - a test service used for diagnostics
	echo_init ();

	// enable the "command" service - respond to remote requests over messenger connections
	command_init ();

	process_start(&sysmon, NULL);

	// enter the main state machine loop
	config_set_calibration_change( );

	etimer_set(&timer, config_get_sensor_interval() * CLOCK_SECOND);

	while (1) {

		PROCESS_WAIT_EVENT( );
		LOG_DBG("event: %d cal change? %d expire? %d\n", ev, config_did_calibration_change(), etimer_expired(&timer));
		if (ev == config_cmd_run || config_did_calibration_change() || etimer_expired(&timer)) {

			green = 1;

			if (config_did_calibration_change()) {
				process_start(&send_cal_proc, (void *) &result);

				PROCESS_WAIT_EVENT_UNTIL(ev == sensor_done_evt);
				result = *(bool *) data;

				LOG_DBG("Previous cal send result: %d\n", result);
				if (result == true) {
					etimer_set(&timer, config_get_sensor_interval() * CLOCK_SECOND);
					red = 0;
					green = 0;
				}
				else {
					etimer_set(&timer, config_get_retry_interval() * CLOCK_SECOND);
					green = 0;
					red = 1;
				}
				config_clear_calbration_changed();
			}
			else {
				process_start(&send_data_proc, (void *) result);

				PROCESS_WAIT_EVENT_UNTIL(ev == sensor_done_evt);
				result = *(bool *) data;

				LOG_DBG("Previous data send result: %d\n", result);
				if (result == true) {
					etimer_set(&timer, config_get_sensor_interval() * CLOCK_SECOND);
					green = 0;
					red = 0;
				}
				else {
					etimer_set(&timer, config_get_retry_interval() * CLOCK_SECOND);
					green = 0;
					red = 1;
				}
			}
		}
	} // end while loop;

	LOG_ERR("This while loop must never end, something went wrong, rebooting.\n");
	watchdog_reboot ();
	PROCESS_END();

}


void config_timeout_change( )
{
	process_post(&sensor_process, config_cmd_run, 0);
}
