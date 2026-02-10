/*
 * MS5637 pressure/temperature sensor - Zephyr driver.
 * API and types aligned with Contiki ms5637.h for compatibility.
 */
#ifndef MODULES_SENSORS_MS5637_Z_H_
#define MODULES_SENSORS_MS5637_Z_H_

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

/**
 * Initialize MS5637. Call once before read_calibration_data or sample.
 * \param i2c_dev Zephyr I2C device (e.g. DEVICE_DT_GET(DT_NODELABEL(i2c0)))
 * \param addr I2C slave address (typically 0x76)
 * \return 0 on success, negative errno on failure
 */
int ms5637_z_init(const void *i2c_dev, uint8_t addr);

/**
 * Read calibration words from PROM. Same semantics as Contiki ms5637_readcalibration_data.
 */
int ms5637_z_read_calibration_data(ms5637_caldata_t *caldata);

/**
 * Blocking sample: start pressure conversion, wait, read; then temperature.
 * Fills data->pressure and data->temperature; data->status 1 on success, 0 on failure.
 */
int ms5637_z_sample(ms5637_data_t *data);

#endif /* MODULES_SENSORS_MS5637_Z_H_ */
