#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spinet_node, LOG_LEVEL_INF);

/*
 * Zephyr entry point for the spinet (edge) node.
 * This skeleton will become the 6LoWPAN border router and SPI/UART
 * bridge to an upstream host (e.g. Raspberry Pi).
 */

void main(void)
{
	LOG_INF("spinet_node: Zephyr app starting");

	/* TODO: initialize config_z, messenger_z, and border-router logic here. */

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}

