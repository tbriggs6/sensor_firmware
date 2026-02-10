/*
 * Analog inputs - Zephyr ADC implementation.
 * Uses Zephyr ADC API; channel mapping is board-specific (overlay).
 */
#include "analog_z.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(analog_z, CONFIG_LOG_DEFAULT_LEVEL);

#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REF ADC_REF_INTERNAL
#define ADC_ACQ_MS 10

static const struct device *analog_adc_dev;
static int16_t adc_buf;
static struct adc_sequence adc_seq = {
	.buffer = &adc_buf,
	.buffer_size = sizeof(adc_buf),
	.resolution = ADC_RESOLUTION,
};
static uint8_t channel_vbat_cfg;
static uint8_t channel_therm_cfg;
static bool adc_ready;

int analog_z_init(const void *adc_dev, uint8_t channel_vbat, uint8_t channel_therm)
{
	analog_adc_dev = (const struct device *)adc_dev;
	channel_vbat_cfg = channel_vbat;
	channel_therm_cfg = channel_therm;
	adc_ready = (analog_adc_dev != NULL && device_is_ready(analog_adc_dev));
	if (!adc_ready) {
		LOG_ERR("Analog ADC device not ready");
		return -EINVAL;
	}
	return 0;
}

static uint32_t adc_read_channel(uint8_t channel)
{
	int ret;

	if (!adc_ready) {
		return 0;
	}
	adc_seq.channel_id = channel;
	ret = adc_read(analog_adc_dev, &adc_seq);
	if (ret) {
		LOG_ERR("ADC read %d", ret);
		return 0;
	}
	return (uint32_t)(adc_buf > 0 ? adc_buf : 0);
}

uint32_t analog_z_vbat_read(void)
{
	return adc_read_channel(channel_vbat_cfg);
}

uint32_t analog_z_vbat_millivolts(uint32_t reading)
{
	/* Board-specific scaling; placeholder for 12-bit ADC and external gain */
	(void)reading;
	return 0;
}

uint32_t analog_z_thermistor_read(void)
{
	return adc_read_channel(channel_therm_cfg);
}

int analog_z_conductivity_read(uint16_t *range, int len)
{
	(void)range;
	(void)len;
	/* Optional: add 5 ADC channels for conductivity ranges when needed */
	return 0;
}
