/*
 * si7020.h
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_SI7020_H_
#define MODULES_SENSORS_SI7020_H_

#include <stdint.h>

int si7020_read_humidity(uint32_t *humidity);
int si7020_read_temperature(uint32_t *temperature);


#endif /* MODULES_SENSORS_SI7020_H_ */
