/*
 * si7020.c
 *
 *  Created on: Jan 5, 2021
 *      Author: contiki
 */

#include "si7210.h"
#include <contiki.h>
#include "ms5637.h"

#include "../modules/command/message.h"
#include "../modules/config/config.h"

#include <Board.h>
#include <dev/i2c-arch.h>
#include "sys/log.h"
#include "sensors.h"


#define LOG_MODULE "Si7210"
#define LOG_LEVEL LOG_LEVEL_SENSOR


#define SLV_ADDR 0x32

#define I2CBUS Board_I2C0


// Si7020 registers
#define HREVID 		0xC0
#define DSPSIGM 	0xC1
#define DSPSIGL 	0xC2
#define DSPSIGSEL 0xC3
#define POWERCTL 	0xC4
#define ARAUTOINC	0xC5
#define CTRL1			0xC6
#define CTRL2			0xC7
#define SLTIME		0xC8
#define CTRL3			0xC9
#define A0				0xCA
#define A1				0xCB
#define A2				0xCC
#define CTRL4			0xCD
#define A3				0xCE
#define A4				0xCF
#define	A5				0xD0
#define OTP_ADDR	0xE1
#define OTP_DATA 	0xE2
#define OTP_CTRL 	0xE3
#define TM_FG 		0xE4


// Si7020 register masks
#define CHIP_ID_MASK				0xF0
#define REV_ID_MASK					0x0F
#define DSP_SIGSEL_MASK 		0x07
#define MEAS_MASK						0x80
#define UCESTORE_MASK				0x08
#define ONEBURST_MASK				0x04
#define STOP_MASK						0x02
#define SLEEP_MASK					0x01
#define ARAUTOMATIC_MASK  	0x01
#define SW_LOWFIELD_MASK 		0x80
#define SW_OP_MASK  				0x7F
#define SW_FIELDPOL_SEL_MASK 0xC0
#define SW_HYST_MASK 				0x3F
#define SW_TAMPER_MASK 			0xFC
#define SL_FAST_MASK				0x02
#define SL_TIMEENA_MASK 		0x01
#define DF_BURSTSIZE_MASK 	0xE0
#define DF_BW_MASK 					0x1E
#define DF_IIR_MASK					0x01
#define OTP_READ_MASK				0x02
#define OTP_BUSY_MASK				0x01
#define TM_FG_MASK					0x03

// other masks
#define DSP_SIGM_DATA_FLAG 		0x80
#define DSP_SIGM_DATA_MASK 		0x7F
#define DSP_SIGSEL_TEMP_MASK 	0x01
#define DSP_SIGSEL_FIELD_MASK 0x04

#define TAMPER_DISABLE (63 << 2)
#define FAST_DISABLE (0 << 1)
#define AUTOWAKE_DISABLE (0 << 0)




#define CLOCK_TIME_MS( x ) \
	((x <= (1000 / CLOCK_SECOND)) ? 1 : ((x * CLOCK_SECOND) / 1000))


/**
 * @brief Wake device up
 */
static bool si7210_wakeup( I2C_Handle handle )
{
	// send the device a write request - API call will fail
	// but it has the side effect of causing device to wake up

	static uint8_t byte_out[1] = { 0x00 };
	static bool rc = 0 ;

	rc = i2c_arch_write(handle, SLV_ADDR, &byte_out, 1);

	return rc;
}


/**
 * @brief Read a single-byte register
 */

static bool si7210_readreg(I2C_Handle handle, uint8_t reg, uint8_t *value)
{
	static bool rc = false;

	rc = i2c_arch_write_read(handle, SLV_ADDR, &reg, 1, value, 1);


	return rc;
}

/**
 * @brief Read a single-byte register
 */

