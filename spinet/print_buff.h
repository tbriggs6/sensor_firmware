/*
 * print_buff.h
 *
 *  Created on: Jun 18, 2020
 *      Author: contiki
 */

#ifndef SPINET_PRINT_BUFF_H_
#define SPINET_PRINT_BUFF_H_

#include "contiki.h"

#define print_hex_dump(level,prefix,rowsize,buff,len) { \
	if (level >= LOG_LEVEL) \
		print_hex_dump_fn(prefix,rowsize,buff,len); \
}

void print_hex_dump_fn(const char *prefix_str,
			   int rowsize, const void *buf, size_t len);


#endif /* SPINET_PRINT_BUFF_H_ */
