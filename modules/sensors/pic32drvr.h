/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_PIC32DRVR_H_
#define MODULES_SENSORS_PIC32DRVR_H_

#include <contiki.h>
#include <stdint.h>

typedef struct {
	bool rc;
	uint16_t completed;
	uint16_t range[5];
} conductivity_t;

PROCESS_NAME(pic32_proc);


#endif /* MODULES_SENSORS_SI7020_H_ */
