#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "echo_z.h"
#include "config_z.h"
#include "messenger_z.h"
#include "command_z.h"

LOG_MODULE_REGISTER(spinet_node, LOG_LEVEL_INF);

#define SPINET_DEVTYPE 0x222
#define ECHO_SERVER_STACK_SIZE 1024
#define ECHO_SERVER_PRIORITY   5
K_THREAD_STACK_DEFINE(echo_server_stack, ECHO_SERVER_STACK_SIZE);
static struct k_thread echo_server_thread;

static void echo_server_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	net_echo_server_run(NET_ECHO_PORT);
}

/*
 * Zephyr spinet (edge) node: UDP echo server and command handler.
 * Border-router and SPI/UART bridge can be added later.
 */
void main(void)
{
	LOG_INF("spinet_node: Zephyr app starting");

	config_z_init(SPINET_DEVTYPE);
	messenger_z_init();
	command_z_register();

	k_thread_create(&echo_server_thread, echo_server_stack,
			K_THREAD_STACK_SIZEOF(echo_server_stack),
			echo_server_entry, NULL, NULL, NULL,
			ECHO_SERVER_PRIORITY, 0, K_NO_WAIT);

	while (1) {
		k_sleep(K_SECONDS(10));
	}
}

