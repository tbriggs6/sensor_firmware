/*
 * confighandler.c
 *
 *  Created on: Dec 4, 2017
 *      Author: contiki
 */

#include <message-service.h>
#include "message.h"
#include "config.h"
#include "neighbors.h"
#include "commandhandler.h"


#ifdef PRINTF
#undef PRINTF
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static void command_handle_set(const command_set_t const *req, const uip_ipaddr_t const *remote_addr, const int remote_port)
{
	static command_ret_t ret;

	ret.header = CMD_RET_HEADER;
	ret.token = req->token;
	ret.length = 4 * sizeof(uint32_t);

	switch(req->token) {
	case CONFIG_CTIME:
		config_set_ctime_offset(req->value.intval);
		ret.value.uivalue = config_get_ctime_offset( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_ROUTER:
		config_set_receiver(&req->value.address);
		config_get_receiver(&ret.value.ipaddr);
		ret.valid = uip_ipaddr_cmp(&ret.value.ipaddr, &req->value.address);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		config_set_bcast_interval(req->value.intval);
		ret.value.uivalue = config_get_bcast_interval();
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_SENSOR_INTERVAL:
		config_set_sensor_interval(req->value.intval);
		ret.value.uivalue = config_get_sensor_interval( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		config_set_neighbor_interval(req->value.intval);
		ret.value.uivalue = config_get_neighbor_interval( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	default:
		ret.valid = 0;
		break;
	}

	messenger_send(remote_addr, &ret, ret.length);
}


static void command_handle_get(const command_set_t *const req, uip_ipaddr_t *remote_addr, int remote_port)
{
	static command_ret_t ret;

	ret.header = CMD_RET_HEADER;
	ret.token = req->token;
	ret.length = 0;
	ret.valid = 1;

	switch(req->token) {
	case CONFIG_CTIME:
		ret.value.uivalue = config_get_ctime_offset( );
		ret.length = 4;
		break;

	case CONFIG_ROUTER:
		config_get_receiver(&ret.value.ipaddr);
		ret.valid = uip_ipaddr_cmp(&ret.value.ipaddr, &req->value.address);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_DEVTYPE:
		ret.value.uivalue = config_get_devtype();
		ret.length = 4;
		break;

	case CONFIG_UPTIME:
		ret.value.uivalue = 0;
		ret.length = 4;
		break;

	case CONFIG_BROADCAST_INTERVAL:
		ret.value.uivalue = config_get_bcast_interval();
		ret.length = 4;
		break;

	case CONFIG_SENSOR_INTERVAL:
		ret.value.uivalue = config_get_sensor_interval( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		ret.value.uivalue = config_get_neighbor_interval( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_NUM:
		ret.value.uivalue = neighbors_getnum( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_POS:
		neighbors_get(req->value.intval, &ret.value.ipaddr);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_NEIGHBOR_RSSI:
		ret.value.uivalue = neighbors_get_rssi(req->value.intval);
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_ROUTER:
		ret.value.uivalue = neighbors_get_router(req->value.intval);
		ret.length = 4;
		break;

	default:
		ret.valid = 0;
		break;
	}

	messenger_send(remote_addr, &ret, ret.length);
}

void command_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{

	command_set_t *req = (command_set_t *) data;

	if (req->config_type == CMD_SET_CONFIG) {
		command_handle_set(req,remote_addr,remote_port);
	}
	else if (req->config_type == CMD_REQ_CONFIG) {
		command_handle_get(req, remote_addr, remote_port);
	}
	else {
		PRINTF("Error - not a valid command\n");
	}



}
