/*
 * config_message.h
 *
 *  Created on: Dec 3, 2017
 *      Author: contiki
 */

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "config.h"

#define CMD_SET_HEADER (0x0beed1eU)

typedef enum cmd_type {
        CMD_SET_CONFIG, CMD_REQ_CONFIG
} command_type_t;


typedef struct __attribute__((__packed__)) {
        uint32_t header;
        command_type_t config_type;
        configtype_t token;
        union {
        	uint32_t intval;
        	uip_ipaddr_t address;
        } value;
} command_set_t;


#define CMD_RET_HEADER (0xdde323f3U)
typedef struct __attribute__((__packed__)) {
        uint32_t header;
        configtype_t token;
        uint8_t valid;
        uint32_t length;
        union {
                uint32_t uivalue;
                unsigned long ulong;
                uip_ipaddr_t ipaddr;
                char buff[32];
                int32_t ivalue;
        } value;
} command_ret_t;


#define DATA_NUM_ADC (4)
#define DATA_HEADER (0x34323233U)

typedef struct {
        uint32_t header;
        uint32_t sequence;
        uint16_t battery;
        uint16_t temperature;
        uint16_t adc[4];
        uint16_t colors[4];
        uint16_t I2CError;
} data_t;

#define DATA_ACK_HEADER (0x90983323U)
typedef struct {
        uint32_t header;
        uint32_t sequence;
        uint32_t ack_seq;
        uint32_t ack_value;
} data_ack_t;

void message_print_data(const data_t *data);



#endif /* _MESSAGE_H_ */
