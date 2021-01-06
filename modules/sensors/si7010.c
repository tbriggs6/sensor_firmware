/*
 * si7020.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#include "si7010.h"
#include <contiki.h>
#include "ms5637.h"

#include "../modules/command/message.h"
#include <Board.h>
#include <dev/i2c-arch.h>

#define DEVICE_ADDR 0x32

#define REG_DATA 0x00

#define I2CBUS Board_I2C0

int si7010_read (uint32_t *magfield)
{
	uint8_t addr = DEVICE_ADDR;
	uint8_t reg;
	uint8_t value;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}

	reg = 0xCD;		//  FIR filiter, 2^8 samples
	value = 0x10;
	i2c_arch_write_read (handle, addr, &reg, 1, &value, 1);

	reg = 0xC5;					// enable auto increment
	value = 0x01;
	i2c_arch_write_read (handle, addr, &reg, 1, &value, 1);
	// poll the device until its done
	int count = 1000;
	bool result;
	do {
		reg = 0xC4;
		value = 0x04;
		result = i2c_arch_write_read (handle, addr, &reg, 1, &value, 1);

	}
	while (((value & 0x80) != 0) && (--count > 0));

	reg = 0xC1;

	i2c_arch_write_read (handle, addr, &reg, 1, &value, 2);
	*magfield = value;

	return result;
}

