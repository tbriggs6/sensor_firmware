/*
 * SI7210 hall effect sensor - Zephyr I2C implementation.
 * Simplified blocking read of magnetic field; full compensation logic
 * can be added later to match Contiki si7210.c.
 */
#include "si7210_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(si7210_z, CONFIG_LOG_DEFAULT_LEVEL);

#define SLV_ADDR_DEF  0x32
#define REG_DSPSIGM   0xc1
#define REG_DSPSIGL   0xc2
#define REG_DSPSIGSEL 0xc3
#define DSP_SIGSEL_FIELD 0x04
#define DSP_SIGM_DATA_FLAG 0x80
#define POLL_MS 10
#define POLL_MAX 30

static const struct device *si7210_i2c_dev;
static uint8_t si7210_addr = SLV_ADDR_DEF;

int si7210_z_init(const void *i2c_dev, uint8_t addr)
{
	si7210_i2c_dev = (const struct device *)i2c_dev;
	si7210_addr = addr;
	if (si7210_i2c_dev == NULL || !device_is_ready(si7210_i2c_dev)) {
		LOG_ERR("SI7210 I2C device not ready");
		return -EINVAL;
	}
	return 0;
}

int si7210_z_read_cal(si7210_calibration_t *cal)
{
	if (cal == NULL) {
		return -EINVAL;
	}
	/* Default; app can override via config_z when integrated */
	cal->config_range = 2;
	return 0;
}

static int si7210_read_reg(uint8_t reg, uint8_t *value)
{
	return i2c_write_read(si7210_i2c_dev, si7210_addr, &reg, 1, value, 1);
}

static int si7210_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = { reg, value };
	return i2c_write(si7210_i2c_dev, buf, 2, si7210_addr);
}

int si7210_z_sample(si7210_data_t *data)
{
	uint8_t sel = REG_DSPSIGSEL;
	uint8_t sigsel_val = DSP_SIGSEL_FIELD;
	uint8_t msb, lsb;
	int ret;
	int i;

	if (si7210_i2c_dev == NULL || data == NULL) {
		return -EINVAL;
	}

	data->rc = false;
	data->magfield = 0;

	/* Select field measurement */
	ret = i2c_write_read(si7210_i2c_dev, si7210_addr, &sel, 1, &sigsel_val, 1);
	if (ret) {
		LOG_ERR("SI7210 sigsel read %d", ret);
		return ret;
	}
	sigsel_val |= DSP_SIGSEL_FIELD;
	ret = si7210_write_reg(REG_DSPSIGSEL, sigsel_val);
	if (ret) {
		LOG_ERR("SI7210 sigsel write %d", ret);
		return ret;
	}

	/* Trigger and poll for data ready */
	for (i = 0; i < POLL_MAX; i++) {
		k_msleep(POLL_MS);
		ret = si7210_read_reg(REG_DSPSIGM, &msb);
		if (ret) {
			return ret;
		}
		if (msb & DSP_SIGM_DATA_FLAG) {
			break;
		}
	}
	if (!(msb & DSP_SIGM_DATA_FLAG)) {
		LOG_WRN("SI7210 data not ready");
		return -ETIMEDOUT;
	}

	ret = si7210_read_reg(REG_DSPSIGL, &lsb);
	if (ret) {
		return ret;
	}

	data->magfield = (uint16_t)(msb & 0x7F) << 8 | lsb;
	data->rc = true;
	return 0;
}
