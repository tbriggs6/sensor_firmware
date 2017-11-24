/*
 * message.c
 *
 *  Created on: Jan 3, 2017
 *      Author: tbriggs
 */
#include "message.h"
#include <stdio.h>



void message_print_data(const data_t *data)
{
	if (data->header != DATA_HEADER) {
		printf("WARN: data header is no match\n");
	}
	else {
		printf("data header: (%-8.8X) seq: %u", (int) data->header, (unsigned int) data->sequence);
		printf("battery: %u temperature: %u\n", data->battery, data->temperature);
	}
}
