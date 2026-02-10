/*
 * TCS3472 color sensor - Zephyr I2C implementation.
 */
#include "tcs3472_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(tcs3472_z, CONFIG_LOG_DEFAULT_LEVEL);

#define ADDR_DEF   0x29
#define REG_ENABLE 0x00
#define REG_ATIME  0x01
#define REG_CONTROL 0x0f
#define REG_ID     0x12
#define REG_CDATA  0x14
#define REG_RDATA  0x16
#define REG_GDATA  0x18
#define REG_BDATA  0x1a
#define CMD_ENABLE 0x80
#define PON 0x01
#define AEN 0x02
#define INTEGRATION_MS 3

static const struct device *tcs3472_i2c_dev;
static uint8_t tcs3472_addr = ADDR_DEF;

int tcs3472_z_init(const void *i2c_dev, uint8_t addr)
{
	tcs3472_i2c_dev = (const struct device *)i2c_dev;
	tcs3472_addr = addr;
	if (tcs3472_i2c_dev == NULL || !device_is_ready(tcs3472_i2c_dev)) {
		LOG_ERR("TCS3472 I2C device not ready");
		return -EINVAL;
	}
	return 0;
}

static int tcs3472_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = { CMD_ENABLE | reg, value };
	return i2c_write(tcs3472_i2c_dev, buf, 2, tcs3472_addr);
}

static int tcs3472_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
	uint8_t cmd = CMD_ENABLE | reg;
	return i2c_write_read(tcs3472_i2c_dev, tcs3472_addr, &cmd, 1, buf, len);
}

int tcs3472_z_sample(tcs3472_data_t *data, uint8_t gain, uint8_t cycles)
{
	uint8_t buf[2];
	int ret;
	int i;

	if (tcs3472_i2c_dev == NULL || data == NULL) {
		return -EINVAL;
	}

	data->rc = false;
	data->red = data->green = data->blue = data->clear = 0;

	ret = tcs3472_write_reg(REG_ENABLE, PON);
	if (ret) {
		LOG_ERR("TCS3472 ENABLE PON %d", ret);
		return ret;
	}
	k_msleep(INTEGRATION_MS);

	ret = tcs3472_write_reg(REG_CONTROL, gain & 3);
	if (ret) {
		return ret;
	}
	ret = tcs3472_write_reg(REG_ATIME, cycles);
	if (ret) {
		return ret;
	}

	ret = tcs3472_write_reg(REG_ENABLE, PON | AEN);
	if (ret) {
		return ret;
	}

	for (i = 0; i < 100; i++) {
		k_msleep(INTEGRATION_MS);
		ret = tcs3472_read_regs(0x13, buf, 1);
		if (ret) {
			return ret;
		}
		if (buf[0] & 0x01) {
			break;
		}
	}
	if (!(buf[0] & 0x01)) {
		LOG_WRN("TCS3472 data not ready");
		return -ETIMEDOUT;
	}

	ret = tcs3472_read_regs(REG_CDATA, buf, 2);
	if (ret) {
		return ret;
	}
	data->clear = (uint16_t)buf[0] | (uint16_t)buf[1] << 8;

	ret = tcs3472_read_regs(REG_RDATA, buf, 2);
	if (ret) {
		return ret;
	}
	data->red = (uint16_t)buf[0] | (uint16_t)buf[1] << 8;

	ret = tcs3472_read_regs(REG_GDATA, buf, 2);
	if (ret) {
		return ret;
	}
	data->green = (uint16_t)buf[0] | (uint16_t)buf[1] << 8;

	ret = tcs3472_read_regs(REG_BDATA, buf, 2);
	if (ret) {
		return ret;
	}
	data->blue = (uint16_t)buf[0] | (uint16_t)buf[1] << 8;

	data->rc = true;
	return 0;
}
