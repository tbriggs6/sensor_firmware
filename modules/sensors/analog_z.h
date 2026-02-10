/*
 * Analog inputs (vbat, thermistor, conductivity) - Zephyr driver.
 * API aligned with Contiki analog.h for compatibility.
 */
#ifndef MODULES_SENSORS_ANALOG_Z_H_
#define MODULES_SENSORS_ANALOG_Z_H_

#include <stdint.h>

/**
 * Initialize ADC for vbat and thermistor. Call once before reads.
 * \param adc_dev Zephyr ADC device (e.g. DEVICE_DT_GET(DT_NODELABEL(adc0)))
 * \param channel_vbat ADC channel index for battery voltage
 * \param channel_therm ADC channel index for thermistor
 * \return 0 on success, negative errno on failure
 */
int analog_z_init(const void *adc_dev, uint8_t channel_vbat, uint8_t channel_therm);

/**
 * Read battery voltage ADC count (raw). Same semantics as Contiki vbat_read().
 */
uint32_t analog_z_vbat_read(void);

/**
 * Convert raw reading to millivolts (optional scaling).
 */
uint32_t analog_z_vbat_millivolts(uint32_t reading);

/**
 * Read thermistor ADC count (raw). Same semantics as Contiki thermistor_read().
 */
uint32_t analog_z_thermistor_read(void);

/**
 * Read conductivity range channels (e.g. 5 ranges). Fills range[0..4] with raw ADC.
 * Pass NULL or 0 len if not used. Requires analog_z_init_conductivity() with channel list.
 */
int analog_z_conductivity_read(uint16_t *range, int len);

#endif /* MODULES_SENSORS_ANALOG_Z_H_ */
