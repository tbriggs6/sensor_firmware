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

	PROCESS_BEGIN( );

	cdata = (tcs3472_data_t *) data;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("could not acquiare I2C bus\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}



	// turn on the device
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x01;
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}

	i2c_arch_release(handle);

	// delay 2400 usec for the device to turn on
	etimer_set(&timer, CLOCK_TIME_MS(3));
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));


	handle = i2c_arch_acquire (I2CBUS);
		if (handle == NULL) {
			LOG_ERR("could not acquire I2C bus\n");
			cdata->rc = false;
			PROCESS_EXIT();
		}


	// set gain
	bytes_out[0] = 0xa0 | 0x0f;

	// gain values are:  0 = 1x, 1= 4x, 2=16x, 3 = 60x
	bytes_out[1] = config_get_calibration(1);
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		i2c_arch_release(handle);
		PROCESS_EXIT();
	}

	// set accumumation time
	bytes_out[0] = 0xa0 | 0x01;
	// time C0 = max 65535, but takes 154ms
	//bytes_out[1] = 0xf0;
	cycles = config_get_calibration(2);
	bytes_out[1] = cycles;

	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		i2c_arch_release(handle);
		PROCESS_EXIT();
	}

	// enable conversion
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x03;
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		cdata->rc = false;
		i2c_arch_release(handle);
		PROCESS_EXIT();
	}


	i2c_arch_release(handle);


	// poll the status
	count = 100;
	done = 0;

	while (done == 0) {

		etimer_set(&timer, CLOCK_TIME_MS(3));
		PROCESS_WAIT_EVENT_UNTIL(etimer_expiration_time(&timer));

		handle = i2c_arch_acquire (I2CBUS);
			if (handle == NULL) {
				LOG_ERR("could not acquiare I2C bus\n");
				PROCESS_EXIT();
			}

		count = count - 1;

		bytes_out[0] = 0xa0 | 0x13;
		rc = i2c_arch_write_read(handle,  ADDR, &bytes_out, 1, &bytes_in, 1);
		if (rc == false)
			continue;


		i2c_arch_release(handle);

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


	handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("could not acquiare I2C bus\n");
		cdata->rc = false;
		PROCESS_EXIT();
	}

	// read the four values
	bytes_out[0] = 0xa0 | 0x14;
	rc = i2c_arch_write_read(handle, ADDR, &bytes_out, 1, &bytes_in, 8);
	if (rc == false) {
		LOG_ERR("could not read color values\n");
		i2c_arch_release(handle);
		cdata->rc = false;
		PROCESS_EXIT();
	}

	i2c_arch_release(handle);

	cdata->clear = bytes_in[1] << 8 | bytes_in[0];
	cdata->red = bytes_in[3] << 8 | bytes_in[2];
	cdata->green = bytes_in[5] << 8 | bytes_in[4];
	cdata->blue = bytes_in[7] << 8 | bytes_in[6];
	cdata->rc = rc;

	PROCESS_END();
}
