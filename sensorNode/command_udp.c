/*
 * sender.c
 *
 *  Created on: Jan 2, 2017
 *      Author: tbriggs
 */

#include <contiki.h>
#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include <simple-udp.h>
#include <sys/node-id.h>

#include "../config.h"
#include "../message.h"
#include "sensor.h"
#include "command.h"
#include "../neighbors.h"

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


#define UDP_PORT 5523

static struct simple_udp_connection connection;
static process_event_t command_event;

static struct cmd_req {
	command_set_t command;
	uip_ipaddr_t sender_addr;
	uint16_t sender_port;
} request;

static int command_process_req(const command_set_t *cmd, command_ret_t *ret)
{
	int retval = 1;

	ret->header = CMD_RET_HEADER;
	ret->token = cmd->token;
	ret->valid = 1;

	switch (cmd->token) {
	case CONFIG_CTIME:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = config_get_ctime_offset();
		PRINTF("CMD: CTIME=%lu\n", ret->value.ulong);
		break;

	case CONFIG_DEVTYPE:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = DEVICE_TYPE;
		PRINTF("CMD: DEVTYPE=%lu\n", ret->value.ulong);
		break;

	case CONFIG_UPTIME:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = clock_seconds();
		PRINTF("CMD: UPTIME=%lu\n", ret->value.ulong);
		break;

	case CONFIG_ENERGEST_CPU:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = energest_type_time(ENERGEST_TYPE_CPU);
		PRINTF("CMD: ENERGEST_CPU=%lu\n", ret->value.ulong);
		break;

	case CONFIG_ENERGEST_LPM:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = energest_type_time(ENERGEST_TYPE_LPM);
		PRINTF("CMD: ENERGEST_LPM=%lu\n", ret->value.ulong);
		break;

	case CONFIG_ENERGEST_TRANSMIT:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = energest_type_time(ENERGEST_TYPE_TRANSMIT);
		PRINTF("CMD: ENERGEST_TRANSMIT=%lu\n", ret->value.ulong);
		break;

	case CONFIG_ENERGEST_LISTEN:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = energest_type_time(ENERGEST_TYPE_LISTEN);
		PRINTF("CMD: ENERGEST_LISTEN=%lu\n", ret->value.ulong);
		break;

	case CONFIG_ENERGEST_MAX:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = energest_type_time(ENERGEST_TYPE_MAX);
		PRINTF("CMD: ENERGEST_MAX=%lu\n", ret->value.ulong);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = config_get_bcast_interval();
		PRINTF("CMD: BROADCAST_INTERVAL=%lu\n", ret->value.ulong);
		break;

	case CONFIG_SENSOR_INTERVAL:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = config_get_sensor_interval();
		PRINTF("CMD: SENSOR_INTERVAL=%lu\n", ret->value.ulong);
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		ret->length = sizeof(unsigned long);
		ret->value.ulong  = config_get_neighbor_interval();
		PRINTF("CMD: NEIGHBOR_INTERVAL=%lu\n", ret->value.ulong);
		break;

	case CONFIG_NEIGHBOR_NUM:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = neighbors_getnum();
		PRINTF("CMD: NEIGHBOR_NUM=%lu\n", ret->value.ulong);
		break;

	case CONFIG_NEIGHBOR_POS:
		ret->length = sizeof(uip_ipaddr_t);
		neighbors_get(cmd->value, &(ret->value.ipaddr));
		break;

	case CONFIG_NEIGHBOR_RSSI:
		ret->length = sizeof(int32_t);
		ret->value.ivalue = neighbors_get_rssi(cmd->value);
		PRINTF("CMD: RSSI=%ld\n", ret->value.ivalue);
		break;

	case CONFIG_NEIGHBOR_ROUTER:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = neighbors_get_router(cmd->value);
		PRINTF("CMD: ROUTER=%lu\n", ret->value.ulong);
		break;

	case CONFIG_RTIMER_SECOND:
		ret->length = sizeof(unsigned long);
		ret->value.ulong = RTIMER_ARCH_SECOND;
		printf("CMD: RTIMER_SECOND: %lu\n", ret->value.ulong);
		break;

	default:
		PRINTF("Unknown token: %lu/%lux\n", cmd->token, cmd->token);
		ret->valid = 0;
		retval = 0;
	}

	return retval;
}

