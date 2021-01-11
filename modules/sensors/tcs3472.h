/*
 * tcs3472.h
 *
 *  Created on: Jan 6, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_TCS3472_H_
#define MODULES_SENSORS_TCS3472_H_

typedef struct {
	bool rc;
	uint16_t red, green, blue, clear;
} tcs3472_data_t;

PROCESS_NAME(tcs3472_proc);

#endif /* MODULES_SENSORS_TCS3472_H_ */
