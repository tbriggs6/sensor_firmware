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


void vaux_enable( )
{

	printf("Turning on AUX power\n");
	IOCPinTypeGpioOutput(IOID_29);
	IOCIOPortPullSet(IOID_29, IOC_IOPULL_DOWN);
	uint32_t rc = GPIO_getOutputEnableDio(IOID_29);
	if (rc != GPIO_OUTPUT_ENABLE) {
		printf("Error! could not enable aux voltage for output!\n");
	}

	GPIO_setDio(IOID_29);


}

void vaux_disable( )
{
	printf("Disabling AUX power\n");
	IOCPinTypeGpioInput(IOID_29);
	GPIO_clearDio( IOID_29);
//	IOCIOPortPullSet(IOID_29, IOC_IOPULL_DOWN);
//	GPIO_getOutputEnableDio(IOID_29);
//

}

