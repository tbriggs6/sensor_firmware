/*
 * Configuration storage - Zephyr Settings/NVS implementation.
 */
#include "config_z.h"
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(config_z, CONFIG_LOG_DEFAULT_LEVEL);

#define SETTINGS_KEY "sensor_cfg"
#define CONFIG_STORAGE_MAGIC 0x534E5300u

typedef struct {
	uint32_t magic;
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t device_type;
	uint32_t sensor_interval;
	uint32_t max_failures;
	uint32_t retry_interval;
	uint16_t server[8];
	uint16_t local_calibration[8];
} config_store_t;

static config_store_t config_ram;
static unsigned int config_dev_type_ram;
static int calibration_changed_flag;

static int config_save(void)
{
	int ret;
	ret = settings_save_one(SETTINGS_KEY, &config_ram, sizeof(config_ram));
	if (ret) {
		LOG_ERR("config save %d", ret);
	}
	return ret;
}

static int config_load(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len;

	if (strcmp(key, SETTINGS_KEY) != 0) {
		return 0;
	}
	len = read_cb(cb_arg, &config_ram, sizeof(config_ram));
	if (len != sizeof(config_ram)) {
		return -EINVAL;
	}
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sensor_cfg, SETTINGS_KEY, NULL, config_load, config_save, NULL);

static void set_defaults(void)
{
	config_ram.magic = CONFIG_Z_MAGIC;
	config_ram.major_version = CONFIG_Z_VERSION_MAJOR;
	config_ram.minor_version = CONFIG_Z_VERSION_MINOR;
	config_ram.sensor_interval = 10;
	config_ram.max_failures = 100;
	config_ram.retry_interval = 15;
	memset(config_ram.server, 0, sizeof(config_ram.server));
	config_ram.server[7] = 1; /* fd00::1 */
	config_ram.local_calibration[0] = 2;
	config_ram.local_calibration[1] = 0;
	config_ram.local_calibration[2] = 64;
	for (int i = 3; i < 8; i++) {
		config_ram.local_calibration[i] = 0;
	}
}

int config_z_init(unsigned int dev_type)
{
	int ret;

	config_dev_type_ram = dev_type;
	calibration_changed_flag = 0;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("settings init %d", ret);
		set_defaults();
		return ret;
	}

	ret = settings_load();
	if (ret) {
		LOG_WRN("settings load %d, using defaults", ret);
		set_defaults();
		config_ram.device_type = dev_type;
		(void)config_save();
		return 0;
	}

	if (config_ram.magic != CONFIG_Z_MAGIC || config_ram.device_type != dev_type) {
		LOG_INF("config magic/type mismatch, applying defaults");
		set_defaults();
		config_ram.device_type = dev_type;
		(void)config_save();
	}

	return 0;
}

void config_z_set_magic(uint32_t magic)
{
	config_ram.magic = magic;
}

uint32_t config_z_get_magic(void)
{
	return config_ram.magic;
}

int config_z_is_magic(void)
{
	return config_ram.magic == CONFIG_Z_MAGIC ? 1 : 0;
}

void config_z_set_sensor_interval(int seconds)
{
	if (seconds < 5) {
		seconds = 5;
	} else if (seconds > 7200) {
		seconds = 7200;
	}
	config_ram.sensor_interval = (uint32_t)seconds;
}

int config_z_get_sensor_interval(void)
{
	return (int)config_ram.sensor_interval;
}

void config_z_set_ctime_offset(int ctime)
{
	(void)ctime;
}

int config_z_get_ctime_offset(void)
{
	return 0;
}

void config_z_set_major_version(uint32_t v)
{
	config_ram.major_version = v;
}

uint32_t config_z_get_major_version(void)
{
	return config_ram.major_version;
}

void config_z_set_minor_version(uint32_t v)
{
	config_ram.minor_version = v;
}

uint32_t config_z_get_minor_version(void)
{
	return config_ram.minor_version;
}

int config_z_get_devtype(void)
{
	return (int)config_dev_type_ram;
}

int config_z_get(int config_id)
{
	switch (config_id) {
	case CONFIG_DEVTYPE:
		return config_z_get_devtype();
	case CONFIG_CTIME:
		return config_z_get_ctime_offset();
	case CONFIG_SENSOR_INTERVAL:
		return config_z_get_sensor_interval();
	case CONFIG_MAX_FAILURES:
		return (int)config_z_get_max_failures();
	case CONFIG_RETRY_INTERVAL:
		return (int)config_z_get_retry_interval();
	case CONFIG_CAL1 ... CONFIG_CAL8:
		return (int)config_z_get_calibration(config_id - CONFIG_CAL1);
	default:
		return -1;
	}
}

void config_z_set(int config_id, int value)
{
	switch (config_id) {
	case CONFIG_CTIME:
		config_z_set_ctime_offset(value);
		break;
	case CONFIG_SENSOR_INTERVAL:
		config_z_set_sensor_interval(value);
		break;
	case CONFIG_MAX_FAILURES:
		config_z_set_max_failures((uint32_t)value);
		break;
	case CONFIG_RETRY_INTERVAL:
		config_z_set_retry_interval((uint32_t)value);
		break;
	case CONFIG_CAL1 ... CONFIG_CAL8:
		config_z_set_calibration(config_id - CONFIG_CAL1, (uint16_t)value);
		break;
	default:
		break;
	}
}

void config_z_get_receiver(config_receiver_t *receiver)
{
	if (receiver) {
		memcpy(receiver->server, config_ram.server, sizeof(config_ram.server));
	}
}

void config_z_set_receiver(const config_receiver_t *receiver)
{
	if (receiver) {
		memcpy(config_ram.server, receiver->server, sizeof(config_ram.server));
	}
}

uint16_t config_z_get_remote_port(void)
{
	return 5323;
}

uint32_t config_z_get_max_failures(void)
{
	return config_ram.max_failures;
}

void config_z_set_max_failures(uint32_t max)
{
	config_ram.max_failures = max < 10 ? 10 : max;
}

uint32_t config_z_get_retry_interval(void)
{
	return config_ram.retry_interval;
}

void config_z_set_retry_interval(uint32_t seconds)
{
	if (seconds < 5) {
		seconds = 5;
	} else if (seconds > 30) {
		seconds = 30;
	}
	config_ram.retry_interval = seconds;
}

void config_z_clear_calibration_changed(void)
{
	calibration_changed_flag = 0;
}

void config_z_set_calibration_change(void)
{
	calibration_changed_flag = 1;
}

int config_z_did_calibration_change(void)
{
	return calibration_changed_flag;
}

void config_z_set_calibration(int cal_num, uint16_t value)
{
	if (cal_num < 0 || cal_num >= 8) {
		return;
	}
	config_ram.local_calibration[cal_num] = value;
	calibration_changed_flag = 1;
}

uint16_t config_z_get_calibration(int cal_num)
{
	if (cal_num < 0 || cal_num >= 8) {
		return 0xffff;
	}
	return config_ram.local_calibration[cal_num];
}

__weak void config_z_timeout_change(void)
{
}
