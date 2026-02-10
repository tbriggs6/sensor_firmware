/*
 * TCS3472 color sensor - Zephyr driver.
 * Types aligned with Contiki tcs3472.h for compatibility.
 */
#ifndef MODULES_SENSORS_TCS3472_Z_H_
#define MODULES_SENSORS_TCS3472_Z_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool rc;
	uint16_t red, green, blue, clear;
} tcs3472_data_t;

/**
 * Initialize TCS3472. Call once before sample.
 * \param i2c_dev Zephyr I2C device
 * \param addr I2C slave address (typically 0x29)
 * \return 0 on success, negative errno on failure
 */
int tcs3472_z_init(const void *i2c_dev, uint8_t addr);

/**
 * Blocking sample: enable sensor, set gain/cycles, wait integration, read RGBC.
 * gain: 0=1x, 1=4x, 2=16x, 3=60x; cycles: 0..255 for integration time.
 */
int tcs3472_z_sample(tcs3472_data_t *data, uint8_t gain, uint8_t cycles);

#endif /* MODULES_SENSORS_TCS3472_Z_H_ */
