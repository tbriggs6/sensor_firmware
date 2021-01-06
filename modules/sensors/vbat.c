/*
 * vbat.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */


#include "contiki.h"
#include "sys/log.h"
#include "dev/leds.h"

#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"


//#include <ti/devices/DeviceFamily.h>
//#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
//#include DeviceFamily_constructPath(driverlib/chipinfo.h)
//#include DeviceFamily_constructPath(driverlib/vims.h)
//#include DeviceFamily_constructPath(driverlib/prcm.h)
//#include DeviceFamily_constructPath(driverlib/gpio.h)
//#include DeviceFamily_constructPath(driverlib/ssi.h)
//#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include DeviceFamily_constructPath(driverlib/aux_adc.h)
#include DeviceFamily_constructPath(driverlib/aux_sysif.h)

//
///* Includes */
//#include <ti/drivers/PIN.h>
//#include <ti/devices/DeviceFamily.h>
//#include DeviceFamily_constructPath(driverlib/ioc.h)
//
//#include <ti/drivers/Board.h>
//#include <ti/drivers/SPI.h>
//#include <ti/drivers/GPIO.h>
//
//#include "Board.h""



uint32_t vbat_read()
{

	AUXSYSIFOpModeChange(AUX_SYSIF_OPMODE_TARGET_A);

	clock_delay_usec(100);

	AUXADCEnableSync(AUXADC_REF_FIXED, AUXADC_SAMPLE_TIME_170_US, AUXADC_TRIGGER_MANUAL);

	clock_delay_usec(100);
	AUXADCFlushFifo( );

	AUXADCSelectInput(ADC_COMPB_IN_AUXIO22);
	clock_delay_usec(100);

	AUXADCGenManualTrigger( );
	while (AUXADCGetFifoStatus() & AUXADC_FIFO_EMPTY_M)
		clock_delay_usec(100);

	uint32_t sample = AUXADCPopFifo( );

	AUXADCDisable( );
	AUXSYSIFOpModeChange(AUX_SYSIF_OPMODE_TARGET_PDLP);
	return sample;
}

uint32_t vbat_millivolts(uint32_t reading)
{
	uint32_t uvolts = AUXADCValueToMicrovolts(AUXADC_FIXED_REF_VOLTAGE_NORMAL, reading);

	// there is an external opamp with a gain of approx 1.1
	float fvolts = (float) uvolts;
	fvolts = fvolts / 1.1;
	uvolts = (uint32_t) fvolts;
	uint32_t mvolts = uvolts / 1000;
	return mvolts;
}
