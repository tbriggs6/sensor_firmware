/*
 * tcs3472.h
 *
 *  Created on: Jan 6, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_TCS3472_H_
#define MODULES_SENSORS_TCS3472_H_

typedef struct {
	uint16_t red, green, blue, clear;
} color_t;


int tcs3472_read (color_t *color);

#endif /* MODULES_SENSORS_TCS3472_H_ */