static int command_process_set(const command_set_t *cmd, command_ret_t *ret)
{

	uint32_t value = cmd->value;
	int retval = 1;

	switch (cmd->token) {
	case CONFIG_CTIME:
		PRINTF("Setting CTIME to %lu\n", value);
		config_set_ctime_offset(value);
		command_process_req(cmd, ret);
		break;

	case CONFIG_BROADCAST_INTERVAL:
		PRINTF("Setting BCAST interval to %lu\n", value);
		config_set_bcast_interval(value);
		command_process_req(cmd, ret);
		break;

	case CONFIG_SENSOR_INTERVAL:
		PRINTF("Setting SENSOR interval to %lu\n", value);
		config_set_sensor_interval(value);
		command_process_req(cmd, ret);
		break;

	case CONFIG_NEIGHBOR_INTERVAL:
		PRINTF("Setting NEIGHBOR interval to %lu\n", value);
		config_set_neighbor_interval(value);
		command_process_req(cmd, ret);
		break;

	default:
		PRINTF("Unknown or unwritable configuration token: %lu/%lux\n", cmd->token, cmd->token);
		retval = 0;
		break;
	}

	return retval;
}




PROCESS(command_process, "Command process");
PROCESS_THREAD(command_process, ev, data)
{
	PROCESS_BEGIN( );
	static command_ret_t result;
	static int rc;

	while(1) {
		PRINTF("**** WAITING FOR COMMAND ****\n");
		PROCESS_WAIT_EVENT_UNTIL(ev == command_event);

		PRINTF("**** FOUND CMD EVENT ****\n");
		if (request.command.config_type == (configtype_t) CMD_SET_CONFIG) {
			rc = command_process_set(&(request.command), &result);
		}
		else if(request.command.config_type == (configtype_t) CMD_REQ_CONFIG) {
			rc = command_process_req(&(request.command), &result);
		}
		else {
			PRINTF("CMD_PROC: neither SET nor REQ\n");
			continue;
		}

		if (rc == 0) {
			PRINTF("CMD_PROC: rejected request\n");
			continue;
		}

		PRINTF("**** SENDING RESULT TO REQUSTOR ****\n");
		int xmit_length = (4 * 4) + result.length;

		simple_udp_sendto_port(&connection, &result, xmit_length, &request.sender_addr, request.sender_port);
	}

	PROCESS_END();
}
/*
 * sender_receiver - Respond to a remote request for a configuration request.
 */
static void command_receive(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
		const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t length)
{
	printf("***** CMD_UDP: Received packet from ");
	uip_debug_ipaddr_print(sender_addr);
	printf(" port %d\n", sender_port );

	if (length != sizeof(command_set_t)) {
		PRINTF("CMD: did not receive expected bytes %d!=%d\n", sizeof(command_set_t), length);
		return;
	}


	command_set_t *cmd = (command_set_t *) data;
	if (cmd->header != CMD_SET_HEADER) {
		PRINTF("Invalid header: %x!=%x\n", (int) cmd->header, CMD_SET_HEADER);
		return;
	}

	memcpy(&(request.command), cmd, sizeof(command_set_t));
	memcpy(&(request.sender_addr), sender_addr, sizeof(request.sender_addr));
	request.sender_port = sender_port;


	process_post(&command_process, command_event, &request);


}

void command_init()
{
	command_event = process_alloc_event( );

	PRINTF("******************************************\n");
	PRINTF("* CMD HANDLER INIT  LISTENING PORT %d *\n", UDP_PORT);
	simple_udp_register(&connection, UDP_PORT, NULL, 0, command_receive);

	process_start(&command_process, NULL);
}

