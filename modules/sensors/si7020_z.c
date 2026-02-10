/*
 * SI7020 humidity/temperature - Zephyr I2C implementation.
 */
#include "si7020_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(si7020_z, CONFIG_LOG_DEFAULT_LEVEL);

#define ADDR_DEF 0x40
#define CMD_RH   0xf5
#define CMD_TEMP 0xf3
#define MEASURE_POLL_MS 20
#define MEASURE_POLL_MAX 25

static const struct device *si7020_i2c_dev;
static uint8_t si7020_addr = ADDR_DEF;

int si7020_z_init(const void *i2c_dev, uint8_t addr)
{
	si7020_i2c_dev = (const struct device *)i2c_dev;
	si7020_addr = addr;
	if (si7020_i2c_dev == NULL || !device_is_ready(si7020_i2c_dev)) {
		LOG_ERR("SI7020 I2C device not ready");
		return -EINVAL;
	}
	return 0;
}

static int si7020_measure(uint8_t cmd, uint16_t *value)
{
	uint8_t buf[2];
	int ret;
	int i;

	ret = i2c_write(si7020_i2c_dev, &cmd, 1, si7020_addr);
	if (ret) {
		return ret;
	}
	for (i = 0; i < MEASURE_POLL_MAX; i++) {
		k_msleep(MEASURE_POLL_MS);
		ret = i2c_read(si7020_i2c_dev, buf, 2, si7020_addr);
		if (ret == 0) {
			*value = (uint16_t)buf[0] << 8 | buf[1];
			return 0;
		}
	}
	return -ETIMEDOUT;
}

int si7020_z_sample(si7020_data_t *data)
{
	uint16_t raw;
	int ret;

	if (si7020_i2c_dev == NULL || data == NULL) {
		return -EINVAL;
	}

	data->rc = false;
	data->humidity = 0;
	data->temperature = 0;

	ret = si7020_measure(CMD_RH, &raw);
	if (ret) {
		LOG_ERR("SI7020 humidity %d", ret);
		return ret;
	}
	data->humidity = raw;

	ret = si7020_measure(CMD_TEMP, &raw);
	if (ret) {
		LOG_ERR("SI7020 temp %d", ret);
		return ret;
	}
	data->temperature = raw;
	data->rc = true;
	return 0;
}
