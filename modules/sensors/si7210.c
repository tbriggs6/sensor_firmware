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


/**
 * @brief Wake device up
 */
static bool si7210_wakeup(I2C_Handle handle)
{
	// send the device a write request - API call will fail
	// but it has the side effect of causing device to wake up

	uint8_t byte_out[1] = { 0x00 };

	return i2c_arch_write(handle, SLV_ADDR, &byte_out, 1);
}


/**
 * @brief Read a single-byte register
 */

static bool si7210_readreg(I2C_Handle i2c_handle, uint8_t reg, uint8_t *value)
{
	bool rc = false;

	rc = i2c_arch_write_read(i2c_handle, SLV_ADDR, &reg, 1, value, 1);
	return rc;
}

/**
 * @brief Write a single byte register
 */

static bool si7210_writereg(I2C_Handle i2c_handle, uint8_t reg, uint8_t value)
{
	bool rc = false;

	uint8_t bytes[2] = { reg, value };
	rc = i2c_arch_write(i2c_handle, SLV_ADDR, &bytes, 2);

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
static bool si7210_init(I2C_Handle handle)
{
	int count = 0;
	bool rc = false;

	si7210_wakeup(handle);


	count = 5;
	do {
		rc = si72020_check(handle);

		// unit can take a long time to start up depending
		// on its state.  If its not ready, try again after
		// a 10ms delay.
		if (rc == false) {
			clock_delay_usec(10000);
		}
	} while ((--count > 0) && (rc == false));

	if (rc == false) {
		LOG_WARN("Could not init Si7210\n");
		return false;
	}

	// CTRL3 - disable periodic auto-wakeup & tamper detect
	rc = si7210_writereg(handle, CTRL3, TAMPER_DISABLE | FAST_DISABLE | AUTOWAKE_DISABLE);
	if (rc == false) {
		LOG_WARN("could not write ctrl3\n");
		return false;
	}
	return true;
}


/**
 * @brief Check communications and configure the device.
 */
static bool si7210_deinit(I2C_Handle handle)
{
	bool rc = si7210_writereg(handle, POWERCTL, MEAS_MASK | SLEEP_MASK);
	if (rc == false) {
			LOG_WARN("could not write power control register.\n");
		}
	return rc;
}


static bool si7210_field_strength(I2C_Handle handle, int32_t *field)
{
	int count = 0;
	bool rc = false;
	int32_t raw_measurement = 0;
	uint8_t val = 0;

	// stop the unit's control loop
	rc = si7210_writereg(handle, POWERCTL, MEAS_MASK | UCESTORE_MASK | STOP_MASK);
	if (rc == false) {
		LOG_WARN("could not write power control register.\n");
		return false;
	}

	//TODO load compensation values from OTP

	// configure for measurement mode
	//TODO play with these values for better results
	uint8_t burstSize = 8; // collect 8 samples
	uint8_t bw = 3; // avg of 8 samples
	uint8_t iir = 0; // FIR mode

	rc = si7210_writereg(handle, CTRL4, burstSize << 5 | bw << 1 | iir);
	if (rc == false) {
		LOG_WARN("could not write burst configuration.");
		return false;
	}

	// configure for field strength
	rc = si7210_writereg(handle, DSPSIGSEL, 0);
	if (rc == false) {
		LOG_WARN("could not write measurement selection\n");
			return false;
	}

	// configure for a measurement
	rc = si7210_writereg(handle, POWERCTL, MEAS_MASK | UCESTORE_MASK | ONEBURST_MASK);
	if (rc == false) {
		LOG_WARN("could not start measurementr.\n");
		return false;
	}

	count = 5;
	do {
		rc = si7210_readreg(handle, DSPSIGM, &val);
		if (rc == false) {
			clock_delay_usec(10000);
		}
	} while ((--count > 0) && (rc == false));

	if (rc == false) {
		LOG_WARN("timed out waiting for result.\n");
		return false;
	}

	if ((val & DSP_SIGM_DATA_FLAG) == 0) {
		LOG_WARN("data flag was not set in result.\n");
		return false;
	}

	// take high bytes
	raw_measurement = (val & DSP_SIGM_DATA_MASK) << 8;

	rc = si7210_readreg(handle, DSPSIGL, &val);
	if (rc == false) {
		LOG_WARN("could not read low bits of result\n");
		return false;
	}

	raw_measurement |= val;


	LOG_DBG("Raw measurement: %d / %x\n", (int) raw_measurement, (int) raw_measurement);
	*field = raw_measurement;

	return true;
}



int si7210_read (int16_t *magfield)
{
	bool rc = false;

	I2C_Handle handle = i2c_arch_acquire (I2CBUS);
	if (handle == NULL) {
		LOG_ERR("Could not aquiare I2C bus.\n");
		return false;
	}


	rc = si7210_init(handle);
	if (rc == false) {
		LOG_ERR("could not init\n");
		goto error;
	}


	int32_t raw_field = 0;
	rc = si7210_field_strength(handle, &raw_field);
	if (rc == false) {
		*magfield = 0;
		LOG_ERR("could not read mag file\n");
		goto error;
	}
	else {
		*magfield = (int16_t) raw_field;
	}
	si7210_deinit(handle);



error:
	i2c_arch_release(handle);
	return rc;
}

