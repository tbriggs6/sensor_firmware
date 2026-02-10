/*
 * Daylight LED control - Zephyr GPIO implementation.
 */
#include "daylight_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(daylight_z, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *daylight_gpio_dev;
static gpio_pin_t daylight_pin = 3;
static bool daylight_ready;

int daylight_z_init(const void *gpio_dev, uint32_t pin)
{
	int ret;

	daylight_gpio_dev = (const struct device *)gpio_dev;
	daylight_pin = (gpio_pin_t)pin;
	daylight_ready = (daylight_gpio_dev != NULL && device_is_ready(daylight_gpio_dev));
	if (!daylight_ready) {
		LOG_ERR("Daylight GPIO not ready");
		return -EINVAL;
	}
	ret = gpio_pin_configure(daylight_gpio_dev, daylight_pin, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Daylight pin configure %d", ret);
		return ret;
	}
	return 0;
}

void daylight_z_enable(void)
{
	if (daylight_ready) {
		gpio_pin_set(daylight_gpio_dev, daylight_pin, 1);
	}
}

void daylight_z_disable(void)
{
	if (daylight_ready) {
		gpio_pin_set(daylight_gpio_dev, daylight_pin, 0);
	}
}
