/*
 * Configuration storage - Zephyr implementation.
 * API aligned with config.h for compatibility; no Contiki types.
 */
#ifndef MODULES_CONFIG_CONFIG_Z_H_
#define MODULES_CONFIG_CONFIG_Z_H_

#include <stdint.h>

#define CONFIG_Z_VERSION_MAJOR 1
#define CONFIG_Z_VERSION_MINOR 0

/* Same config type tokens as config.h for command/response compatibility */
typedef enum {
	CONFIG_CTIME = 10,
	CONFIG_ROUTER = 11,
	CONFIG_DEVTYPE = 12,
	CONFIG_UPTIME = 13,
	CONFIG_ENERGEST_CPU = 14,
	CONFIG_ENERGEST_LPM = 15,
	CONFIG_ENERGEST_TRANSMIT = 16,
	CONFIG_ENERGEST_LISTEN = 17,
	CONFIG_ENERGEST_DEEP_LPM = 18,
	CONFIG_SENSOR_INTERVAL = 32,
	CONFIG_MAX_FAILURES = 33,
	CONFIG_RETRY_INTERVAL = 34,
	CONFIG_CAL1 = 64,
	CONFIG_CAL2 = 65,
	CONFIG_CAL3 = 66,
	CONFIG_CAL4 = 67,
	CONFIG_CAL5 = 68,
	CONFIG_CAL6 = 69,
	CONFIG_CAL7 = 70,
	CONFIG_CAL8 = 71
} config_type_t;

#define CONFIG_Z_MAGIC (0x0B160000u | (CONFIG_Z_VERSION_MAJOR << 4) | CONFIG_Z_VERSION_MINOR)

/** IPv6 address as 8 x uint16_t (network order) */
typedef struct {
	uint16_t server[8];
} config_receiver_t;

/**
 * Initialize config: load from NVS/Settings or set defaults.
 * \param dev_type device type (e.g. WATER_SENSOR_DEVTYPE)
 * \return 0 on success, negative errno on failure
 */
int config_z_init(unsigned int dev_type);

void config_z_set_magic(uint32_t magic);
uint32_t config_z_get_magic(void);
int config_z_is_magic(void);

void config_z_set_sensor_interval(int seconds);
int config_z_get_sensor_interval(void);

void config_z_set_ctime_offset(int ctime);
int config_z_get_ctime_offset(void);

void config_z_set_major_version(uint32_t v);
uint32_t config_z_get_major_version(void);
void config_z_set_minor_version(uint32_t v);
uint32_t config_z_get_minor_version(void);

int config_z_get_devtype(void);

int config_z_get(int config_id);
void config_z_set(int config_id, int value);

void config_z_get_receiver(config_receiver_t *receiver);
void config_z_set_receiver(const config_receiver_t *receiver);
uint16_t config_z_get_remote_port(void);

uint32_t config_z_get_max_failures(void);
void config_z_set_max_failures(uint32_t max);
uint32_t config_z_get_retry_interval(void);
void config_z_set_retry_interval(uint32_t seconds);

void config_z_clear_calibration_changed(void);
void config_z_set_calibration_change(void);
int config_z_did_calibration_change(void);

void config_z_set_calibration(int cal_num, uint16_t value);
uint16_t config_z_get_calibration(int cal_num);

/** Weak: allow app to trigger a config timeout (e.g. re-schedule send) */
void config_z_timeout_change(void);

#endif /* MODULES_CONFIG_CONFIG_Z_H_ */
