/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_SI7210_H_
#define MODULES_SENSORS_SI7210_H_

#include <contiki.h>
#include <stdint.h>

typedef struct {
	uint16_t config_range; // OTP compensation range
} si7210_calibration_t;

typedef struct {
	bool rc;
	uint16_t magfield;
} si7210_data_t;

int si7210_read_cal(si7210_calibration_t *cal);

PROCESS_NAME(si7210_proc);



#endif /* MODULES_SENSORS_SI7020_H_ */
