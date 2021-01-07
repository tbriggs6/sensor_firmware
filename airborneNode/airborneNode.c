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
 * Implement main logic loop for the airborne sensors for the
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
#include "../modules/sensors/ms5637.h"
#include "../modules/sensors/vaux.h"
#include "../modules/sensors/si7020.h"
#include "../modules/sensors/analog.h"

#include "devtype.h"

#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_SENSOR

// used to track the current message number
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
	// calibration message sent to the server
	airborne_cal_t message;

	// MS5637 pressure sensor data
	ms5637_caldata_t data;

	// enable the auxillary voltage bus
	vaux_enable ();

	// read the calibration values from the MS5637 sensor
	if (ms5637_readcalibration_data (&data) == 0) {
		printf ("Error - could not read cal data, aborting.\n");
		return;
	}

	// we are done with the auxillary bus
	vaux_disable ();

	// prepare the calibration message
	memset (&message, 0, sizeof(message));

	message.header = AIRBORNE_CAL_HEADER;
	message.sequence = sequence++;

	// order matters
	message.caldata[0] = data.sens;
	message.caldata[1] = data.off;
	message.caldata[2] = data.tcs;
	message.caldata[3] = data.tco;
	message.caldata[4] = data.tref;
	message.caldata[5] = data.temp;

	// emit some debugging information to the serial console
	LOG_INFO("**************************\n");
	LOG_INFO("* Cal Data - %6.4u      *\n", (unsigned int ) message.sequence);
	LOG_INFO("* TCO: %-6.4u           *\n", (unsigned int ) data.tco);
	LOG_INFO("* TCS: %-6.4u           *\n", (unsigned int ) data.tcs);
	LOG_INFO("* Off: %-6.4u           *\n", (unsigned int ) data.off);
	LOG_INFO("* Sens: %-6.4u          *\n", (unsigned int ) data.sens);
	LOG_INFO("* Temp: %-6.4u          *\n", (unsigned int ) data.temp);
	LOG_INFO("**************************\n");

	// get the configured address of the server
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	// request the messenger to start sending the message
	// the messenger process will take it from here, sending asynchrously
	// and posting a status message when its done
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
	// the message to be sent to the server
	airborne_t message;

	// values read from the sensors
	uint32_t pressure = 0, temperature = 0;
	uint32_t si7020_humid = 0, si7020_temp = 0;

	// turn on the vaux bus to enable sensors
	vaux_enable ();

	// read the pressure sensor & temperature
	if (ms5637_read_pressure (&pressure) == 0) {
		LOG_ERR("Error - could not read pressure data, aborting.\n");
	}

	if (ms5637_read_temperature (&temperature) == 0) {
		LOG_ERR("Error - could not read temperature data, aborting.\n");
	}

	// read the humditiy and temperature
	si7020_read_humidity (&si7020_humid);
	si7020_read_temperature (&si7020_temp);

	// done reading sensors, turn off the vaux bus
	vaux_disable ();

	// prepare the message
	memset (&message, 0, sizeof(message));
	message.header = AIRBORNE_HEADER;
	message.sequence = sequence++;
	message.ms5637_temp = temperature;
	message.ms5637_pressure = pressure;
	message.si7020_humid = si7020_humid;
	message.si7020_temp = si7020_temp;
	message.battery = vbat_millivolts (vbat_read ());

	LOG_INFO("************************************\n");
	LOG_INFO("* Data -    seq: %10u    *\n", (unsigned int ) message.sequence);
	LOG_INFO("* Pressure   : %10u      *\n", (unsigned int ) message.ms5637_pressure);
	LOG_INFO("* Temperature: %10u      *\n", (unsigned int ) message.ms5637_temp);
	LOG_INFO("* Humidity   : %10u      *\n", (unsigned int ) message.si7020_humid);
	LOG_INFO("* Temperature: %10u      *\n", (unsigned int ) message.si7020_temp);
	LOG_INFO("* Battery    : %10u      *\n", (unsigned int ) message.battery);
	LOG_INFO("***********************************\n");

	// send the message
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	// post the message to the messenger service, it will take over sending
	// the message and posting a result asynchronously
	messenger_send (&addr, (void*) &message, sizeof(message));

}

