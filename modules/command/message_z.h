/*
 * Message formats for Zephyr stack - same layout as message.h for backend compatibility.
 * No Contiki types; IPv6 address as uint16_t[8].
 */
#ifndef MODULES_COMMAND_MESSAGE_Z_H_
#define MODULES_COMMAND_MESSAGE_Z_H_

#include <stdint.h>
#include "config_z.h"

#define CMD_SET_HEADER_Z (0x0beed1eU)

typedef enum {
	CMD_SET_CONFIG,
	CMD_REQ_CONFIG
} command_type_t;

typedef struct __attribute__((__packed__)) {
	uint32_t header;
	command_type_t config_type;
	config_type_t token;
	union {
		uint32_t intval;
		uint16_t address[8];
	} value;
} command_set_t;

#define CMD_RET_HEADER_Z (0xdde323f3U)
typedef struct __attribute__((__packed__)) {
	uint32_t header;
	config_type_t token;
	uint8_t valid;
	uint32_t length;
	union {
		uint32_t uivalue;
		unsigned long ulong;
		uint16_t ipaddr[8];
		char buff[32];
		int32_t ivalue;
	} value;
} command_ret_t;

#define WATER_CAL_HEADER_Z (0x3536370U)
typedef struct __attribute__((packed)) {
	uint32_t header;
	uint32_t sequence;
	int32_t rssi;
	uint16_t caldata[6];
	uint16_t resistorVals[8];
	uint16_t si7210_offset;
	uint16_t si7210_gain;
} water_cal_t;

#define WATER_DATA_HEADER_Z (0x34323233U)
typedef struct __attribute__((packed)) {
	uint32_t header;
	uint32_t sequence;
	int32_t rssi;
	uint32_t pressure;
	uint32_t temppressure;
	uint16_t battery;
	uint16_t color_blue;
	uint16_t color_clear;
	uint16_t color_green;
	uint16_t color_red;
	uint16_t ambient;
	uint16_t range1;
	uint16_t range2;
	uint16_t range3;
	uint16_t range4;
	uint16_t range5;
	uint16_t temperature;
	int16_t hall;
} water_data_t;

#define AIRBORNE_HEADER_Z (0x54ab23efU)
typedef struct __attribute__((packed)) {
	uint32_t header;
	uint32_t sequence;
	int32_t rssi;
	uint32_t ms5637_pressure;
	uint32_t ms5637_temp;
	uint32_t si7020_humid;
	uint32_t si7020_temp;
	uint16_t battery;
	uint16_t i2cerror;
} airborne_t;

#define ACK_HEADER_Z (0x90983323U)
typedef struct __attribute__((packed)) {
	uint32_t header;
	uint32_t sequence;
	uint32_t ack_seq;
	uint32_t ack_value;
} ack_t;

#endif /* MODULES_COMMAND_MESSAGE_Z_H_ */
