#ifndef SENSOR_MS5637_H
#define SENSOR_MS5637_H

#include <contiki.h>
#include <stdint.h>

typedef struct {
	uint16_t sens;
	uint16_t off;
	uint16_t tcs;
	uint16_t tco;
	uint16_t tref;
	uint16_t temp;
} ms5637_caldata_t;

typedef struct {
	int status;
	uint32_t pressure;
	uint32_t temperature;
} ms5637_data_t;

int ms5637_readcalibration_data(ms5637_caldata_t *caldata);

PROCESS_NAME(ms5637_proc);


#endif
