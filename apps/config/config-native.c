/*
 * config.c
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#include <stdio.h>
#include <contiki.h>
#include <string.h>
#include "config.h"

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "MESSAGE"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif


#define FILE_NAME "config.txt"

void config_read()
{
	FILE *fp = fopen(FILE_NAME,"r");
	if (fp == NULL) {
		LOG_ERR("Could not open file config file!");
		return;
	}

	char str[255];
	uip_ip6addr_t server;

	uint32_t value;
	while (strlen(str) > 0) {

		memset(str,0, sizeof(str));
		fgets(str,254,fp);

		if (str[0] == '#') continue;
		if (str[0] == '\n') continue;
		if (str[0] == '\r') continue;

		if (!strncmp(str,"MAGIC=",6)) {
			sscanf(str,"MAGIC=%x", &value);
			config_set_magic(value);
		}
		else if (!strncmp(str,"BCAST=",6)) {
			sscanf(str,"BCAST=%d", &value);
			config_set_bcast_interval(value);
		}
		else if (!strncmp(str,"SENSOR=",7)) {
			sscanf(str,"SENSOR=%d", &value);
			config_set_sensor_interval(value);
		}
		else if (!strncmp(str,"NEIGHBOR=",8)) {
			sscanf(str,"NEIGHBOR=%d", &value);
			config_set_neighbor_interval(value);
		}
		else if (!strncmp(str,"MAJOR=",6)) {
			sscanf(str,"MAJOR=%d", &value);
			config_set_major_version(value);
		}
		else if (!strncmp(str,"MINOR=",6)) {
			sscanf(str,"MINOR=%d", &value);
			config_set_minor_version(value);
		}
		else if (!strncmp(str,"SERVER=",7)) {
			uiplib_ip6addrconv(str+7, &server);
			config_set_receiver(&server);
		}
		else {
			LOG_ERR("Unknown config token: %s\n", str);
		}
	}

}

void config_write()
{

	FILE *fp = fopen(FILE_NAME,"w+");
	fprintf(fp,"MAGIC=%x\n", config_get_magic());
	fprintf(fp,"BCAST=%d\n", config_get_bcast_interval());
	fprintf(fp,"SENSOR=%d\n", config_get_sensor_interval());
	fprintf(fp,"NEIGHBOR=%d\n", config_get_neighbor_interval());
	fprintf(fp,"MAJOR=%d\n", config_get_major_version());
	fprintf(fp,"MINOR=%d\n", config_get_minor_version());

	uip_ip6addr_t addr;
	config_get_receiver(&addr);

	fprintf(fp,"SERVER=");
	for (int i = 0; i < 8; i++) {
		if (i > 0) fprintf(fp,":");
		fprintf(fp,"%x", addr.u16[i]);
	}
	fprintf(fp,"\n");

	fclose(fp);
}