// // // // // // // // // // // // // // // // // // // // // // //
/*---------------------------------------------------------------------------*/
PROCESS(airborne_process, "Airborne process");
AUTOSTART_PROCESSES(&airborne_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(airborne_process, ev, data)
{
	// event timer - used to configure data intervals and retransmits
	static struct etimer timer =	{ 0 };

	// the IPv6 address of the server, usually fd00::1
	static uip_ip6addr_t serverAddr =	{ 0 };

	// failure  counter
	static int failure_counter = 0;

	// state machine control
	static enum sender_states
	{
		INIT, SEND_CAL, WAIT_OK_CAL, SEND_DATA, WAIT_OK_DATA
	} state = INIT;

	// begin the airborne contiki process
	PROCESS_BEGIN();

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
		config_init ( AIRBORNE_SENSOR_DEVTYPE);
		config_get_receiver (&serverAddr);

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
				// initial state for the state machine - set the timeout for sending calibration data
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
						LOG_DBG("unexpected event received... %d", ev);
					}
					break;

					// wait for the send to finish - retry on timeout
				case WAIT_OK_CAL:

					// messenger responded...
					if (ev == PROCESS_EVENT_MSG) {

						// sending data was OK (server said so!)
						if (messenger_last_result_okack ()) {
							LOG_INFO("calibration data sent OK\n");
							leds_single_off (LEDS_CONF_GREEN);

							// get ready to enter the normal send data loop
							failure_counter = 0;
							state = SEND_DATA;
						}
					}
					// server didn't respond before the timer expired :-(
					else if (etimer_expired (&timer)) {

						// increment failure counter
						failure_counter++;
						LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);

						// if too many in a row, reboot the system to hopefully fix the problem
						if (failure_counter >= config_get_maxfailures ()) {
							LOG_ERR("too many consecutive failures, need to restart system\n");
							leds_single_on (LEDS_CONF_RED);

							// use the watchdog to reboot the system
							watchdog_reboot ();
						}
						// otherwise, just resend
						else {
							// resending ....
							state = SEND_CAL;
							process_poll (&airborne_process);
						}
					}
					break;


				// sending data
				case SEND_DATA:

					if (etimer_expired (&timer)) {
						// trigger reading sensors and adding a message to the outgoing queue
						send_sensor_data ();

						// set the retry / timeout interval
						etimer_set (&timer, config_get_retry_interval () * CLOCK_CONF_SECOND);

						// enter the wait for ACK state
						state = WAIT_OK_DATA;
						leds_single_on (LEDS_CONF_GREEN);
					}
					break;

					// awaitng the response from the server or a timeout
				case WAIT_OK_DATA:

					// messenger responded...
					if (ev == PROCESS_EVENT_MSG) {

						// its good!  2 points for the field goal!
						if (messenger_last_result_okack ()) {
							LOG_INFO("sensor data sent OK\n");

							leds_single_off (LEDS_CONF_GREEN);

							// get ready for next sensor push
							failure_counter = 0;
							state = SEND_DATA;

							// use the configuration sensor interval to select the next clock time
							etimer_set (&timer, config_get_sensor_interval () * CLOCK_SECOND);

						}
					}

					// its not good!
					else if (etimer_expired (&timer)) {

						failure_counter++;
						LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int ) failure_counter);

						// check to see if we've had a bad run
						if (failure_counter >= config_get_maxfailures ()) {

							// fire the kicker
							LOG_ERR("too many consecutive failures, need to restart system\n");
							leds_single_on (LEDS_CONF_RED);

							// use the watchdog to reboot
							watchdog_reboot ();
						}

						// the occaisonal failure happens, its OK
						else {
							// resending ....
							state = SEND_DATA;
							process_poll (&airborne_process);
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

