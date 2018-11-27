/*
 * spinet.h
 *
 *  Created on: Nov 15, 2018
 *      Author: contiki
 */

#ifndef SPINET_SPINET_H_
#define SPINET_SPINET_H_

#include "contiki.h"

extern struct process spislv_ctrl;
extern struct pt_sem spi_mutex;

void spinet_slv_reset( );
void toggle_rx_interrupt( );
void write_register(uint32_t reg, uint32_t val);

void raise_spi_intr( );
void clear_spi_intr( );

int register_get(int regnum);

#endif /* SPINET_SPINET_H_ */
