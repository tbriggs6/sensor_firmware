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
#include "../modules/sensors/sensors.h"
#include "devtype.h"

#include "config_nvs.h"
#include "config.h"

#define LOG_MODULE "AIR"
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
/*---------------------------------------------------------------------------*/
PROCESS(send_sensor_proc, "Send Data");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_sensor_proc, ev, data)
{
	PROCESS_BEGIN( );

	// the message to send to the server
	static struct etimer et = { 0 };
  static unsigned int completions = 0;
  static volatile ms5637_data_t mdata = { 0 };
  static volatile si7020_data_t sdata = { 0 };
	static airborne_t message = { 0 };

	static clock_t start = 0;
	static clock_t now = 0;

	// start preparing the message to send
	memset (&message, 0, sizeof(message));
	message.header = AIRBORNE_HEADER;
	message.sequence = sequence++;

	// turn on the auxillary bus
	vaux_enable ();

	// 1 period = 1/32 of a second
	etimer_set(&et, 1);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  start = clock_time( );

  completions = 0;

  // start the sensor readings
  process_start(&ms5637_proc, (void *)&mdata);
  completions |= (1 << 0);

  process_start(&si7020_proc, (void *)&sdata);
  completions |= (1 << 1);


  etimer_set(&et, CLOCK_SECOND);

  while (completions != 0) {
  	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT || etimer_expired(&et));

  	if ((completions & (1 << 0)) && !process_is_running(&ms5637_proc)) {
  		completions &= ~(1 << 0);
  		now = clock_time();

  		message.ms5637_pressure = mdata.pressure;
  		message.ms5637_temp = mdata.temperature;
  	}

  	if ((completions & (1 << 1)) && !process_is_running(&si7020_proc)) {
  		completions &= ~(1 << 1);
  		now = clock_time();

  		message.si7020_humid = sdata.humidity;
  		message.si7020_temp = sdata.temperature;

  		LOG_DBG("si7010 %lu ticks, still running: %x\n", (now-start), completions);
  	}

  	if (etimer_expired(&et)) {
  		etimer_set(&et, CLOCK_SECOND);
  		LOG_WARN("Sensor timeout...\n");
  		if (completions & (1 << 0)) LOG_WARN("ms5637 still running: %d\n", process_is_running(&ms5637_proc));
  		if (completions & (1 << 1)) LOG_WARN("si7010 still running: %d\n", process_is_running(&si7020_proc));
  	}
  }

  now = clock_time( );

	message.battery = vbat_read( );

	vaux_disable ();

		LOG_INFO("************************************\n");
		LOG_INFO("* Data -    seq: %10u    *\n", (unsigned int ) message.sequence);
		LOG_INFO("* Pressure   : %10u      *\n", (unsigned int ) message.ms5637_pressure);
		LOG_INFO("* Temperature: %10u      *\n", (unsigned int ) message.ms5637_temp);
		LOG_INFO("* Humidity   : %10u      *\n", (unsigned int ) message.si7020_humid);
		LOG_INFO("* Temperature: %10u      *\n", (unsigned int ) message.si7020_temp);
		LOG_INFO("* Battery    : %10u      *\n", (unsigned int ) message.battery);
		LOG_INFO("***********************************\n");

	// dispatch the message to the messenger service for delivery
	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	messenger_send (&addr, (void*) &message, sizeof(message));

PROCESS_END( );
}




// // // // // // // // // // // // // // // // // // // // // // //
/*---------------------------------------------------------------------------*/
PROCESS(airborne_process, "Airborne process");
AUTOSTART_PROCESSES(&airborne_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(airborne_process, ev, data)
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


		sensors_init();

		// radio should have begun initialization in the background

		// initialize the configuration module - used for non-volatile config
		nvs_init( );

		config_init ( AIRBORNE_SENSOR_DEVTYPE );
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
					if (etimer_expired (&timer) || (ev == PROCESS_EVENT_POLL)) {
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
					else if (etimer_expired (&timer) || (ev == PROCESS_EVENT_POLL)) {
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
							process_poll (&airborne_process);
						}
					}
					break;

				case SEND_DATA:
					if (etimer_expired (&timer) || (ev == PROCESS_EVENT_POLL)) {

						if (config_did_calibration_change())
						{
							send_calibration_data();
							state = WAIT_OK_CAL;
						}
						else {
							process_start(&send_sensor_proc, NULL);
							state = WAIT_OK_DATA;
						}

						etimer_set (&timer, config_get_retry_interval () * CLOCK_CONF_SECOND);
						leds_single_on (LEDS_CONF_GREEN);
					}
	//				else {
	//					LOG_DBG("unexpected event received... %d", ev);
	//				}
					break;

				case WAIT_OK_DATA:
					// messenger responded...
					if (process_is_running(&send_sensor_proc)) {
						state = SEND_DATA;
					}

					else if (ev == PROCESS_EVENT_MSG) {
						if (messenger_last_result_okack ()) {
							LOG_INFO("sensor data sent OK\n");

							leds_single_off (LEDS_CONF_GREEN);

							// get ready for next sensor push
							failure_counter = 0;
							// resending ....
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

void config_timeout_change( )
{
	process_poll(&airborne_process);
}
