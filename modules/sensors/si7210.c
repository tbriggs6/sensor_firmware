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
static bool si7210_wakeup( void )
{
	// send the device a write request - API call will fail
	// but it has the side effect of causing device to wake up

	uint8_t byte_out[1] = { 0x00 };

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
		if (handle == NULL) {
			LOG_ERR("Could not aquiare I2C bus.\n");
			return false;
		}


	bool rc = i2c_arch_write(handle, SLV_ADDR, &byte_out, 1);

	i2c_arch_release(handle);
	return rc;
}


/**
 * @brief Read a single-byte register
 */

static bool si7210_readreg(uint8_t reg, uint8_t *value)
{
	bool rc = false;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
		if (handle == NULL) {
			LOG_ERR("Could not aquiare I2C bus.\n");
			return false;
		}


	rc = i2c_arch_write_read(handle, SLV_ADDR, &reg, 1, value, 1);

	i2c_arch_release(handle);

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

	PT_BEGIN(pt);

	*rc = true;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
		if (handle == NULL) {
			LOG_ERR("Could not aquiare I2C bus.\n");
			*rc = false;
			PT_EXIT(pt);
		}


	bytes_out[0] = OTP_ADDR;
	bytes_out[1] = reg;
	rc = i2c_arch_write(handle, SLV_ADDR, &bytes_out, 2);

	bytes_out[0] = OTP_CTRL;
	bytes_out[1] = 1 << 1;
	rc &= i2c_arch_write(handle, SLV_ADDR, &bytes_out, 2);

	i2c_arch_release(handle);

	// wait for it to be NOT busy
	do {

			etimer_set(&timer, CLOCK_TIME_MS(2));
			PT_WAIT_UNTILpt, etimer_expired(&timer));

			handle = i2c_arch_acquire (I2CBUS);
			if (handle == NULL) {
				LOG_ERR("Could not aquiare I2C bus.\n");
				*rc = false;
				PT_EXIT(pt);
			}

		bytes_out[0] = OTP_CTRL;
		rc &= i2c_arch_write_read(handle, SLV_ADDR, &bytes_out, 1, &byte_in, 1);

		i2c_arch_release(handle);

	} while ((rc == true) && ((byte_in & 0x01) == 1));

	handle = i2c_arch_acquire (I2CBUS);
			if (handle == NULL) {
				LOG_ERR("Could not aquiare I2C bus.\n");
				*rc = false;
				PT_EXIT(pt);
			}

	bytes_out[0] = OTP_DATA;
	rc &= i2c_arch_write_read(handle, SLV_ADDR, &bytes_out, 1, &byte_in, 1);

	*value = byte_in;

	i2c_arch_release(handle);

	PT_EXIT(pt);
}



/**
 * @brief Write a single byte register
 */

static bool si7210_writereg(uint8_t reg, uint8_t value)
{
	bool rc = false;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
			if (handle == NULL) {
				LOG_ERR("Could not aquiare I2C bus.\n");
				return false;
			}

	uint8_t bytes[2] = { reg, value };
	rc = i2c_arch_write(handle, SLV_ADDR, &bytes, 2);

	i2c_arch_release(handle);

	return rc;
}



/**
 * @brief Check if device is responding to commands
 */
static bool si72020_check()
{
	bool rc = false;
	uint8_t value;

	rc = si7210_readreg(HREVID, &value);


	return rc;

}


/**
 * @brief Check communications and configure the device.
 */
static PT_THREAD(si7210_init(struct pt *pt, bool *rc))
{

	static int count = 0;
	static etimer timer = { 0 };

	PT_BEGIN(pt);

	si7210_wakeup();

	count = 5;
	do {
		*rc = si72020_check();

		// unit can take a long time to start up depending
		// on its state.  If its not ready, try again after
		// a 10ms delay.
		if (*rc == false) {
			etimer_set(&timer, CLOCK_TIMER_MS(10));
			PT_WAIT_UNTIL(pt, etimer_expired(&timer));
		}
	} while ((--count > 0) && (*rc == false));

	if (*rc == false) {
		LOG_WARN("Could not init Si7210\n");
		PT_EXIT(pt);
	}

	// CTRL3 - disable periodic auto-wakeup & tamper detect
	*rc = si7210_writereg(CTRL3, TAMPER_DISABLE | FAST_DISABLE | AUTOWAKE_DISABLE);
	if (*rc == false) {
		LOG_WARN("could not write ctrl3\n");
		PT_EXIT(pt);
	}

	PT_EXIT(pt);
}


