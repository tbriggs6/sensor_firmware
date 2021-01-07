/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */
#include <contiki.h>
#include <sys/clock.h>
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"

#include <stdio.h> /* For printf() */
#include <sys/energest.h>
#include "sys/log.h"

#include "../modules/config/config.h"
#include "../modules/echo/echo.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/command.h"
#include "sys/log.h"
#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG
#include "../modules/command/message.h"


#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_DBG

static int sequence = 0;

#define TRY_I2C
#ifndef TRY_I2C
void send_fake_cal( )
{
	static uip_ip6addr_t addr;
	static airborne_cal_t cal_data;

	config_get_receiver (&addr);

	memset(&cal_data, 0, sizeof(cal_data));
	cal_data.sequence = sequence++;
	cal_data.header = AIRBORNE_CAL_HEADER;

	// send data
	messenger_send (&addr, (void *) &cal_data, sizeof(cal_data));
}

void send_fake_data( )
{
	static uip_ip6addr_t addr;
	static airborne_t data;

	config_get_receiver (&addr);

	memset(&data, 0, sizeof(data));
	data.sequence = sequence++;
	data.header = AIRBORNE_HEADER;

	// send data
	messenger_send (&addr, (void *) &data, sizeof(data));

}

#else

#include <Board.h>
#include <dev/i2c-arch.h>

#include "../modules/sensors/ms5637.h"
#include <string.h>
#include "../modules/sensors/vaux.h"
#include "../modules/sensors/si7020.h"
#include "../modules/sensors/analog.h"
void send_fake_cal( )
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

void send_fake_data( )
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



#endif





/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  static struct etimer timer;
  static uip_ip6addr_t addr;
  static int send_cal_data = 1;

  int send_len, recv_len, status;

  PROCESS_BEGIN();
  tsch_set_coordinator(0);

  energest_init( );
  NETSTACK_MAC.on();

  // let things settle....
  etimer_set(&timer, CLOCK_SECOND );

  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  config_init( 000 );
  messenger_init();
  echo_init();
  command_init( );


    uiplib_ip6addrconv("fd00::1", &addr);
    config_set_receiver(&addr);

  LOG_INFO("Stored destination address: ");
  config_get_receiver (&addr);
  LOG_6ADDR(LOG_LEVEL_INFO, &addr);
  LOG_INFO_("\n");


  etimer_set(&timer, 60 * CLOCK_SECOND );
  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));



    if (send_cal_data == 1) {
      printf("Sending calibration data\n");
    	send_fake_cal( );
    }
    else {
    	printf("Sending calibration data\n");
    	send_fake_data( );
    }

    // start the wait loop
    etimer_set(&timer, 30*CLOCK_SECOND);

    while(1) {
    	PROCESS_WAIT_EVENT();
    	printf("Event received: %d\n", ev);

    	if (etimer_expired(&timer)) {
    		printf("Timeout waiting for response\n");
    		break;
    	}
    	else if (ev == PROCESS_EVENT_MSG) {
    		printf("Received message send event\n");


    		messenger_get_last_result(&send_len, &recv_len, sizeof(status), &status);
    		printf("Result: send: %d, recv: %d, status: %d\n", send_len, recv_len, status);
    		if (status >= 0) {
    			if (send_cal_data == 1) {
    				printf("Calibration data was received, starting to send actual data\n");
    				send_cal_data = 0;
    			}
    		}
    		break;
    	}
    }

    // wait for the next timer.
    etimer_set(&timer, 60 * CLOCK_SECOND );
  }

  PROCESS_END();
}

