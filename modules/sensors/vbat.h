/*
 * vbat.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_VBAT_H_
#define MODULES_SENSORS_VBAT_H_


#include <stdint.h>

uint32_t vbat_read( );
uint32_t vbat_millivolts(uint32_t reading);

#endif /* MODULES_SENSORS_VBAT_H_ */
