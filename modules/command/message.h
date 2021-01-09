/*
 * message.h
 *
 *  Created on: May 22, 2018
 *      Author: contiki
 */

#ifndef MODULES_COMMAND_MESSAGE_H_
#define MODULES_COMMAND_MESSAGE_H_

#include <stdint.h>
#include <contiki.h>
#include <contiki-net.h>
#include <../config/config.h>

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
        	uip_ip6addr_t address;
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


#define WATER_CAL_HEADER (0x3536370U)
typedef struct __attribute__((packed)) {
    uint32_t header;
    uint32_t sequence;
    uint16_t caldata[6];
    uint16_t resistorVals[8];
    uint16_t si7210_offset;
    uint16_t si7210_gain;
} water_cal_t;

#define WATER_DATA_HEADER (0x34323233U)

typedef struct __attribute__((packed)) {
        uint32_t header;
        uint32_t sequence;

        // put 32-bits first, then 16... then 8...
        // temperature and pressure
        uint32_t pressure;
        uint32_t temppressure;

        // battery level
        uint16_t battery;

        // light on
        uint16_t color_blue;
        uint16_t color_clear;
        uint16_t color_green;
        uint16_t color_red;

        // light off
        uint16_t ambient;

        // conductivity
        uint16_t range1;
        uint16_t range2;
        uint16_t range3;
        uint16_t range4;
        uint16_t range5;

        // temperature
        uint16_t temperature;

        // depth / hall sensor
        int16_t hall;
} water_data_t;



#define AIRBORNE_CAL_HEADER (0x65bce4f0U)
typedef struct {
    uint32_t header;
    uint32_t sequence;
    uint16_t caldata[6];
} airborne_cal_t;

#define AIRBORNE_HEADER (0x54ab23efU)
typedef struct __attribute__((packed)) {
    uint32_t header;
    uint32_t sequence;
    uint32_t ms5637_pressure;
    uint32_t ms5637_temp;
    uint32_t si7020_humid;
    uint32_t si7020_temp;
    uint16_t battery;
    uint16_t i2cerror;
} airborne_t;

#define DATA_ACK_HEADER (0x45ba32feU)
typedef struct __attribute__((packed)) {
        uint32_t header;
        uint32_t sequence;
        uint32_t ack_seq;
        uint32_t ack_value;
} airborne_ack_t;



#endif /* MODULES_COMMAND_MESSAGE_H_ */
