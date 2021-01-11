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


#define CLOCK_TIME_MS( x ) \
	((x <= (1000 / CLOCK_SECOND)) ? 1 : ((x * CLOCK_SECOND) / 1000))


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

static PT_THREAD(ms5637_cvt_and_read (struct pt *pt, ms5637_data_t *data, enum sampletype type))
{
	static uint8_t address = DEVICE_ADDR;
	static uint8_t regnum = 0;
	static uint8_t bytes[3] = { 0 };
	static I2C_Handle handle = NULL;
	static struct etimer timer = { 0 };
	static uint32_t value = 0;

	PT_BEGIN(pt);

	handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		data->status = false;
		PT_EXIT(pt);
	}

	regnum = (type == PRESSURE) ? CMD_START_PRESS : CMD_START_TEMP;

	if (!i2c_arch_write (handle, address, &regnum, 1)) {
		data->status = false;
		PT_EXIT(pt);
	}


	i2c_arch_release(handle);



	// delay for at least 3 ms
	etimer_set(&timer,(clock_time_t) CLOCK_TIME_MS(3));
	PT_WAIT_UNTIL(pt, etimer_expired(&timer));

	handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		data->status = false;
		PT_EXIT(pt);
	}

	regnum = REG_DATA;

	if (!i2c_arch_write_read (handle, address, &regnum, 1, &bytes, 3)) {
		data->status = false;
		PT_EXIT(pt);
	}

	i2c_arch_release(handle);

	value = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];

	if (type == PRESSURE) {
		data->pressure = value;
	}
	else if (type == TEMP) {
		data->temperature = value;
	}

	PT_END(pt);
}


/** Threaded worker to read settings **/
PROCESS(ms5637_proc,"MS5637 Sensor");
PROCESS_THREAD(ms5637_proc, ev, data)
{
	static struct pt child = { 0 };
	static ms5637_data_t *mdata = 0;
	PROCESS_BEGIN( );

	mdata = (ms5637_data_t *) data;
	mdata->status = true;
	PROCESS_PT_SPAWN(&child, ms5637_cvt_and_read(&child, mdata, PRESSURE));
	PROCESS_PT_SPAWN(&child, ms5637_cvt_and_read(&child, mdata, TEMP));

	PROCESS_END( );
}
