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

int tcs3472_read (color_t *color)
{
	uint8_t bytes_out[4];
	uint8_t bytes_in[8];
	uint16_t count, done;
	bool rc;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("could not acquiare I2C bus\n");
		return 0;
	}


	// turn on the device
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x01;
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		goto error;
	}

	// delay 2400 usec for the device to turn on
	clock_delay_usec(2400);

	// set gain
	bytes_out[0] = 0xa0 | 0x0f;

	// gain values are:  0 = 1x, 1= 4x, 2=16x, 3 = 60x
	bytes_out[1] = config_get_calibration(1);
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		goto error;
	}

	// set accumumation time
	bytes_out[0] = 0xa0 | 0x01;
	// time C0 = max 65535, but takes 154ms
	//bytes_out[1] = 0xf0;
	unsigned int cycles = config_get_calibration(2);
	bytes_out[1] = cycles;

	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		goto error;
	}

	// enable conversion
	bytes_out[0] = 0xa0;
	bytes_out[1] = 0x03;
	rc = i2c_arch_write(handle, ADDR, &bytes_out, 2);
	if (rc == false) {
		LOG_ERR("could not set tcs3472 register\n");
		goto error;
	}

	// poll the status
	count = 1000;
	done = 0;

	while (done == 0) {

		clock_delay_usec(2400 * cycles);

		count = count - 1;

		bytes_out[0] = 0xa0 | 0x13;
		rc = i2c_arch_write_read(handle,  ADDR, &bytes_out, 1, &bytes_in, 1);
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
		goto error;
	}

	// read the four values
	bytes_out[0] = 0xa0 | 0x14;
	rc = i2c_arch_write_read(handle, ADDR, &bytes_out, 1, &bytes_in, 8);
	if (rc == false) {
		LOG_ERR("could not read color values\n");
		goto error;
	}

	color->clear = bytes_in[1] << 8 | bytes_in[0];
	color->red = bytes_in[3] << 8 | bytes_in[2];
	color->green = bytes_in[5] << 8 | bytes_in[4];
	color->blue = bytes_in[7] << 8 | bytes_in[6];


error:
	i2c_arch_release(handle);
	return rc;
}
