/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_PIC32DRVR_H_
#define MODULES_SENSORS_PIC32DRVR_H_

#include <stdint.h>

typedef struct {
	uint16_t completed;
	uint16_t range[5];
} conductivity_t;

int pic32drvr_read(conductivity_t *conduct);


#endif /* MODULES_SENSORS_SI7020_H_ */
