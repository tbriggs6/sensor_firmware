/*
 * SI7020 humidity/temperature sensor - Zephyr driver.
 * Types aligned with Contiki si7020.h for compatibility.
 */
#ifndef MODULES_SENSORS_SI7020_Z_H_
#define MODULES_SENSORS_SI7020_Z_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool rc;
	uint32_t humidity;
	uint32_t temperature;
} si7020_data_t;

/**
 * Initialize SI7020. Call once before sample.
 * \param i2c_dev Zephyr I2C device
 * \param addr I2C slave address (typically 0x40)
 * \return 0 on success, negative errno on failure
 */
int si7020_z_init(const void *i2c_dev, uint8_t addr);

/**
 * Blocking sample: measure humidity then temperature; fills data.
 */
int si7020_z_sample(si7020_data_t *data);

#endif /* MODULES_SENSORS_SI7020_Z_H_ */
