/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_SI7210_H_
#define MODULES_SENSORS_SI7210_H_

#include <stdint.h>

typedef struct {
	uint16_t config_range; // OTP compensation range
} si7210_calibration_t;

int si7210_read_cal(si7210_calibration_t *cal);

int si7210_read(int16_t *magfield);



#endif /* MODULES_SENSORS_SI7020_H_ */
