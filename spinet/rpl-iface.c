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
#include "spi-regs.h"
#include "spi-iface.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/gpio.h)
#include DeviceFamily_constructPath(driverlib/ssi.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)

#include <ti/drivers/Board.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "Board.h"

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_ERR


static void raise_rx_intr ()
{
	GPIO_setDio(Board_GPIO_RXIRQ);



}

static void clear_rx_intr ()
{
	GPIO_clearDio(Board_GPIO_RXIRQ);
}

static void toggle_rx_intr ()
{
	clock_delay_usec (150);
	raise_rx_intr ();
	clock_delay_usec (150);
	clear_rx_intr ();
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

	clear_rx_intr( );

}

int rpl_output (void)
{
	LOG_DBG ("RPL_OUTPUT is called uip_len = %d %d\n", uip_len, register_read(17));
	if (uip_len == 0) {
		LOG_ERR ("Err - uip_len = 0, no frame!\n");
		return 0;
	}

	copy_uip_to_spi( );

	toggle_rx_intr();
	return 0;
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface spi_rpl_interface =
	{
			rpl_init, rpl_output
	};
