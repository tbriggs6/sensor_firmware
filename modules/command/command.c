#include <stdint.h>
#include <contiki.h>
#include "../../modules/messenger/message-service.h"
#include "command.h"
#include "message.h"
#include <sys/energest.h>

#define DEBUG

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "CONFIG"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif



static void command_handle_set(const command_set_t *const req, command_ret_t *ret, int *num_bytes)
{
	ret->header = CMD_RET_HEADER;
	ret->token = req->token;
	ret->length = 4 * sizeof(uint32_t);

	switch(req->token) {
	case CONFIG_CTIME:
		LOG_INFO("Set CONFIG_CTIME...%d\n", (int) req->value.intval);
		config_set_ctime_offset(req->value.intval);
		ret->value.uivalue = config_get_ctime_offset( );
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;
		break;

	case CONFIG_ROUTER:
		LOG_INFO("Set CONFIG_ROUTER...");
		LOG_6ADDR(LOG_LEVEL_INFO,&req->value.address);
		LOG_INFO_("\n");

		config_set_receiver(&req->value.address);
		config_get_receiver(&ret->value.ipaddr);
		ret->valid = uip_ipaddr_cmp(&ret->value.ipaddr, &req->value.address);
		ret->length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		LOG_INFO("Set CONFIG_BROADCAST_INTERVAL...%d\n", (int) req->value.intval);

		config_set_bcast_interval(req->value.intval);
		ret->value.uivalue = config_get_bcast_interval();
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;
		break;

	case CONFIG_SENSOR_INTERVAL:
		LOG_INFO("Set CONFIG_SENSOR_INTERVAL...%d\n", (int) req->value.intval);
		config_set_sensor_interval(req->value.intval);
		ret->value.uivalue = config_get_sensor_interval( );
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		LOG_INFO("Set CONFIG_NEIGHBOR_INTERVAL...%d\n", (int) req->value.intval);
		config_set_neighbor_interval(req->value.intval);
		ret->value.uivalue = config_get_neighbor_interval( );
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;
		break;

	default:
		LOG_ERR("Command %X does not match any known commands.\n", req->token);
		ret->valid = 0;
		break;
	}

	*num_bytes = ret->length;
}


static void command_handle_get(const command_set_t *const req, command_ret_t *ret, int *num_bytes)
{
	ret->header = CMD_RET_HEADER;
	ret->token = req->token;
	ret->length = 0; // this will need to be modified
	ret->valid = 1;

	switch(req->token) {
	case CONFIG_CTIME:
		LOG_INFO("Get CONFIG_CTIME...\n");
		ret->value.uivalue = config_get_ctime_offset( );
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ROUTER:
		LOG_INFO("Get CONFIG_ROUTER...\n");
		config_get_receiver(&(ret->value.ipaddr));
		ret->valid = uip_ipaddr_cmp(&(ret->value.ipaddr), &req->value.address);
		ret->length += sizeof(ret->value.ipaddr);
		break;

	case CONFIG_DEVTYPE:
		LOG_INFO("Get CONFIG_DEVTYPE...\n");
		ret->value.uivalue = config_get_devtype();
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_UPTIME:
		LOG_INFO("Get CONFIG_UPTIME...\n");
		ret->value.ulong = (unsigned long) clock_seconds(); // get current uptime
		ret->length += sizeof(ret->value.ulong);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		LOG_INFO("Get CONFIG_BROADCAST_INTERVAL...\n");
		ret->value.uivalue = config_get_bcast_interval();
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_SENSOR_INTERVAL:
		LOG_INFO("Get CONFIG_SENSOR_INTERVAL...\n");
		ret->value.uivalue = config_get_sensor_interval( );
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		LOG_INFO("Get CONFIG_NEIGHBOR_INTERVAL...\n");
		ret->value.uivalue = config_get_neighbor_interval( );
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_NEIGHBOR_NUM:
		LOG_INFO("Get CONFIG_NEIGHBOR_NUM...\n");
		//ret->value.uivalue = neighbors_getnum( );
		ret->value.uivalue = 0;
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_NEIGHBOR_POS:
		LOG_INFO("Get CONFIG_NEIGHBOR_POS...\n");
		//neighbors_get(req->value.intval, &ret->value.ipaddr);
		ret->value.uivalue = 0;
		ret->length += sizeof(ret->value.ipaddr);
		break;

	case CONFIG_NEIGHBOR_RSSI:
		LOG_INFO("Get CONFIG_NEIGHBOR_RSSI...\n");
		//ret->value.uivalue = neighbors_get_rssi(req->value.intval);
		ret->value.uivalue = 0;
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_NEIGHBOR_ROUTER:
		LOG_INFO("Get CONFIG_NEIGHBOR_ROUTER...\n");
		//ret->value.uivalue = neighbors_get_router(req->value.intval);
		ret->value.uivalue = 0;
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_RTIMER_SECOND: // I don't know what this is supposed to do. -- Mic
		LOG_INFO("Get CONFIG_RTIMER_SECOND...\n");
		ret->value.uivalue = 0xFF;
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_CPU:
		LOG_INFO("Get CONFIG_ENERGEST_CPU...\n");
		ret->value.uivalue = energest_type_time(ENERGEST_TYPE_CPU);
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_LPM:
		LOG_INFO("Get CONFIG_ENERGEST_LPM...\n");
		ret->value.uivalue = energest_type_time(ENERGEST_TYPE_LPM);
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_TRANSMIT:
		LOG_INFO("Get CONFIG_ENERGEST_TRANSMIT...\n");
		ret->value.uivalue = energest_type_time(ENERGEST_TYPE_TRANSMIT);
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_LISTEN:
		LOG_INFO("Get CONFIG_ENERGEST_LISTEN...\n");
		ret->value.uivalue = energest_type_time(ENERGEST_TYPE_LISTEN);
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_MAX:
		LOG_INFO("Get CONFIG_ENERGEST_MAX...\n");
		ret->value.uivalue = energest_type_time(ENERGEST_TYPE_MAX);
		ret->length += sizeof(ret->value.uivalue);
		break;

	default:
		LOG_ERR("Get unknown token %x\n", req->token);
		ret->length = 0;
		ret->valid = 0;
		break;
	}

	*num_bytes = 13 + ret->length;

}

int command_handler(const uint8_t *inputdata, int inputlength, uint8_t *outputdata, int *maxoutputlen)
{
	const command_set_t *req = (const command_set_t *) inputdata;
	command_ret_t *response = (command_ret_t *) outputdata;

	if (*maxoutputlen < sizeof(command_ret_t)) {
		LOG_ERR("Error - output buffer is too small!!\n");
		return 0;
	}

	*maxoutputlen = sizeof(command_ret_t);

	if (req->config_type == CMD_SET_CONFIG) {
		command_handle_set(req,response, maxoutputlen);
	}
	else if (req->config_type == CMD_REQ_CONFIG) {
		command_handle_get(req, response, maxoutputlen);
	}
	else {
		LOG_ERR("Error - not a valid command\n");
		*maxoutputlen = 0;
		return 0;
	}

	return *maxoutputlen;
}


void command_init( )
{

    messenger_add_handler(0x0beed1eU,   4,  sizeof(command_set_t), command_handler);
}
