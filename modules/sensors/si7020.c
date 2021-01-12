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
#include "sensors.h"

#define LOG_MODULE "Si7020"
#define LOG_LEVEL LOG_LEVEL_DBG

#define DEVICE_ADDR 0x40

#define I2CBUS Board_I2C0

#define CLOCK_TIME_MS( x ) \
	((x <= (1000 / CLOCK_SECOND)) ? 1 : ((x * CLOCK_SECOND) / 1000))

enum si7020_cmd_type {
	HUMID, TEMP
};

static PT_THREAD(si7020_read_data(struct pt *pt, enum si7020_cmd_type type, si7020_data_t *sdata))
{
	static uint8_t addr = DEVICE_ADDR;
	static uint8_t reg = 0;
  static bool rc = true;
  static I2C_Handle handle = 0;
  static struct etimer timer = { 0 };
  static int count = 100;

  PT_BEGIN(pt);

  reg = 0;
  rc = true;
  handle = 0;
  count = 100;


  LOG_DBG("Reading: %d\n", type);

	reg = (type == HUMID) ? 0xF5 : 0xF3;

	SENSOR_AQ(pt, handle, rc);
	i2c_arch_write(handle, addr,  &reg, 1);
	SENSOR_REL(pt, handle)


	// wait until the sampling has finished...

	// poll the device until its done
	count = 100;
	uint8_t bytes[2] = { 0 };
	do {
		etimer_set (&timer, CLOCK_TIME_MS(5));
		PT_WAIT_UNTIL(pt, etimer_expired (&timer));

		SENSOR_AQ(pt, handle, rc);
		rc = i2c_arch_read(handle, addr, &bytes, 2);
		SENSOR_REL(pt, handle)

	} while ((rc == false) && (--count > 0));

	if (count == 0) {
		LOG_ERR("Error - did not get data from Si7020\n");
		sdata->rc = false;
		PT_EXIT(pt);
	}

	LOG_DBG("Completed reading from Si7020: %d\n", bytes[0] << 8 | bytes[1]);

	if (type == HUMID)
		sdata->humidity = bytes[0] << 8 | bytes[1];
	else
		sdata->temperature = bytes[0] << 8 | bytes[1];

	sdata->rc = rc;

	PT_END(pt);
}


/** Threaded worker to read settings **/
PROCESS(si7020_proc,"Si7020 Sensor");
PROCESS_THREAD(si7020_proc, ev, data)
{
	static struct pt child = { 0 };
	static si7020_data_t *sdata;

	PROCESS_BEGIN( );

	sdata = (si7020_data_t *) data;


	PROCESS_PT_SPAWN(&child, si7020_read_data(&child, HUMID, sdata));
	PROCESS_PT_SPAWN(&child, si7020_read_data(&child, TEMP, sdata));
	LOG_DBG("Si7020 finished : %d %d\n",(int) sdata->humidity, (int) sdata->temperature);

	PROCESS_END( );
}

