/*
 * tcs3472.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */
#include <contiki.h>
#include "tcs3472.h"

#include "../modules/command/message.h"
#include <Board.h>
#include <dev/i2c-arch.h>
#include "sys/log.h"
#include "tcs3472.h"

#include "config.h"
#include "sensors.h"

#define LOG_MODULE "TCS3472"
#define LOG_LEVEL LOG_LEVEL_SENSOR

#define ADDR 0x29
#define I2CBUS Board_I2C0


#define CLOCK_TIME_MS( x ) \
	((x <= (1000 / CLOCK_SECOND)) ? 1 : ((x * CLOCK_SECOND) / 1000))


PROCESS(tcs3472_proc,"TCS3472 Sensor");
PROCESS_THREAD(tcs3472_proc,ev, data)
{
	static uint8_t bytes_out[4] = { 0 };
	static uint8_t bytes_in[8] = { 0 } ;
	static uint16_t count, done = 0;
	static bool rc = true;
	static struct etimer timer = { 0 };
	static unsigned int cycles = 0;
	static tcs3472_data_t *cdata = 0;
	static I2C_Handle handle;

	PROCESS_BEGIN( );

	count = 0;
	done = 0;
	rc = true;
	cycles = 0;

	cdata = (tcs3472_data_t *) data;

	LOG_DBG("Starting process %p\n", cdata);
	// turn on the device
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x01;

	SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc)
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	SENSOR_REL(&tcs3472_proc.pt, handle);

	// this is frustrating the second read for ambient light.
//	if (rc == false) {
//		LOG_ERR("could not set tcs3472 register\n");
//		cdata->rc = false;
//		PROCESS_EXIT();
//	}

	// delay 2400 usec for the device to turn on
	etimer_set(&timer, CLOCK_TIME_MS(3));
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	// set gain
	bytes_out[0] = 0xa0 | 0x0f;

	SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc)

	// gain values are:  0 = 1x, 1= 4x, 2=16x, 3 = 60x
	bytes_out[1] = config_get_calibration(1);
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	SENSOR_REL(&tcs3472_proc.pt, handle);

	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}

	// set accumumation time
	bytes_out[0] = 0xa0 | 0x01;
	// time C0 = max 65535, but takes 154ms
	//bytes_out[1] = 0xf0;

	cycles = config_get_calibration(2);
	bytes_out[1] = cycles;

	SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc)
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	SENSOR_REL(&tcs3472_proc.pt, handle);

	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}

	// enable conversion
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x03;

	SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc)
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	SENSOR_REL(&tcs3472_proc.pt, handle);

	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		SENSOR_REL(&tcs3472_proc.pt, handle);
		PROCESS_EXIT();
	}


	// poll the status
	count = 100;
	done = 0;

	while (done == 0) {

		etimer_set(&timer, CLOCK_TIME_MS(3));
		PROCESS_WAIT_EVENT_UNTIL(etimer_expiration_time(&timer));

		count = count - 1;
		bytes_out[0] = 0xa0 | 0x13;

		SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc)
		rc = i2c_arch_write_read(handle,  ADDR, &bytes_out, 1, &bytes_in, 1);
		SENSOR_REL(&tcs3472_proc.pt, handle);

		if (rc == false)
			continue;


		if (bytes_in[0] & 0x01) {
			done = 1;
		}

		if (count == 0) {
			done = 2;
		}
	}

	if (done == 2) {
		LOG_ERR("did not get finished status in 1000 attempts.\n");

		PROCESS_EXIT();
	}

	// read the four values
	bytes_out[0] = 0xa0 | 0x14;
	SENSOR_AQ(&tcs3472_proc.pt, handle, cdata->rc);
	rc = i2c_arch_write_read(handle, ADDR, &bytes_out, 1, &bytes_in, 8);
	SENSOR_REL(&tcs3472_proc.pt, handle);

	if (rc == false) {
		LOG_ERR("could not read color values\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}

	cdata->clear = bytes_in[1] << 8 | bytes_in[0];
	cdata->red = bytes_in[3] << 8 | bytes_in[2];
	cdata->green = bytes_in[5] << 8 | bytes_in[4];
	cdata->blue = bytes_in[7] << 8 | bytes_in[6];
	cdata->rc = rc;


	LOG_DBG("Finished reading values < %d, %d, %d, %d> into %p\n", cdata->clear, cdata->red, cdata->green, cdata->blue, cdata);

	PROCESS_END();
}
