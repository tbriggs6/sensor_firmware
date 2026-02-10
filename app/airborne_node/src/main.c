/*
 * Zephyr airborne sensor node: SI7020 humidity/temp, periodic reporting.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "echo_z.h"
#include "config_z.h"
#include "messenger_z.h"
#include "command_z.h"
#include "message_z.h"
#include "si7020_z.h"
#include "analog_z.h"

LOG_MODULE_REGISTER(airborne_node, LOG_LEVEL_INF);

#define AIRBORNE_SENSOR_DEVTYPE 0x333
#define PEER_IPV6_DEFAULT "fd00::1"

static uint32_t sequence;
static int failure_counter;

static void send_airborne_data(void)
{
	airborne_t message;
	messenger_addr_t addr;
	config_receiver_t recv;
	si7020_data_t sdata;

	memset(&message, 0, sizeof(message));
	message.header = AIRBORNE_HEADER_Z;
	message.sequence = sequence++;
	message.rssi = messenger_z_recvd_rssi();

	if (si7020_z_sample(&sdata) == 0) {
		message.si7020_humid = sdata.humidity;
		message.si7020_temp = sdata.temperature;
	}
	message.ms5637_pressure = 0;
	message.ms5637_temp = 0;
	message.battery = analog_z_vbat_read();
	message.i2cerror = 0;

	config_z_get_receiver(&recv);
	memcpy(addr.u16, recv.server, sizeof(addr.u16));

	if (messenger_z_send(&addr, MESSAGE_SERVER_PORT_Z, (uint16_t)message.sequence,
			    &message, sizeof(message)) == 0) {
		messenger_z_wait_reply(2000);
		if (messenger_z_last_result_okack()) {
			failure_counter = 0;
		} else {
			failure_counter++;
		}
	} else {
		failure_counter++;
	}
}

static void airborne_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(airborne_work, airborne_work_handler);

static void airborne_work_handler(struct k_work *work)
{
	send_airborne_data();
	k_work_reschedule(&airborne_work,
			  failure_counter > 0 ? K_SECONDS(config_z_get_retry_interval())
					      : K_SECONDS(config_z_get_sensor_interval()));
}

void main(void)
{
	LOG_INF("airborne_node: Zephyr app starting");

	sequence = 0;
	failure_counter = 0;

	config_z_init(AIRBORNE_SENSOR_DEVTYPE);
	messenger_z_init();
	command_z_register();
	si7020_z_init(NULL, 0x40);
	analog_z_init(NULL, 0, 0);

	k_sleep(K_SECONDS(5));
	if (net_echo_client_send_once(PEER_IPV6_DEFAULT, NET_ECHO_PORT, NULL, 0, 3000)) {
		LOG_INF("echo OK");
	}

	k_work_reschedule(&airborne_work, K_SECONDS(1));

	while (1) {
		k_sleep(K_SECONDS(60));
	}
}
