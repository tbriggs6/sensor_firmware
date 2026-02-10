/*
 * Host test: verify Zephyr message struct layout for backend compatibility.
 * Compile from repo root or tests/baseline with -I pointing to modules.
 * Run: ./message_struct_test
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

/* Include Zephyr message formats (no Zephyr dependency) */
#include "config_z.h"
#include "messenger_z.h"
#include "message_z.h"

int main(void)
{
	/* Headers must match backend expectations */
	assert(WATER_DATA_HEADER_Z == 0x34323233U);
	assert(WATER_CAL_HEADER_Z == 0x3536370U);
	assert(AIRBORNE_HEADER_Z == 0x54ab23efU);
	assert(ACK_HEADER_Z == 0x90983323U);
	assert(CMD_SET_HEADER_Z == 0x0beed1eU);
	assert(CMD_RET_HEADER_Z == 0xdde323f3U);

	/* Struct sizes: ensure no accidental padding change */
	assert(sizeof(water_data_t) >= 44 && sizeof(water_data_t) <= 64);
	assert(sizeof(water_cal_t) >= 32 && sizeof(water_cal_t) <= 48);
	assert(sizeof(airborne_t) >= 28 && sizeof(airborne_t) <= 36);
	assert(sizeof(ack_t) == 16);
	assert(sizeof(command_set_t) >= 12 && sizeof(command_set_t) <= 24);
	assert(sizeof(command_ret_t) >= 12 && sizeof(command_ret_t) <= 52);

	/* Key field offsets for water_data_t (packed) */
	assert(sizeof(uint32_t) == 4);
	assert(offsetof(water_data_t, header) == 0);
	assert(offsetof(water_data_t, sequence) == 4);
	assert(offsetof(water_data_t, pressure) == 12);
	assert(offsetof(water_data_t, battery) == 20);
	assert(offsetof(water_data_t, hall) == 44);
	assert(sizeof(water_data_t) == 46);

	/* IPv6 as 8 x uint16_t */
	assert(sizeof(config_receiver_t) == 16);
	assert(sizeof(messenger_addr_t) == 16);

	printf("message_struct_test: all checks passed\n");
	return 0;
}
