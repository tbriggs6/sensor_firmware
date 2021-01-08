/*
 * vaux.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#include "contiki.h"
#include "dev/spi.h"
#include "sys/log.h"
#include "dev/leds.h"

#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"


#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/gpio.h)
#include DeviceFamily_constructPath(driverlib/ssi.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include DeviceFamily_constructPath(driverlib/udma.h)

/* Includes */
#include <ti/drivers/PIN.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/ioc.h)

#include <ti/drivers/Board.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "Board.h"


void daylight_enable( )
{

	IOCPinTypeGpioOutput(IOID_3);
	IOCIOPortPullSet(IOID_3, IOC_IOPULL_DOWN);
	uint32_t rc = GPIO_getOutputEnableDio(IOID_3);
	if (rc != GPIO_OUTPUT_ENABLE) {
		printf("Error! could not enable aux voltage for output!\n");
	}

	GPIO_setDio(IOID_3);


}

void daylight_disable( )
{
	IOCPinTypeGpioInput(IOID_3);
	GPIO_clearDio( IOID_3);
}

