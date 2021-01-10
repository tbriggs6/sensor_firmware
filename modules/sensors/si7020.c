/*
 * si7020.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#include "si7020.h"
#include <contiki.h>

#include "../modules/command/message.h"
#include <Board.h>
#include <dev/i2c-arch.h>

#define DEVICE_ADDR 0x40

#define I2CBUS Board_I2C0

int si7020_read_humidity(uint32_t *humidity)
{
	uint8_t addr = DEVICE_ADDR;
	uint8_t reg;

  bool result;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}


	// start a single sample
	reg = 0xF5;
	i2c_arch_write(handle, addr,  &reg, 1);

	i2c_arch_release(handle);

	// wait until the sampling has finished...

	// poll the device until its done
	int count = 1000;
	uint8_t bytes[2] = { 0 };
	do {
		clock_delay_usec(1000);

		handle = i2c_arch_acquire (I2CBUS);
			if (handle == NULL) {
				return 0;
			}

		result = i2c_arch_read(handle, addr, &bytes, 2);

		i2c_arch_release(handle);

	} while ((result == false) && (--count > 0));

	*humidity = bytes[0] << 8 | bytes[1];

	return result;
}


int si7020_read_temperature(uint32_t *temperature)
{
	uint8_t addr = DEVICE_ADDR;
	uint8_t reg;

  bool result;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}


	// start a single sample
	reg = 0xF3;
	i2c_arch_write(handle, addr,  &reg, 1);


	// wait until the sampling has finished...

	// poll the device until its done
	int count = 1000;
	uint8_t bytes[2] = { 0 };
	do {
		result = i2c_arch_read(handle, addr, &bytes, 2);
	} while ((result == false) && (--count > 0));

	*temperature = bytes[0] << 8 | bytes[1];

	i2c_arch_release(handle);
	return result;
}

