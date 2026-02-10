/*
 * AUX power control (vaux enable/disable) - Zephyr GPIO driver.
 * Same API as Contiki vaux.h for compatibility.
 */
#ifndef MODULES_SENSORS_VAUX_Z_H_
#define MODULES_SENSORS_VAUX_Z_H_

/**
 * Initialize AUX power GPIO. Call once before enable/disable.
 * \param gpio_dev Zephyr GPIO device
 * \param pin GPIO pin number for AUX enable (e.g. 29 on CC13xx)
 * \return 0 on success, negative errno on failure
 */
int vaux_z_init(const void *gpio_dev, uint32_t pin);

void vaux_z_enable(void);
void vaux_z_disable(void);

#endif /* MODULES_SENSORS_VAUX_Z_H_ */
