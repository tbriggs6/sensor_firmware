/*
 * rpl-status.h
 *
 *  Created on: Nov 15, 2018
 *      Author: contiki
 */

#ifndef SPINET_RPL_IF_H_
#define SPINET_RPL_IF_H_

#include <contiki.h>

#define RPLSTAT_AUTOREPAIR (20 * CLOCK_SECOND)

void rplstat_set_prefix( void  );
void rplstat_init( void );
void rplstat_reset_timer( void );
unsigned int rplif_copy_buffer(void *dest, int max_len);


#endif /* SPINET_RPL_IF_H_ */
