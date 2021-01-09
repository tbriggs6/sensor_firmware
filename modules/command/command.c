#include <stdint.h>
#include <contiki.h>
#include "../../modules/messenger/message-service.h"
#include "command.h"
#include "message.h"
#include <sys/energest.h>

//#define DEBUG

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "CMD"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif



static void command_handle_set(const command_set_t *const req, command_ret_t *ret, int *num_bytes)
{
	int id;

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

	case CONFIG_SENSOR_INTERVAL:
		LOG_INFO("Set CONFIG_SENSOR_INTERVAL...%d\n", (int) req->value.intval);
		config_set_sensor_interval(req->value.intval);
		ret->value.uivalue = config_get_sensor_interval( );
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;

		break;

	case CONFIG_MAX_FAILURES:
		LOG_INFO("Set CONFIG_MAX_FAILURES...%d\n", (int) req->value.intval);
		config_set_maxfailures(req->value.intval);
		ret->value.uivalue = config_get_maxfailures();
		ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
		ret->length = 4;

		break;

	case CONFIG_RETRY_INTERVAL:
			LOG_INFO("Set CONFIG_RETRY_INTERVAL...%d\n", (int) req->value.intval);
			config_set_retry_interval(req->value.intval);
			ret->value.uivalue = config_get_retry_interval();
			ret->valid = (ret->value.uivalue == req->value.intval) ? 1 : 0;
			ret->length = 4;

			break;

	case CONFIG_CAL1:
	case CONFIG_CAL2:
	case CONFIG_CAL3:
	case CONFIG_CAL4:
	case CONFIG_CAL5:
	case CONFIG_CAL6:
	case CONFIG_CAL7:
	case CONFIG_CAL8:
			id = (req->token - CONFIG_CAL1);
			LOG_INFO("Set CONFIG_CAL%d...%d\n", id + 1, (int) req->value.intval);

			config_set_calibration(id, (uint16_t) req->value.intval);
			ret->value.uivalue = config_get_retry_interval();
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
	int idx;
	float value;

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

	case CONFIG_SENSOR_INTERVAL:
		LOG_INFO("Get CONFIG_SENSOR_INTERVAL...\n");
		ret->value.uivalue = config_get_sensor_interval( );
		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_MAX_FAILURES:
			LOG_INFO("Get CONFIG_MAX_FAILURES...\n");
			ret->value.uivalue = config_get_maxfailures();
			ret->length += sizeof(ret->value.uivalue);
			break;

	case CONFIG_RETRY_INTERVAL:
				LOG_INFO("Get CONFIG_RETRY_INTERVAL...\n");
				ret->value.uivalue = config_get_retry_interval();
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
			idx = req->token - CONFIG_CAL1;
			LOG_INFO("Get CONFIG_CAL%d...\n", idx);
					ret->value.uivalue = config_get_calibration(idx);
					ret->length += sizeof(ret->value.uivalue);
					break;

	case CONFIG_ENERGEST_CPU:
		LOG_INFO("Get CONFIG_ENERGEST_CPU...\n");

		value = energest_type_time(ENERGEST_TYPE_CPU);
		value = value / (float) ENERGEST_GET_TOTAL_TIME();
		ret->value.uivalue = (int)(value * 100.0);

		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_LPM:
		LOG_INFO("Get CONFIG_ENERGEST_LPM...\n");

		value = energest_type_time(ENERGEST_TYPE_LPM);
		value = value / (float) ENERGEST_GET_TOTAL_TIME();
		ret->value.uivalue = (int)(value * 100.0);

		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_TRANSMIT:
		LOG_INFO("Get CONFIG_ENERGEST_TRANSMIT...\n");

		value = energest_type_time(ENERGEST_TYPE_TRANSMIT);
		value = value / (float) ENERGEST_GET_TOTAL_TIME();
		ret->value.uivalue = (int)(value * 100.0);

		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_LISTEN:
		LOG_INFO("Get CONFIG_ENERGEST_LISTEN...\n");

		value = energest_type_time(ENERGEST_TYPE_LISTEN);
		value = value / (float) ENERGEST_GET_TOTAL_TIME();
		ret->value.uivalue = (int)(value * 100.0);

		ret->length += sizeof(ret->value.uivalue);
		break;

	case CONFIG_ENERGEST_DEEP_LPM:
		LOG_INFO("Get CONFIG_ENERGEST_DEEP_LPM...\n");

		value = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
		value = value / (float) ENERGEST_GET_TOTAL_TIME();
		ret->value.uivalue = (int)(value * 100.0);
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
	int i;
	LOG_DBG("HANDLER INPUT: (%d) ", inputlength);
	for (i = 0; i < inputlength; i++)
		LOG_DBG_("%-2.2x ", inputdata[i]);
	LOG_DBG_("\n");

	const command_set_t *req = (const command_set_t *) inputdata;
	command_ret_t *response = (command_ret_t *) outputdata;

	LOG_DBG("COMMAND REQ: \n");
	LOG_DBG("  HEADER: 0x%x\n", (unsigned int) req->header);
	LOG_DBG("  TYPE: %d\n", (unsigned int) req->config_type);
	LOG_DBG("  TOKEN: 0x%x\n", (unsigned int) req->token);
	LOG_DBG("  VALUE: 0x%x\n", (unsigned int) req->value.intval);

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


#define ADDR_DIFF(x,y) ((int) &x.y - (int) &x)
void command_init( )
{

	command_set_t test;
	LOG_DBG("Starts / sizes: total (%u)\n", sizeof(test));
	LOG_DBG("   header %d - %d\n", ADDR_DIFF(test,header), sizeof(test.header));
	LOG_DBG("   type   %d - %d\n", ADDR_DIFF(test,config_type), sizeof(test.config_type));
	LOG_DBG("   token  %d - %d\n", ADDR_DIFF(test,token), sizeof(test.token));
	LOG_DBG("   intval %d - %d\n", ADDR_DIFF(test,value.intval), sizeof(test.value.intval));

    messenger_add_handler(0x0beed1eU,   4,  sizeof(command_set_t), command_handler);
}