/**
 * @brief Check communications and configure the device.
 */
static bool si7210_deinit()
{
	bool rc = si7210_writereg(POWERCTL, MEAS_MASK | SLEEP_MASK);
	if (rc == false) {
			LOG_WARN("could not write power control register.\n");
		}
	return rc;
}


static PT_THREAD(si7210_field_strength(struct pt *pt, bool *rc, int32_t *field))
{
	static int count = 0;
	static int32_t raw_measurement = 0;
	static uint8_t val = 0;

	PT_BEGIN(pt);

	// stop the unit's control loop
	rc = si7210_writereg(POWERCTL, MEAS_MASK | UCESTORE_MASK | STOP_MASK);
	if (rc == false) {
		LOG_WARN("could not write power control register.\n");
		return false;
	}


	// configure for measurement mode
	//TODO play with these values for better results
	uint8_t burstSize = 7; // collect 8 samples
	uint8_t bw = 6; // avg of 8 samples
	uint8_t iir = 0; // FIR mode

	rc = si7210_writereg(CTRL4, burstSize << 5 | bw << 1 | iir);
	if (rc == false) {
		LOG_WARN("could not write burst configuration.");
		return false;
	}

	// configure for field strength
	rc = si7210_writereg(DSPSIGSEL, 0);
	if (rc == false) {
		LOG_WARN("could not write measurement selection\n");
			return false;
	}

	// configure for a measurement
	rc = si7210_writereg(POWERCTL, MEAS_MASK | UCESTORE_MASK | ONEBURST_MASK);
	if (rc == false) {
		LOG_WARN("could not start measurementr.\n");
		return false;
	}

	count = 1 << (bw+2);
	do {
		rc = si7210_readreg(DSPSIGM, &val);
		if (rc == false) {
			clock_delay_usec(10000);
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

	rc = si7210_readreg(DSPSIGL, &val);
	if (rc == false) {
		LOG_WARN("could not read low bits of result\n");
		return false;
	}

	raw_measurement |= val;


	LOG_DBG("Raw measurement: %d / %x\n", (int) raw_measurement, (int) raw_measurement);
	*field = raw_measurement;

	return true;
}


int si7210_read_cal(si7210_calibration_t *cal)
{
	//TODO promote this to a named configuration value
	cal->config_range = config_get_calibration(0);

	return true;
}


static int si7210_apply_compensation(uint8_t compRange)
{
	uint8_t startRegs[6] = { 0x21, 0x27, 0x2d, 0x33, 0x39 };
	uint8_t destRegs[6] = { 0xCA, 0xCB, 0xCC, 0xCE, 0xCF, 0xD0 };
	int i = 0;
	int rc = true;
	uint8_t regVal = 0;
	uint8_t srcReg;
	uint8_t dstReg;

	LOG_DBG("Applying compensation %d\n", (int) compRange);

	for (i = 0; i < 6; i++) {

		srcReg = startRegs[compRange] + i;
		dstReg = destRegs[i];

		rc &= si7210_read_otp_reg(srcReg, &regVal);
		if (rc == false) {
			LOG_ERR("could not set OTP regs\n");
			return false;
		}

		printf("R[%x] = R[%x]/%x\n", dstReg, srcReg, regVal);
		rc &= si7210_writereg(dstReg, regVal);
	}

	return rc;
}

int si7210_read (int16_t *magfield)
{
	bool rc = false;


	rc = si7210_init();
	if (rc == false) {
		LOG_ERR("could not init\n");
		goto error;
	}

	//TODO promote to named parameter
	int compRange = config_get_calibration(0);
	if ((compRange < 0) || (compRange >= 6)) compRange = 0;

	si7210_apply_compensation(compRange);


	int32_t raw_field = 0;
	rc = si7210_field_strength( &raw_field);
	if (rc == false) {
		*magfield = 0;
		LOG_ERR("could not read mag file\n");
		goto error;
	}
	else {
		*magfield = (int16_t) raw_field;
	}


	// No need to read temperature - comes from other
	// sensors, AND, this value is already compensated.

	si7210_deinit();



error:
	return rc;
}

