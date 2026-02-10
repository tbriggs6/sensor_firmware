/*
 * Zephyr water sensor node: periodic sampling and reporting.
 * Uses config_z, messenger_z, command_z, and sensor_z modules.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "echo_z.h"
#include "config_z.h"
#include "messenger_z.h"
#include "command_z.h"
#include "message_z.h"
#include "ms5637_z.h"
#include "si7210_z.h"
#include "tcs3472_z.h"
#include "analog_z.h"
#include "vaux_z.h"
#include "daylight_z.h"

LOG_MODULE_REGISTER(water_node, LOG_LEVEL_INF);

#define WATER_SENSOR_DEVTYPE 0x111
#define PEER_IPV6_DEFAULT "fd00::1"

static uint32_t sequence;
static int failure_counter;
static int red_led, green_led;

static void build_and_send_data(void)
{
	water_data_t message;
	messenger_addr_t addr;
	config_receiver_t recv;
	ms5637_data_t mdata;
	si7210_data_t sdata;
	tcs3472_data_t cdata;

	memset(&message, 0, sizeof(message));
	message.header = WATER_DATA_HEADER_Z;
	message.sequence = sequence++;
	message.rssi = messenger_z_recvd_rssi();

	config_z_get_receiver(&recv);
	memcpy(addr.u16, recv.server, sizeof(addr.u16));

	vaux_z_enable();
	daylight_z_enable();
	k_msleep(10);

	if (ms5637_z_sample(&mdata) == 0) {
		message.pressure = mdata.pressure;
		message.temppressure = mdata.temperature;
	}
	if (si7210_z_sample(&sdata) == 0) {
		message.hall = (int16_t)sdata.magfield;
	}
	if (tcs3472_z_sample(&cdata, 0, 64) == 0) {
		message.color_red = cdata.red;
		message.color_green = cdata.green;
		message.color_blue = cdata.blue;
		message.color_clear = cdata.clear;
	}

	message.range1 = message.range2 = message.range3 = message.range4 = message.range5 = 0;
	message.battery = (uint16_t)analog_z_vbat_read();
	message.temperature = (uint16_t)analog_z_thermistor_read();

	daylight_z_disable();
	k_msleep(5);
	/* Ambient: could run tcs3472 again with LED off; for now reuse clear */
	message.ambient = cdata.clear;
	vaux_z_disable();

	green_led = 1;
	if (messenger_z_send(&addr, MESSAGE_SERVER_PORT_Z, (uint16_t)message.sequence,
			    &message, sizeof(message)) == 0) {
		messenger_z_wait_reply(2000);
		if (messenger_z_last_result_okack()) {
			failure_counter = 0;
			config_z_clear_calibration_changed();
			green_led = 0;
			red_led = 0;
		} else {
			failure_counter++;
			red_led = 1;
		}
	} else {
		failure_counter++;
		red_led = 1;
	}
}

static void send_calibration(void)
{
	water_cal_t message;
	ms5637_caldata_t mcal;
	si7210_calibration_t scal;
	messenger_addr_t addr;
	config_receiver_t recv;

	memset(&message, 0, sizeof(message));
	message.header = WATER_CAL_HEADER_Z;
	message.sequence = sequence++;
	message.rssi = messenger_z_recvd_rssi();

	vaux_z_enable();
	k_msleep(5);
	if (ms5637_z_read_calibration_data(&mcal) == 0) {
		message.caldata[0] = mcal.sens;
		message.caldata[1] = mcal.off;
		message.caldata[2] = mcal.tcs;
		message.caldata[3] = mcal.tco;
		message.caldata[4] = mcal.tref;
		message.caldata[5] = mcal.temp;
	}
	si7210_z_read_cal(&scal);
	vaux_z_disable();

	for (int i = 0; i < 8; i++) {
		message.resistorVals[i] = config_z_get_calibration(i);
	}
	message.si7210_gain = scal.config_range;
	message.si7210_offset = 0;

	config_z_get_receiver(&recv);
	memcpy(addr.u16, recv.server, sizeof(addr.u16));
	messenger_z_send(&addr, MESSAGE_SERVER_PORT_Z, (uint16_t)message.sequence,
			 &message, sizeof(message));
	messenger_z_wait_reply(2000);
	if (messenger_z_last_result_okack()) {
		failure_counter = 0;
		config_z_clear_calibration_changed();
	} else {
		failure_counter++;
	}
}

static void sensor_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(sensor_work, sensor_work_handler);

static void sensor_work_handler(struct k_work *work)
{
	if (config_z_did_calibration_change()) {
		send_calibration();
		k_work_reschedule(&sensor_work, K_SECONDS(config_z_get_retry_interval()));
		if (failure_counter > 0) {
			return;
		}
	}
	build_and_send_data();
	if (failure_counter >= (int)config_z_get_max_failures()) {
		LOG_ERR("max failures, rebooting");
		k_msleep(100);
		/* No watchdog_reboot in Zephyr app; just reschedule retry */
		failure_counter = 0;
	}
	k_work_reschedule(&sensor_work,
			 failure_counter > 0 ? K_SECONDS(config_z_get_retry_interval())
					    : K_SECONDS(config_z_get_sensor_interval()));
}

static void sysmon_timer_expiry(struct k_timer *timer);
K_TIMER_DEFINE(sysmon_timer, sysmon_timer_expiry, NULL);

static void sysmon_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	/* LED blink and watchdog check: could toggle GPIO here */
	if (failure_counter >= (int)config_z_get_max_failures()) {
		/* Trigger reboot or alert */
	}
}

void main(void)
{
	LOG_INF("water_node: Zephyr app starting");

	sequence = 0;
	failure_counter = 0;
	red_led = 0;
	green_led = 0;

	if (config_z_init(WATER_SENSOR_DEVTYPE) != 0) {
		LOG_ERR("config_z init failed");
	}
	messenger_z_init();
	command_z_register();

	/* Sensor init: pass NULL if no devicetree; drivers will no-op or return -EINVAL */
	ms5637_z_init(NULL, 0x76);
	si7210_z_init(NULL, 0x32);
	tcs3472_z_init(NULL, 0x29);
	analog_z_init(NULL, 0, 0);
	vaux_z_init(NULL, 29);
	daylight_z_init(NULL, 3);

	k_timer_start(&sysmon_timer, K_MSEC(500), K_MSEC(500));

	/* One-time echo validation */
	k_sleep(K_SECONDS(5));
	if (net_echo_client_send_once(PEER_IPV6_DEFAULT, NET_ECHO_PORT, NULL, 0, 3000)) {
		LOG_INF("echo OK");
	}

	config_z_set_calibration_change();
	k_work_reschedule(&sensor_work, K_SECONDS(1));

	while (1) {
		k_sleep(K_SECONDS(60));
	}
}