static PT_THREAD(si7210_read_otp_reg(struct pt *pt, uint8_t reg, bool *rc, uint8_t *value))
{

	static uint8_t bytes_out[2] = { 0 };
	static uint8_t byte_in = 0;
	static struct etimer timer = { 0 };
	static I2C_Handle handle = 0;

	PT_BEGIN(pt);

	*rc = true;

	SENSOR_AQ(pt, handle, *rc)

	bytes_out[0] = OTP_ADDR;
	bytes_out[1] = reg;
	*rc = i2c_arch_write(handle, SLV_ADDR, &bytes_out, 2);

	bytes_out[0] = OTP_CTRL;
	bytes_out[1] = 1 << 1;
	*rc &= i2c_arch_write(handle, SLV_ADDR, &bytes_out, 2);

	SENSOR_REL(pt, handle);


	// wait for it to be NOT busy
	do {

			etimer_set(&timer, CLOCK_TIME_MS(2));
			PT_WAIT_UNTIL(pt, etimer_expired(&timer));

		SENSOR_AQ(pt, handle, rc);

		bytes_out[0] = OTP_CTRL;
		*rc &= i2c_arch_write_read(handle, SLV_ADDR, &bytes_out, 1, &byte_in, 1);

		SENSOR_REL(pt, handle);

	} while ((*rc == true) && ((byte_in & 0x01) == 1));

	SENSOR_AQ(pt, handle, rc);

	bytes_out[0] = OTP_DATA;
	*rc &= i2c_arch_write_read(handle, SLV_ADDR, &bytes_out, 1, &byte_in, 1);

	*value = byte_in;

	SENSOR_REL(pt, handle);

	PT_END(pt);
}


/**
 * @brief Write a single byte register
 */

static bool si7210_writereg(I2C_Handle handle, uint8_t reg, uint8_t value)
{
	bool rc = false;


	uint8_t bytes[2] = { reg, value };
	rc = i2c_arch_write(handle, SLV_ADDR, &bytes, 2);

	return rc;
}



/**
 * @brief Check if device is responding to commands
 */
static bool si72020_check(I2C_Handle handle)
{
	bool rc = false;
	uint8_t value;

	rc = si7210_readreg(handle, HREVID, &value);


	return rc;

}


/**
 * @brief Check communications and configure the device.
 */
static PT_THREAD(si7210_init(struct pt *pt, bool *rc))
{

	static int count = 0;
	static struct etimer timer = { 0 };
	static I2C_Handle handle;

	PT_BEGIN(pt);

	SENSOR_AQ(pt, handle, *rc);

	si7210_wakeup(handle);

	SENSOR_REL(pt, handle);

	count = 5;
	do {
		SENSOR_AQ(pt, handle, *rc);

		*rc = si72020_check(handle);

		SENSOR_REL(pt, handle);


		// unit can take a long time to start up depending
		// on its state.  If its not ready, try again after
		// a 10ms delay.
		if (*rc == false) {
			etimer_set(&timer, CLOCK_TIME_MS(10));
			PT_WAIT_UNTIL(pt, etimer_expired(&timer));
		}
	} while ((--count > 0) && (*rc == false));

	if (*rc == false) {
		LOG_WARN("Could not init Si7210\n");
		PT_EXIT(pt);
	}

	SENSOR_AQ(pt, handle, *rc);

	// CTRL3 - disable periodic auto-wakeup & tamper detect
	*rc = si7210_writereg(handle, CTRL3, TAMPER_DISABLE | FAST_DISABLE | AUTOWAKE_DISABLE);

	SENSOR_REL(pt, handle);

	if (*rc == false) {
		LOG_WARN("could not write ctrl3\n");
		PT_EXIT(pt);
	}


	PT_END(pt);
}




