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
#include "sensors.h"

#define LOG_MODULE "MS5637"
#define LOG_LEVEL LOG_LEVEL_SENSOR

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
	static uint8_t address = DEVICE_ADDR;
	static uint8_t regnum = 0;
	static bool rc = true;
	static uint8_t vals[2];

	memset(caldata, 0, sizeof(ms5637_caldata_t));

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("Failed to acquire I2C Bus\n");
		return 0;
	}

	rc = true;

	regnum = REG_SENS;
	rc &= i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->sens = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read SENS\n");


	regnum = REG_OFF;
	rc &= i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->off = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read OFF\n");

	regnum = REG_TCS;
	rc &= i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->tcs = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read TCS\n");

	regnum = REG_TCO;
	rc&= i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->tco = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read TCO\n");

	regnum = REG_TREF;
	rc &=i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->tref = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read TREF\n");

	regnum = REG_TEMP;
	rc &= i2c_arch_write_read (handle, address, &regnum, 1, &vals, 2);
	caldata->temp = vals[0] << 8 | vals[1];
	if (!rc) LOG_ERR("could not read TEMP\n");
	i2c_arch_release(handle);

	return rc;
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

	LOG_DBG("starting %d\n", type);

	data->status = true;
	regnum = (type == PRESSURE) ? CMD_START_PRESS : CMD_START_TEMP;

	SENSOR_AQ(pt, handle, data->status);
	data->status &= i2c_arch_write (handle, address, &regnum, 1);
	SENSOR_REL(pt, handle);

	if (data->status == false) {
		LOG_ERR("Could not read sensor\n");
		PT_EXIT(pt);
	}

	LOG_DBG("Setting timeout\n");

	// delay for at least 3 ms
	etimer_set(&timer,(clock_time_t) CLOCK_TIME_MS(3));
	PT_WAIT_UNTIL(pt, etimer_expired(&timer));

	LOG_DBG("Reading data for type %d\n", type);

	regnum = REG_DATA;
	SENSOR_AQ(pt, handle, data->status);
	data->status = i2c_arch_write_read (handle, address, &regnum, 1, &bytes, 3);
	SENSOR_REL(pt, handle);

	if (!data->status) {
		LOG_ERR("Could not read sensor\n");
		PT_EXIT(pt);
	}

	value = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];

	LOG_DBG("Setting value in %p for type %d = %d\n", data, type, (int) value);

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
	static ms5637_data_t *mdata;

	PROCESS_BEGIN( );

	mdata = (ms5637_data_t *) data;

	LOG_DBG("MS5637 staring %p\n", data);
	PROCESS_PT_SPAWN(&child, ms5637_cvt_and_read(&child, mdata, PRESSURE));
	PROCESS_PT_SPAWN(&child, ms5637_cvt_and_read(&child, mdata, TEMP));

		LOG_DBG("MS5637 finished %p\n", data);
	PROCESS_END( );
}
