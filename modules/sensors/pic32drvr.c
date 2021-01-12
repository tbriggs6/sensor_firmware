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
#include "sensors.h"

#define LOG_MODULE "PIC32DRVR"
#define LOG_LEVEL LOG_LEVEL_SENSOR

#define DEVICE_ADDR 0x24
#define I2CBUS Board_I2C0

#define RETRY_COUNT 32

#define CLOCK_TIME_MS( x ) \
	((x <= (1000 / CLOCK_SECOND)) ? 1 : ((x * CLOCK_SECOND) / 1000))

PROCESS(pic32_proc, "PIC32 Sensor");

PROCESS_THREAD(pic32_proc, ev, data)
{
	static uint8_t bytes_in[2] = { 0 };
	static uint8_t bytes_out[2] =	{ 0 };
	static unsigned int count = 0;
	static unsigned int i = 0;
	static I2C_Handle handle = 0;
	static struct etimer timer = { 0 };
	static conductivity_t *conduct = 0;
	static bool rc = true;
	static uint16_t value = 0;

	PROCESS_BEGIN( );

	count = RETRY_COUNT;
	LOG_DBG("Starting pic32 loop to wait for completions\n");
	conduct = (conductivity_t *) data;

	do {
		etimer_set (&timer, CLOCK_TIME_MS(5));
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired (&timer));


		bytes_out[0] = 0;
		bytes_in[0] = 0;
		bytes_in[0] = 0;

		SENSOR_AQ(&pic32_proc.pt, handle, rc);
		rc = i2c_arch_write_read (handle, DEVICE_ADDR, &bytes_out, 1, &bytes_in, 2);
		SENSOR_REL(&pic32_proc.pt, handle)

		if (rc == false)
			continue;

		value = bytes_in[0] << 8 | bytes_in[1];

		if (value > 0) {
			LOG_DBG("Found %d completions\n", value);
			break;
		}
	}
	while (--count > 0);

	LOG_DBG("PIC32 completions %d, count %d\n", value, count);

	if (count == 0) {
		LOG_ERR("did not get a completion in %d attempts\n", RETRY_COUNT);
		conduct->rc = false;
		PROCESS_EXIT();
	}


	LOG_DBG("pic32 reading out data values into %p\n", data);

		for (i = 0; i < 4; i++) {
		bytes_out[0] = (i + 1) * 2;

		SENSOR_AQ(&pic32_proc.pt, handle, ((conductivity_t*) data)->rc);
		rc = i2c_arch_write_read (handle, DEVICE_ADDR, &bytes_out, 1, &bytes_in, 2);
		SENSOR_REL(&pic32_proc.pt, handle);

		if (rc == 0) {
			conduct->rc = false;
			PROCESS_EXIT();
		}

		conduct->range[i] = bytes_in[0] << 8 | bytes_in[1];
		LOG_DBG("%d = %d\n", i, conduct->range[i]);
	}

	conduct->rc = rc;

	LOG_DBG("pic32 done\n");
	PROCESS_END( );
}

