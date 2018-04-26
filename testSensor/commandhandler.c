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
		PRINTF("[COMMAND HANDLE SET] Token is CONFIG_CTIME...\r\n");
		config_set_ctime_offset(req->value.intval);
		ret.value.uivalue = config_get_ctime_offset( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_ROUTER:
		PRINTF("[COMMAND HANDLE SET] Token is CONFIG_ROUTER...\r\n");
		config_set_receiver(&req->value.address);
		config_get_receiver(&ret.value.ipaddr);
		ret.valid = uip_ipaddr_cmp(&ret.value.ipaddr, &req->value.address);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		PRINTF("[COMMAND HANDLE SET] Token is CONFIG_BROADCAST_INTERVAL...\r\n");
		config_set_bcast_interval(req->value.intval);
		ret.value.uivalue = config_get_bcast_interval();
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_SENSOR_INTERVAL:
		PRINTF("[COMMAND HANDLE SET] Token is CONFIG_SENSOR_INTERVAL...\r\n");
		config_set_sensor_interval(req->value.intval);
		ret.value.uivalue = config_get_sensor_interval( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		PRINTF("[COMMAND HANDLE SET] Token is CONFIG_NEIGHBOR_INTERVAL...\r\n");
		config_set_neighbor_interval(req->value.intval);
		ret.value.uivalue = config_get_neighbor_interval( );
		ret.valid = (ret.value.uivalue == req->value.intval) ? 1 : 0;
		ret.length = 4;
		break;

	default:
		PRINTF("[CMD HANDLE SET] ERROR - Command %X does not match any known commands.\r\n", req->token);
		ret.valid = 0;
		break;
	}

	commander_send(remote_addr, &ret, ret.length);
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
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_CTIME...\r\n");
		ret.value.uivalue = config_get_ctime_offset( );
		ret.length = 4;
		break;

	case CONFIG_ROUTER:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_ROUTER...\r\n");
		config_get_receiver(&ret.value.ipaddr);
		ret.valid = uip_ipaddr_cmp(&ret.value.ipaddr, &req->value.address);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_DEVTYPE:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_DEVTYPE...\r\n");
		ret.value.uivalue = config_get_devtype();
		ret.length = 4;
		break;

	case CONFIG_UPTIME:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_UPTIME...\r\n");
		ret.value.uivalue = 0;
		ret.length = sizeof(command_ret_t);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_BROADCAST_INTERVAL...\r\n");
		ret.value.uivalue = config_get_bcast_interval();
		ret.length = 4;
		break;

	case CONFIG_SENSOR_INTERVAL:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_SENSOR_INTERVAL...\r\n");
		ret.value.uivalue = config_get_sensor_interval( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_NEIGHBOR_INTERVAL...\r\n");
		ret.value.uivalue = config_get_neighbor_interval( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_NUM:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_NEIGHBOR_NUM...\r\n");
		ret.value.uivalue = neighbors_getnum( );
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_POS:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_NEIGHBOR_POS...\r\n");
		neighbors_get(req->value.intval, &ret.value.ipaddr);
		ret.length = sizeof(uip_ipaddr_t);
		break;

	case CONFIG_NEIGHBOR_RSSI:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_NEIGHBOR_RSSI...\r\n");
		ret.value.uivalue = neighbors_get_rssi(req->value.intval);
		ret.length = 4;
		break;

	case CONFIG_NEIGHBOR_ROUTER:
		PRINTF("[COMMAND HANDLE GET] Token is CONFIG_NEIGHBOR_ROUTER...\r\n");
		ret.value.uivalue = neighbors_get_router(req->value.intval);
		ret.length = 4;
		break;

	default:
		PRINTF("[COMMAND HANDLE GET] INVALID TOKEN %X. ABORTING RETURN MESSAGE...\r\n", req->token);
		ret.valid = 0;
		break;
	}

	commander_send(remote_addr, &ret, ret.length);
}

void command_handler(uip_ipaddr_t *remote_addr, int remote_port, char *data, int length)
{
	command_set_t *req = (command_set_t *) data;

	PRINTF("[COMMAND HANDLER] Beginning function. header = 0x%X, config_type = 0x%X, token = 0x%X, intval = 0x%X\r\n", req->header, req->config_type, req->token, req->value.intval);
	uint8_t *b = (uint8_t *) data;

	int i = 0;
	for (i = 0; i < 16; i++){
		PRINTF("[COMMAND HANDLER] Data (in uint8_t) at 0x%X: %d: 0x%X\r\n", b, i, *b);
		b++;
	}

	if (req->config_type == CMD_SET_CONFIG) {
		command_handle_set(req,remote_addr,remote_port);
	}
	else if (req->config_type == CMD_REQ_CONFIG) {
		command_handle_get(req, remote_addr, remote_port);
	}
	else {
		PRINTF("Error - not a valid command\r\n");
	}

}
