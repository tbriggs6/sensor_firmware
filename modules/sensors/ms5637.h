#ifndef SENSOR_MS5637_H
#define SENSOR_MS5637_H

#include <stdint.h>

typedef struct {
	uint16_t sens;
	uint16_t off;
	uint16_t tcs;
	uint16_t tco;
	uint16_t tref;
	uint16_t temp;
} ms5637_caldata_t;

int ms5637_readcalibration_data(ms5637_caldata_t *caldata);
int ms5637_read_pressure (uint32_t *value);
int ms5637_read_temperature (uint32_t *value);

#endif
