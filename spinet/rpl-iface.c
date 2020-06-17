/*
 * rpl-iface.c
 *
 *  Created on: Jun 14, 2020
 *      Author: contiki
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/ipv6/uiplib.h"
#include "sys/log.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/gpio.h)

#include <ti/drivers/Board.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_DBG


static void raise_rx_intr ()
{
	GPIO_setDio(27);
}

static void clear_rx_intr ()
{
	GPIO_clearDio(27);
}

static void toggle_rx_intr ()
{
	LOG_DBG("Toggle SPI intr\n");
	clear_rx_intr ();
	clock_delay_usec (5);
	raise_rx_intr ();
}


void rplstat_set_prefix ()
{
	uip_ip6addr_t addr;
	uiplib_ip6addrconv ("fd00::", &addr);

	NETSTACK_ROUTING.root_set_prefix (&addr, NULL);
	NETSTACK_ROUTING.root_start ();
}

void rpl_init (void)
{

	rplstat_set_prefix ();


  NETSTACK_ROUTING.root_start();
  NETSTACK_MAC.on();


}

int rpl_output (void)
{
	LOG_DBG ("RPL_OUTPUT is called\n");
	if (uip_len == 0) {
		LOG_ERR ("Err - uip_len = 0, no frame!\n");
		return 0;
	}

	toggle_rx_intr();
	return 0;
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface spi_rpl_interface =
	{
			rpl_init, rpl_output
	};

