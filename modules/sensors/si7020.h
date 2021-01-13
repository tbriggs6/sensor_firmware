/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_SI7020_H_
#define MODULES_SENSORS_SI7020_H_

#include <stdint.h>
#include <contiki.h>

typedef struct {
	bool rc;
	uint32_t humidity;
	uint32_t temperature;
} si7020_data_t;

PROCESS_NAME(si7020_proc);

#endif /* MODULES_SENSORS_SI7020_H_ */
