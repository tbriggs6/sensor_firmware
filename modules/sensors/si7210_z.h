/*
 * SI7210 hall effect sensor - Zephyr driver.
 * Types aligned with Contiki si7210.h for compatibility.
 */
#ifndef MODULES_SENSORS_SI7210_Z_H_
#define MODULES_SENSORS_SI7210_Z_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint16_t config_range;
} si7210_calibration_t;

typedef struct {
	bool rc;
	uint16_t magfield;
} si7210_data_t;

/**
 * Initialize SI7210. Call once before read_cal or sample.
 * \param i2c_dev Zephyr I2C device
 * \param addr I2C slave address (typically 0x32)
 * \return 0 on success, negative errno on failure
 */
int si7210_z_init(const void *i2c_dev, uint8_t addr);

/**
 * Read calibration (config_range). May use config module for stored value.
 */
int si7210_z_read_cal(si7210_calibration_t *cal);

/**
 * Blocking sample: read magnetic field; fills data->magfield and data->rc.
 */
int si7210_z_sample(si7210_data_t *data);

#endif /* MODULES_SENSORS_SI7210_Z_H_ */
