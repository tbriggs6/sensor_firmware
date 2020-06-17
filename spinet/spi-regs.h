/*
 * spi-regs.h
 *
 *  Created on: Jun 17, 2020
 *      Author: contiki
 */

#ifndef SPINET_SPI_REGS_H_
#define SPINET_SPI_REGS_H_


int register_read(int regnum);
void register_write(int regnum, int value);


#endif /* SPINET_SPI_REGS_H_ */
