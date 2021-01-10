/*
 * ms5637.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */
#include <contiki.h>
#include "ms5637.h"

#include "../modules/command/message.h"
#include <Board.h>
#include <dev/i2c-arch.h>

#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_DBG

#define DEVICE_ADDR 0x76

#define REG_DATA 0x00
#define CMD_START_PRESS 0x44
#define CMD_START_TEMP 0x54

#define REG_SENS 0xa2
#define REG_OFF 0xa4
#define REG_TCS 0xa6
#define REG_TCO 0xa8
#define REG_TREF 0xaa
#define REG_TEMP 0xac

#define I2CBUS Board_I2C0

int ms5637_readcalibration_data (ms5637_caldata_t *caldata)
{
	uint8_t address = DEVICE_ADDR;
	uint8_t regnum = 0;

	printf("Reading calibration data\n");
	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}

	regnum = REG_SENS;
	uint8_t vals[2];
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->sens = vals[0] << 8 | vals[1];


	regnum = REG_OFF;
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->off = vals[0] << 8 | vals[1];

	regnum = REG_TCS;
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->tcs = vals[0] << 8 | vals[1];

	regnum = REG_TCO;
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->tco = vals[0] << 8 | vals[1];

	regnum = REG_TREF;
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->tref = vals[0] << 8 | vals[1];


	regnum = REG_TEMP;
	if (!i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2)) {
		return 0;
	}
	caldata->temp = vals[0] << 8 | vals[1];

	i2c_arch_release(handle);

	return 1;
}

enum sampletype { PRESSURE, TEMP };

static int ms5637_cvt_and_read (enum sampletype type, uint32_t *value)
{
	uint8_t address = DEVICE_ADDR;
	uint8_t regnum = 0;
	uint8_t bytes[3] = { 0 };


	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}

	regnum = (type == PRESSURE) ? CMD_START_PRESS : CMD_START_TEMP;

	if (!i2c_arch_write (handle, address, &regnum, 1)) {
		return 0;
	}


	i2c_arch_release(handle);


	// sleep 3 ms
	ClockP_usleep(3000);


	handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		return 0;
	}

	regnum = REG_DATA;

	if (!i2c_arch_write_read (handle, address, &regnum, 1, &bytes, 3)) {
		return 0;
	}

	i2c_arch_release(handle);

	*value = 0;
	*value = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];

	return 1;
}

int ms5637_read_pressure (uint32_t *value)
{
	return ms5637_cvt_and_read(PRESSURE, value);
}

int ms5637_read_temperature (uint32_t *value)
{
	return ms5637_cvt_and_read(TEMP, value);
}


