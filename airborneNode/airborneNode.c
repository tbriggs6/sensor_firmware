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
#include <sys/clock.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)


#include "dev/leds.h"
#include <stdio.h> /* For printf() */
#include <sys/energest.h>
#include "sys/log.h"

#include "../modules/config/config.h"
#include "../modules/echo/echo.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/command.h"


#include "sys/log.h"
#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_SENSOR


#include "../modules/command/message.h"

static int sequence = 0;

#include <Board.h>
#include <dev/i2c-arch.h>

#include "../modules/sensors/ms5637.h"
#include <string.h>
#include "../modules/sensors/vaux.h"
#include "../modules/sensors/si7020.h"
#include "../modules/sensors/vbat.h"


void send_calibration_data( )
{
	//static uip_ip6addr_t addr;
	airborne_cal_t message;
	ms5637_caldata_t data;

	vaux_enable();

	if (ms5637_readcalibration_data(&data) == 0) {
		printf("Error - could not read cal data, aborting.\n");
		return;
	}

	if (sizeof(message.caldata)!=sizeof(data)) {
		printf("Warning - datatype mismatch between message and sensor cal data.\n");
	}

	memset(&message, 0, sizeof(message));
	message.header = AIRBORNE_CAL_HEADER;
	message.sequence = sequence++;

	message.caldata[0] = data.sens;
	message.caldata[1] = data.off;
	message.caldata[2] = data.tcs;
	message.caldata[3] = data.tco;
	message.caldata[4] = data.tref;
	message.caldata[5] = data.temp;


	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		printf("**************************\n");
		printf("* Cal Data - %6.4u      *\n", (unsigned int) message.sequence);
		printf("* TCO: %-6.4u           *\n", (unsigned int) data.tco);
		printf("* TCS: %-6.4u           *\n", (unsigned int) data.tcs);
		printf("* Off: %-6.4u           *\n", (unsigned int) data.off);
		printf("* Sens: %-6.4u          *\n", (unsigned int) data.sens);
		printf("* Temp: %-6.4u          *\n", (unsigned int) data.temp);
		printf("**************************\n");
	}


	static uip_ip6addr_t addr;
	config_get_receiver (&addr);

	messenger_send (&addr, (void *) &message, sizeof(message));

	vaux_disable();
}

