/*
 * Daylight LED control - Zephyr GPIO driver.
 * Same API as Contiki daylight.h for compatibility.
 */
#ifndef MODULES_SENSORS_DAYLIGHT_Z_H_
#define MODULES_SENSORS_DAYLIGHT_Z_H_

/**
 * Initialize daylight LED GPIO. Call once before enable/disable.
 * \param gpio_dev Zephyr GPIO device
 * \param pin GPIO pin number for daylight LED (e.g. 3 on CC13xx)
 * \return 0 on success, negative errno on failure
 */
int daylight_z_init(const void *gpio_dev, uint32_t pin);

void daylight_z_enable(void);
void daylight_z_disable(void);

#endif /* MODULES_SENSORS_DAYLIGHT_Z_H_ */
