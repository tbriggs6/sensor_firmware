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

#include <net/mac/tsch/tsch.h>
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

#include "devtype.h"

#include "config_nvs.h"
#include "config.h"

#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_SENSOR

static int sequence = 0;

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

void send_calibration_data ()
{
	// calibration message to send to server
	water_cal_t message =
				{ 0 };

	// MS5637 pressure sensor data
	ms5637_caldata_t data =
				{ 0 };

	// Si7210 calibration data
	si7210_calibration_t si7210_cal = { 0 };

	//TODO get the remaining calibration data

	vaux_enable ();

	if (ms5637_readcalibration_data (&data) == 0) {
		LOG_ERR("Error - could not read cal data, aborting.\n");
	}

	if (si7210_read_cal(&si7210_cal) == 0) {
		LOG_ERR("Error - could not read Si7210 cal data\n");
	}

	vaux_disable ();

	memset (&message, 0, sizeof(message));
	message.header = WATER_CAL_HEADER;
	message.sequence = sequence++;

	message.caldata[0] = data.sens;
	message.caldata[1] = data.off;
	message.caldata[2] = data.tcs;
	message.caldata[3] = data.tco;
	message.caldata[4] = data.tref;
	message.caldata[5] = data.temp;

	message.resistorVals[0] = config_get_calibration (0);  // si7210 compensation range
	message.resistorVals[1] = config_get_calibration (1);
	message.resistorVals[2] = config_get_calibration (2);
	message.resistorVals[3] = config_get_calibration (3);
	message.resistorVals[4] = config_get_calibration (4);
	message.resistorVals[5] = config_get_calibration (5);
	message.resistorVals[6] = config_get_calibration (6);
	message.resistorVals[7] = config_get_calibration (7);

	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_INFO("**************************\n");
		LOG_INFO("* Cal Data - %6.4u      *\n", (unsigned int ) message.sequence);
		LOG_INFO("* TCO: %-6.4u           *\n", (unsigned int ) data.tco);
		LOG_INFO("* TCS: %-6.4u           *\n", (unsigned int ) data.tcs);
		LOG_INFO("* Off: %-6.4u           *\n", (unsigned int ) data.off);
		LOG_INFO("* Sens: %-6.4u          *\n", (unsigned int ) data.sens);
		LOG_INFO("* Temp: %-6.4u          *\n", (unsigned int ) data.temp);
		LOG_INFO("**************************\n");
	}

	// dispatch the message to the message queue
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	messenger_send (&addr, (void*) &message, sizeof(message));
}

/**
 * \brief read sensors and send data to server
 *
 * Read from the integrated sensors and report their values to the server.
 * This function should be called periodically to report sensor values.
 */
void send_sensor_data ()
{
	// the message to send to the server
	water_data_t message;

	// values read from sensors
	uint32_t pressure = 0, temperature = 0;
	conductivity_t conduct =
				{ 0 };
	color_t color =
				{ 0 };

	// start preparing the message to send
	memset (&message, 0, sizeof(message));
	message.header = WATER_DATA_HEADER;
	message.sequence = sequence++;

	// turn on the auxillary bus
	vaux_enable ();

	// add a 50ms delay to give everything a chance to settle
	clock_delay_usec (50000);

	// read pressure & pressure from ms5637
	if (ms5637_read_pressure (&pressure) == 1) {
		message.pressure = pressure;
	}
	else {
		LOG_ERR("Error - could not read pressure data\n");
	}

	if (ms5637_read_temperature (&temperature) == 1) {
		message.temppressure = temperature;
	}
	else {
		LOG_ERR("Error - could not read temperature data.\n");
	}

	// read battery level
	message.battery = vbat_millivolts (vbat_read ());

	// read hall sensor
	int16_t magfield;
	if (si7210_read (&magfield) == 1) {
		message.hall = (int16_t) magfield;
	}
	else {
		LOG_ERR("Error - could not read depth / hall sensor\n");
	}

	// read conductivity
	if (pic32drvr_read (&conduct) == 1) {
		message.range1 = conduct.range[0];
		message.range2 = conduct.range[1];
		message.range3 = conduct.range[2];
		message.range4 = conduct.range[3];
		message.range5 = conduct.range[4];
	}
	else {
		LOG_ERR("Error - could not read conductivity data\n");
	}

	// read color sensor, delay 5ms to stablize
	daylight_enable ();
	clock_delay_usec (5000);

	// read color sensor value
	if (tcs3472_read (&color) == 1) {
		message.color_red = color.red;
		message.color_green = color.green;
		message.color_blue = color.blue;
		message.color_clear = color.clear;
	}
	else {
		LOG_ERR("Error - could not read from color sensor\n");
	}

	daylight_disable ();

	// read color again with light turned of to get ambient / "clear" value
	if (tcs3472_read (&color) == 1) {
		message.ambient = color.clear;
	}
	else {
		LOG_ERR("Error - could not read from color sensor (again)\n");
	}

	// read thermistor
	message.temperature = thermistor_read ();

	daylight_disable ();
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

	LOG_INFO("* Color                           *\n");
	LOG_INFO("*           Red : %10u      *\n", (unsigned int ) message.color_red);
	LOG_INFO("*         Green : %10u      *\n", (unsigned int ) message.color_green);
	LOG_INFO("*          Blue : %10u      *\n", (unsigned int ) message.color_blue);
	LOG_INFO("*         Clear : %10u      *\n", (unsigned int ) message.color_clear);
	LOG_INFO("* Ambient       : %10u      *\n", (unsigned int ) message.ambient);
	LOG_INFO("* Pressure      : %10u      *\n", (unsigned int ) message.pressure);
	LOG_INFO("* Internal Temp : %10u      *\n", (unsigned int ) message.temppressure);
	LOG_INFO("***********************************\n");

	// dispatch the message to the messenger service for delivery
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	messenger_send (&addr, (void*) &message, sizeof(message));
}

