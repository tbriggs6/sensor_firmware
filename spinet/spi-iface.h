/*
 * spi-iface.h
 *
 *  Created on: Jun 17, 2020
 *      Author: contiki
 */

#ifndef SPINET_SPI_IFACE_H_
#define SPINET_SPI_IFACE_H_

#include "contiki.h"

extern struct process spi_rdcmd;

void spi_init( );
int to_pi_pktlen( );
void copy_uip_to_spi( );

#endif /* SPINET_SPI_IFACE_H_ */
