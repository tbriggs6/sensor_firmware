/*
 * vbat.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_ANALOG_H_
#define MODULES_SENSORS_ANALOG_H_


#include <stdint.h>

uint32_t vbat_read( );
uint32_t vbat_millivolts(uint32_t reading);

uint32_t thermistor_read( );

#endif /* MODULES_SENSORS_ANALOG_H_ */
