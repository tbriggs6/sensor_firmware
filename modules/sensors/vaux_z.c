/*
 * AUX power control - Zephyr GPIO implementation.
 */
#include "vaux_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vaux_z, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *vaux_gpio_dev;
static gpio_pin_t vaux_pin = 29;
static bool vaux_ready;

int vaux_z_init(const void *gpio_dev, uint32_t pin)
{
	int ret;

	vaux_gpio_dev = (const struct device *)gpio_dev;
	vaux_pin = (gpio_pin_t)pin;
	vaux_ready = (vaux_gpio_dev != NULL && device_is_ready(vaux_gpio_dev));
	if (!vaux_ready) {
		LOG_ERR("VAUX GPIO not ready");
		return -EINVAL;
	}
	ret = gpio_pin_configure(vaux_gpio_dev, vaux_pin, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("VAUX pin configure %d", ret);
		return ret;
	}
	return 0;
}

void vaux_z_enable(void)
{
	if (vaux_ready) {
		gpio_pin_set(vaux_gpio_dev, vaux_pin, 1);
	}
}

void vaux_z_disable(void)
{
	if (vaux_ready) {
		gpio_pin_set(vaux_gpio_dev, vaux_pin, 0);
	}
}