/*---------------------------------------------------------------------------*/
PROCESS(water_process, "Water process");
AUTOSTART_PROCESSES(&water_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(water_process, ev, data)
{
	// event timer - used to configure data intervals and retransmits
	static struct etimer timer = { 0 };

	// the IPv6 address of the server, usually fd00::1
	static uip_ip6addr_t serverAddr = { 0 };

	// failure  counter
	static int failure_counter = 0;

	// state machine control
	static enum sender_states
	{
		INIT, SEND_CAL, WAIT_OK_CAL, SEND_DATA, WAIT_OK_DATA
	} state = INIT;

	// begin the contiki process

	PROCESS_BEGIN()	;

	// this node is NOT a coordinator (only the spinet / router is)
	tsch_set_coordinator (0);

	// initialize the energest module
	energest_init ();

	// enable the radio MAC
	NETSTACK_MAC.on ();

	// let things settle for 1 second.
	etimer_set (&timer, CLOCK_SECOND);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired (&timer));

	// radio should have begun initialization in the background

	// initialize the configuration module - used for non-volatile config
	nvs_init( );

	config_init ( WATER_SENSOR_DEVTYPE);
	config_get_receiver (&serverAddr);

	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_INFO("Stored destination address: ");
		LOG_6ADDR(LOG_LEVEL_INFO, &serverAddr);
		LOG_INFO_("\n");
	}

	// initialize the messenger service - used for bidirectional comms with server
	messenger_init ();

	// enable the "echo" service - a test service used for diagnostics
	echo_init ();

	// enable the "command" service - respond to remote requests over messenger connections
	command_init ();

	// enter the main state machine loop
	while (1) {

		PROCESS_WAIT_EVENT();

		switch (state)
			{
			case INIT:
				etimer_set (&timer, 15 * CLOCK_SECOND);
				state = SEND_CAL;

			// send calibration data, retry every 15 seconds until success / ACK by remote
			case SEND_CAL:
				if (etimer_expired (&timer)) {
					send_calibration_data ();
					etimer_set (&timer, config_get_retry_interval () * CLOCK_CONF_SECOND);
					state = WAIT_OK_CAL;
					leds_single_on (LEDS_CONF_GREEN);
				}
				else {
					//LOG_DBG("unexpected event received... %d", ev);
				}
				break;

				// wait for the send to finish - retry on timeout
			case WAIT_OK_CAL:
				// messenger responded...
				if (ev == PROCESS_EVENT_MSG) {
					if (messenger_last_result_okack ()) {
						LOG_INFO("calibration data sent OK\n");

						leds_single_off (LEDS_CONF_GREEN);

						failure_counter = 0;
						config_clear_calbration_changed( );
						state = SEND_DATA;
					}
				}
				else if (etimer_expired (&timer)) {
					failure_counter++;
					LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);

					if (failure_counter >= config_get_maxfailures ()) {
						LOG_ERR("too many consecutive failures, need to restart system\n");
						leds_single_on (LEDS_CONF_RED);
						watchdog_reboot ();
					}
					else {
						// resending ....
						state = SEND_CAL;
						process_poll (&water_process);
					}
				}
				break;

			case SEND_DATA:
				if (etimer_expired (&timer)) {
					send_sensor_data ();

					etimer_set (&timer, config_get_retry_interval () * CLOCK_CONF_SECOND);

					state = WAIT_OK_DATA;
					leds_single_on (LEDS_CONF_GREEN);
				}
//				else {
//					LOG_DBG("unexpected event received... %d", ev);
//				}
				break;

			case WAIT_OK_DATA:
				// messenger responded...
				if (ev == PROCESS_EVENT_MSG) {
					if (messenger_last_result_okack ()) {
						LOG_INFO("sensor data sent OK\n");

						leds_single_off (LEDS_CONF_GREEN);

						// get ready for next sensor push
						failure_counter = 0;
						state = SEND_DATA;
						etimer_set (&timer, config_get_sensor_interval () * CLOCK_SECOND);

					}
				}

				else if (etimer_expired (&timer)) {
					failure_counter++;
					LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);

					if (failure_counter >= config_get_maxfailures ()) {

						LOG_ERR("too many consecutive failures, need to restart system\n");
						leds_single_on (LEDS_CONF_RED);
						watchdog_reboot ();
					}

					else {
						// resending ....
						if (config_did_calibration_change())
							state = SEND_CAL;
						else
							state = SEND_DATA;
						process_poll (&water_process);
					}
				}
				break;

			default:
				LOG_ERR("error - hit an invalid / unknown state in FSM, rebooting\n");
				watchdog_reboot ();

			} // end state machine

	} // end while loop;

	LOG_ERR("This while loop must never end, something went wrong, rebooting.\n");
	watchdog_reboot ();
	PROCESS_END();
}

