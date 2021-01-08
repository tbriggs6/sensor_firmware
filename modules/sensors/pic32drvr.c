/*
 * si7020.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#include "pic32drvr.h"
#include <contiki.h>

#include "../modules/command/message.h"
#include <Board.h>
#include <dev/i2c-arch.h>
#include "sys/log.h"

#define LOG_MODULE "PIC32DRVR"
#define LOG_LEVEL LOG_LEVEL_SENSOR

#define DEVICE_ADDR 0x24
#define I2CBUS Board_I2C0

#define RETRY_COUNT 32

int pic32drvr_read(conductivity_t *conduct)
{
	uint8_t bytes_in[2] = { 0 };
	uint8_t bytes_out[2] = { 0 };

	unsigned int count = 0;
	unsigned int i = 0;
	int rc = 0;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("could not acquire i2c handle...\n");
		return 0;
	}

	count = RETRY_COUNT;
	do {
		clock_delay_usec(10000);

		bytes_out[0] = 0;
		bytes_in[0] = 0;
		bytes_in[1] = 0;

		rc = i2c_arch_write_read(handle, DEVICE_ADDR, &bytes_out, 1, &bytes_in, 2);
		if (rc == false)
			continue;

		uint16_t value = bytes_in[0] << 8 | bytes_in[1];
		if (value > 0) {
			LOG_DBG("Found %d completions\n", value);
			break;
		}
	} while (--count > 0);

	if (count == 0) {
		LOG_ERR("did not get a completion in %d attempts\n", RETRY_COUNT);
		i2c_arch_release(handle);
		return 0;
	}


	for (i = 0; i < 4; i++) {
		bytes_out[0] = (i+1) * 2;

		rc = i2c_arch_write_read(handle, DEVICE_ADDR, &bytes_out, 1,&bytes_in, 2);
		if (rc == 0) {
			LOG_ERR("failed reading range %d\n", i);
			break;
		}

		conduct->range[i] = bytes_in[0] << 8 | bytes_in[1];
	}


	i2c_arch_release(handle);
	return rc;
}

