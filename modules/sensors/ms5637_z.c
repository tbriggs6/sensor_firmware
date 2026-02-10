/*
 * MS5637 pressure/temperature sensor - Zephyr I2C implementation.
 */
#include "ms5637_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ms5637_z, CONFIG_LOG_DEFAULT_LEVEL);

#define REG_DATA       0x00
#define CMD_START_PRESS 0x44
#define CMD_START_TEMP  0x54
#define REG_SENS       0xa2
#define REG_OFF        0xa4
#define REG_TCS        0xa6
#define REG_TCO        0xa8
#define REG_TREF       0xaa
#define REG_TEMP       0xac
#define CONV_DELAY_MS   4

static const struct device *ms5637_i2c_dev;
static uint8_t ms5637_addr;

int ms5637_z_init(const void *i2c_dev, uint8_t addr)
{
	ms5637_i2c_dev = (const struct device *)i2c_dev;
	ms5637_addr = addr;
	if (ms5637_i2c_dev == NULL || !device_is_ready(ms5637_i2c_dev)) {
		LOG_ERR("MS5637 I2C device not ready");
		return -EINVAL;
	}
	return 0;
}

int ms5637_z_read_calibration_data(ms5637_caldata_t *caldata)
{
	uint8_t reg;
	uint8_t vals[2];
	int ret;

	if (ms5637_i2c_dev == NULL || caldata == NULL) {
		return -EINVAL;
	}

	memset(caldata, 0, sizeof(ms5637_caldata_t));

	reg = REG_SENS;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 SENS read %d", ret);
		return ret;
	}
	caldata->sens = (uint16_t)vals[0] << 8 | vals[1];

	reg = REG_OFF;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 OFF read %d", ret);
		return ret;
	}
	caldata->off = (uint16_t)vals[0] << 8 | vals[1];

	reg = REG_TCS;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 TCS read %d", ret);
		return ret;
	}
	caldata->tcs = (uint16_t)vals[0] << 8 | vals[1];

	reg = REG_TCO;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 TCO read %d", ret);
		return ret;
	}
	caldata->tco = (uint16_t)vals[0] << 8 | vals[1];

	reg = REG_TREF;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 TREF read %d", ret);
		return ret;
	}
	caldata->tref = (uint16_t)vals[0] << 8 | vals[1];

	reg = REG_TEMP;
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, vals, 2);
	if (ret) {
		LOG_ERR("MS5637 TEMP read %d", ret);
		return ret;
	}
	caldata->temp = (uint16_t)vals[0] << 8 | vals[1];

	return 0;
}

static int ms5637_cvt_and_read(uint8_t cmd, uint32_t *value)
{
	uint8_t reg = REG_DATA;
	uint8_t bytes[3];
	int ret;

	ret = i2c_write(ms5637_i2c_dev, &cmd, 1, ms5637_addr);
	if (ret) {
		return ret;
	}
	k_msleep(CONV_DELAY_MS);
	ret = i2c_write_read(ms5637_i2c_dev, ms5637_addr, &reg, 1, bytes, 3);
	if (ret) {
		return ret;
	}
	*value = (uint32_t)bytes[0] << 16 | (uint32_t)bytes[1] << 8 | bytes[2];
	return 0;
}

int ms5637_z_sample(ms5637_data_t *data)
{
	uint32_t raw;
	int ret;

	if (ms5637_i2c_dev == NULL || data == NULL) {
		return -EINVAL;
	}

	data->status = 0;

	ret = ms5637_cvt_and_read(CMD_START_PRESS, &raw);
	if (ret) {
		LOG_ERR("MS5637 pressure read %d", ret);
		return ret;
	}
	data->pressure = raw;

	ret = ms5637_cvt_and_read(CMD_START_TEMP, &raw);
	if (ret) {
		LOG_ERR("MS5637 temp read %d", ret);
		return ret;
	}
	data->temperature = raw;
	data->status = 1;
	return 0;
}
