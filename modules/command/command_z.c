/*
 * Command handler - Zephyr implementation using config_z.
 */
#include "command_z.h"
#include "message_z.h"
#include "../config/config_z.h"
#include "../messenger/messenger_z.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(command_z, CONFIG_LOG_DEFAULT_LEVEL);

static void command_handle_set(const command_set_t *req, command_ret_t *ret, int *num_bytes)
{
	int id;

	ret->header = CMD_RET_HEADER_Z;
	ret->token = req->token;
	ret->length = 4;

	switch (req->token) {
	case CONFIG_CTIME:
		config_z_set_ctime_offset((int)req->value.intval);
		ret->value.uivalue = (uint32_t)config_z_get_ctime_offset();
		ret->valid = (ret->value.uivalue == (uint32_t)req->value.intval) ? 1 : 0;
		break;
	case CONFIG_ROUTER: {
		config_receiver_t recv;
		memcpy(recv.server, req->value.address, sizeof(recv.server));
		config_z_set_receiver(&recv);
		config_z_get_receiver(&recv);
		ret->valid = (memcmp(recv.server, req->value.address, sizeof(recv.server)) == 0) ? 1 : 0;
		memcpy(ret->value.ipaddr, recv.server, sizeof(ret->value.ipaddr));
		ret->length = sizeof(ret->value.ipaddr);
		break;
	}
	case CONFIG_SENSOR_INTERVAL:
		config_z_set_sensor_interval((int)req->value.intval);
		ret->value.uivalue = (uint32_t)config_z_get_sensor_interval();
		ret->valid = (ret->value.uivalue == (uint32_t)req->value.intval) ? 1 : 0;
		config_z_timeout_change();
		break;
	case CONFIG_MAX_FAILURES:
		config_z_set_max_failures((uint32_t)req->value.intval);
		ret->value.uivalue = config_z_get_max_failures();
		ret->valid = (ret->value.uivalue == (uint32_t)req->value.intval) ? 1 : 0;
		break;
	case CONFIG_RETRY_INTERVAL:
		config_z_set_retry_interval((uint32_t)req->value.intval);
		ret->value.uivalue = config_z_get_retry_interval();
		ret->valid = (ret->value.uivalue == (uint32_t)req->value.intval) ? 1 : 0;
		config_z_timeout_change();
		break;
	case CONFIG_CAL1:
	case CONFIG_CAL2:
	case CONFIG_CAL3:
	case CONFIG_CAL4:
	case CONFIG_CAL5:
	case CONFIG_CAL6:
	case CONFIG_CAL7:
	case CONFIG_CAL8:
		id = (int)(req->token - CONFIG_CAL1);
		config_z_set_calibration(id, (uint16_t)req->value.intval);
		ret->value.uivalue = config_z_get_calibration(id);
		ret->valid = (ret->value.uivalue == (uint32_t)req->value.intval) ? 1 : 0;
		break;
	default:
		ret->valid = 0;
		break;
	}
	*num_bytes = (int)sizeof(command_ret_t);
}

static void command_handle_get(const command_set_t *req, command_ret_t *ret, int *num_bytes)
{
	int idx;

	ret->header = CMD_RET_HEADER_Z;
	ret->token = req->token;
	ret->length = 0;
	ret->valid = 1;

	switch (req->token) {
	case CONFIG_CTIME:
		ret->value.uivalue = (uint32_t)config_z_get_ctime_offset();
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_ROUTER: {
		config_receiver_t recv;
		config_z_get_receiver(&recv);
		memcpy(ret->value.ipaddr, recv.server, sizeof(ret->value.ipaddr));
		ret->length += sizeof(ret->value.ipaddr);
		ret->valid = (memcmp(recv.server, req->value.address, sizeof(recv.server)) == 0) ? 1 : 0;
		break;
	}
	case CONFIG_DEVTYPE:
		ret->value.uivalue = (uint32_t)config_z_get_devtype();
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_UPTIME:
		ret->value.ulong = (unsigned long)k_uptime_sec();
		ret->length += sizeof(ret->value.ulong);
		break;
	case CONFIG_SENSOR_INTERVAL:
		ret->value.uivalue = (uint32_t)config_z_get_sensor_interval();
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_MAX_FAILURES:
		ret->value.uivalue = config_z_get_max_failures();
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_RETRY_INTERVAL:
		ret->value.uivalue = config_z_get_retry_interval();
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_CAL1:
	case CONFIG_CAL2:
	case CONFIG_CAL3:
	case CONFIG_CAL4:
	case CONFIG_CAL5:
	case CONFIG_CAL6:
	case CONFIG_CAL7:
	case CONFIG_CAL8:
		idx = (int)(req->token - CONFIG_CAL1);
		ret->value.uivalue = config_z_get_calibration(idx);
		ret->length += sizeof(ret->value.uivalue);
		break;
	case CONFIG_ENERGEST_CPU:
	case CONFIG_ENERGEST_LPM:
	case CONFIG_ENERGEST_TRANSMIT:
	case CONFIG_ENERGEST_LISTEN:
	case CONFIG_ENERGEST_DEEP_LPM:
		/* Stub: 0% */
		ret->value.uivalue = 0;
		ret->length += sizeof(ret->value.uivalue);
		break;
	default:
		ret->length = 0;
		ret->valid = 0;
		break;
	}
	*num_bytes = 13 + (int)ret->length;
}

int command_z_handler(const uint8_t *inputdata, int inputlength,
		      uint8_t *outputdata, int *maxoutputlen)
{
	const command_set_t *req;
	command_ret_t *response;

	if (inputdata == NULL || outputdata == NULL || maxoutputlen == NULL ||
	    *maxoutputlen < (int)sizeof(command_ret_t)) {
		return 0;
	}
	if (inputlength < (int)sizeof(command_set_t)) {
		*maxoutputlen = 0;
		return 0;
	}

	req = (const command_set_t *)inputdata;
	response = (command_ret_t *)outputdata;
	*maxoutputlen = sizeof(command_ret_t);

	if (req->config_type == CMD_SET_CONFIG) {
		command_handle_set(req, response, maxoutputlen);
	} else if (req->config_type == CMD_REQ_CONFIG) {
		command_handle_get(req, response, maxoutputlen);
	} else {
		*maxoutputlen = 0;
		return 0;
	}
	return *maxoutputlen;
}

void command_z_register(void)
{
	messenger_z_add_handler(CMD_SET_HEADER_Z,
				(uint32_t)sizeof(command_set_t),
				(uint32_t)sizeof(command_set_t),
				command_z_handler);
}