static PT_THREAD(si7210_field_strength(struct pt *pt, bool *rc, int32_t *field))
{
	static int count = 0;
	static int32_t raw_measurement = 0;
	static uint8_t val = 0;

	static uint8_t burstSize = 7; // collect 8 samples
	static uint8_t bw = 6; // avg of 8 samples
	static uint8_t iir = 0; // FIR mode
	static I2C_Handle handle;

	static struct etimer timer = { 0 };

	PT_BEGIN(pt);

	SENSOR_AQ(pt, handle, *rc);

	// stop the unit's control loop
	*rc = si7210_writereg(handle, POWERCTL, MEAS_MASK | UCESTORE_MASK | STOP_MASK);
	*rc &= si7210_writereg(handle, POWERCTL, MEAS_MASK | UCESTORE_MASK | STOP_MASK);
	*rc &= si7210_writereg(handle, CTRL4, burstSize << 5 | bw << 1 | iir);
	*rc &= si7210_writereg(handle, DSPSIGSEL, 0);
	*rc = si7210_writereg(handle, POWERCTL, MEAS_MASK | UCESTORE_MASK | ONEBURST_MASK);

	SENSOR_REL(pt, handle);

	if (*rc == false) {
		LOG_WARN("setup measurement values.\n");
		PT_EXIT(pt);
	}


	count = 1 << (bw+2);
	do {
		SENSOR_AQ(pt, handle, *rc);
		*rc = si7210_readreg(handle,DSPSIGM, &val);
		SENSOR_REL(pt, handle);

		if (*rc == false) {
			etimer_set(&timer, CLOCK_TIME_MS(10));
			PT_WAIT_UNTIL(pt, etimer_expired(&timer));

		}
	} while ((--count > 0) && ((val & DSP_SIGM_DATA_FLAG) == 0));

	if (count == 0) {
		LOG_WARN("timed out waiting for result.\n");
		return false;
	}

	if ((val & DSP_SIGM_DATA_FLAG) == 0) {
		LOG_WARN("data flag was not set in result.\n");
		return false;
	}

	// take high bytes
	raw_measurement = (val & DSP_SIGM_DATA_MASK) << 8;

	SENSOR_AQ(pt, handle, *rc);
	*rc = si7210_readreg(handle, DSPSIGL, &val);
	SENSOR_REL(pt, handle);

	if (rc == false) {
		LOG_WARN("could not read low bits of result\n");
		return false;
	}

	raw_measurement |= val;


	LOG_DBG("Raw measurement: %d / %x\n", (int) raw_measurement, (int) raw_measurement);
	*field = raw_measurement;

	*rc = true;

	PT_END(pt);
}


int si7210_read_cal(si7210_calibration_t *cal)
{
	//TODO promote this to a named configuration value
	cal->config_range = config_get_calibration(0);

	return true;
}


static PT_THREAD(si7210_apply_compensation(struct pt *pt, bool *rc, uint8_t compRange))
{
	static uint8_t startRegs[6] = { 0x21, 0x27, 0x2d, 0x33, 0x39 };
	static uint8_t destRegs[6] = { 0xCA, 0xCB, 0xCC, 0xCE, 0xCF, 0xD0 };
	static int i = 0;
	static uint8_t regVal = 0;
	static uint8_t srcReg;
	static uint8_t dstReg;
	static struct pt child;
	static I2C_Handle handle;

	PT_BEGIN(pt);

	LOG_DBG("Applying compensation %d\n", (int) compRange);

	for (i = 0; i < 6; i++) {

		srcReg = startRegs[compRange] + i;
		dstReg = destRegs[i];

		// invoke this protothread
		PT_SPAWN(pt, &child, si7210_read_otp_reg(&child, srcReg, rc, &regVal));
		// this supposedly waits until the thread finishes??

		if (*rc == false) {
			LOG_ERR("could not set OTP regs\n");
			PT_EXIT(pt);
		}

		SENSOR_AQ(pt, handle, *rc);
		*rc &= si7210_writereg(handle, dstReg, regVal);
		SENSOR_REL(pt, handle);
	}

	PT_END(pt);
}

PROCESS(si7210_proc, "Si7210 Sensor");
PROCESS_THREAD(si7210_proc,ev, data)
{
	static si7210_data_t *sdata = 0;
	static struct pt init_thrd = { 0 };
	static struct pt comp_thrd = { 0 };
	static struct pt fs_thrd = { 0 };
	static int32_t raw_field = 0;
	static int compRange = 0;

	PROCESS_BEGIN( );

	sdata = (si7210_data_t *) data;

	PROCESS_PT_SPAWN(&init_thrd, si7210_init(&init_thrd, &(sdata->rc)));
	if (sdata->rc == false) {
		LOG_ERR("could not init\n");
		PROCESS_EXIT();
	}

	//TODO promote to named parameter
	compRange = config_get_calibration(0);
	if ((compRange < 0) || (compRange >= 6)) compRange = 0;

	PROCESS_PT_SPAWN(&comp_thrd, si7210_apply_compensation(&comp_thrd, &(sdata->rc), compRange));
	if (sdata->rc  == false) {
		LOG_ERR("Error in compensation\n");
		PROCESS_EXIT();
	}


	PROCESS_PT_SPAWN(&fs_thrd, si7210_field_strength(&fs_thrd, &(sdata->rc), &raw_field));
	if (sdata->rc == false) {
		sdata->magfield = 0;
		LOG_ERR("could not read mag file\n");
		PROCESS_EXIT();
	}
	else {
		sdata->magfield = (int16_t) raw_field;
	}



	PROCESS_END();
}