void send_sensor_data( )
{
	//static uip_ip6addr_t addr;
		airborne_t message;
		uint32_t pressure =  0, temperature = 0;
		uint32_t si7020_humid = 0, si7020_temp = 0;

		vaux_enable();
		if (ms5637_read_pressure(&pressure) == 0) {
			printf("Error - could not read pressure data, aborting.\n");
			return;
		}

		if (ms5637_read_temperature(&temperature) == 0) {
					printf("Error - could not read temperature data, aborting.\n");
					return;
				}

		si7020_read_humidity(&si7020_humid);
		si7020_read_temperature(&si7020_temp);

		memset(&message, 0, sizeof(message));
		message.header = AIRBORNE_HEADER;
		message.sequence = sequence++;
		message.ms5637_temp = temperature;
		message.ms5637_pressure = pressure;
		message.si7020_humid = si7020_humid;
		message.si7020_temp = si7020_temp;
		message.battery = vbat_millivolts( vbat_read() );
//		message.battery = vbat_read( );

		if (LOG_LEVEL >= LOG_LEVEL_DBG) {
				printf("************************************\n");
				printf("* Data -    seq: %10u    *\n", (unsigned int) message.sequence);
				printf("* Pressure   : %10u      *\n", (unsigned int) message.ms5637_pressure);
				printf("* Temperature: %10u      *\n", (unsigned int) message.ms5637_temp);
				printf("* Humidity   : %10u      *\n", (unsigned int) message.si7020_humid);
				printf("* Temperature: %10u      *\n", (unsigned int) message.si7020_temp);
				printf("* Battery    : %10u      *\n", (unsigned int) message.battery);
				printf("***********************************\n");
			}

		static uip_ip6addr_t addr;
		config_get_receiver (&addr);

		messenger_send (&addr, (void *) &message, sizeof(message));

		vaux_disable();
}




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
  static enum sender_states {
  	INIT, SEND_CAL, WAIT_OK_CAL, SEND_DATA, WAIT_OK_DATA
  } state = INIT;

  // begin the airborne contiki process

  PROCESS_BEGIN();

  // this node is NOT a coordinator (only the spinet / router is)
  tsch_set_coordinator(0);

  // initialize the energest module
  energest_init( );

  // enable the radio MAC
  NETSTACK_MAC.on();

  // let things settle for 1 second.
  etimer_set(&timer, CLOCK_SECOND );
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));


  // radio should have begun initialization in the background

  // initialize the configuration module - used for non-volatile config
  config_init( );
  config_get_receiver (&serverAddr);

  if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		LOG_INFO("Stored destination address: ");
		LOG_6ADDR(LOG_LEVEL_INFO, &serverAddr);
		LOG_INFO_("\n");
  }



  // initialize the messenger service - used for bidirectional comms with server
  messenger_init();

  // enable the "echo" service - a test service used for diagnostics
  echo_init();

  // enable the "command" service - respond to remote requests over messenger connections
  command_init( );


  // enter the main state machine loop

  while(1) {


  	PROCESS_WAIT_EVENT( );
  	switch(state) {
  		case INIT:
  			etimer_set(&timer, 15*CLOCK_SECOND);
  			state = SEND_CAL;


  		// send calibration data, retry every 15 seconds until success / ACK by remote
  		case SEND_CAL:
  			if (etimer_expired(&timer)) {
  				send_calibration_data();
  				etimer_set(&timer, config_get_retry_interval() * CLOCK_CONF_SECOND);
  				state = WAIT_OK_CAL;
  				leds_single_on(LEDS_CONF_GREEN);
  			}
  			else {
  				LOG_DBG("unexpected event received... %d", ev);
  			}
  			break;

  		// wait for the send to finish - retry on timeout
  		case WAIT_OK_CAL:
  			// messenger responded...
  			if (ev == PROCESS_EVENT_MSG) {
  				if (messenger_last_result_okack()) {
  					LOG_INFO("calibration data sent OK\n");

  					leds_single_off(LEDS_CONF_GREEN);

  					failure_counter = 0;
  					state = SEND_DATA;
  				}
  			}
  			else if (etimer_expired(&timer)) {
  				failure_counter++;
  				LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int) failure_counter);

  				if (failure_counter >= config_get_maxfailures()) {
  					LOG_ERR("too many consecutive failures, need to restart system\n");
  					leds_single_on(LEDS_CONF_RED);
  					SysCtrlSystemReset();
  				}
  				else {
  					// resending ....
  					state = SEND_CAL;
  					process_poll(&airborne_process);
  				}
  			}
  			break;



  		case SEND_DATA:
  			if (etimer_expired(&timer)) {
  				send_sensor_data();

  				etimer_set(&timer, config_get_retry_interval() * CLOCK_CONF_SECOND);

  				state = WAIT_OK_DATA;
  				leds_single_on(LEDS_CONF_GREEN);
  			}
  			else {
  				LOG_DBG("unexpected event received... %d", ev);
  			}
  			break;


  		case WAIT_OK_DATA:
  			// messenger responded...
				if (ev == PROCESS_EVENT_MSG) {
					if (messenger_last_result_okack()) {
						LOG_INFO("sensor data sent OK\n");

						leds_single_off(LEDS_CONF_GREEN);

						// get ready for next sensor push
						failure_counter = 0;
						state = SEND_DATA;
						etimer_set(&timer, config_get_sensor_interval() * CLOCK_SECOND);

					}
				}

				else if (etimer_expired(&timer)) {
					failure_counter++;
					LOG_DBG("timeout waiting for remote, failures: %u\n", (unsigned int) failure_counter);

					if (failure_counter >= config_get_maxfailures()) {

						LOG_ERR("too many consecutive failures, need to restart system\n");
						leds_single_on(LEDS_CONF_RED);
						SysCtrlSystemReset();
					}

					else {
						// resending ....
						state = SEND_DATA;
						process_poll(&airborne_process);
					}
				}
				break;

  		default:
  			LOG_ERR("error - hit an invalid / unknown state in FSM, rebooting\n");
  			SysCtrlSystemReset();

  	} // end state machine


  } // end while loop;


  LOG_ERR("This while loop must never end, something went wrong, rebooting.\n");
  SysCtrlSystemReset();
  PROCESS_END();
}

