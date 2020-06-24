/*
 * spi-regs.h
 *
 *  Created on: Jun 17, 2020
 *      Author: contiki
 */

#ifndef SPINET_SPI_REGS_H_
#define SPINET_SPI_REGS_H_

#define REG_TXBUSY 0
#define REG_TXOK 1
#define REG_TXCOLL 2

#define REG_SELFID 16
#define REG_UIPLEN 17

#define REG_NUM_RW 16

int register_read(unsigned regnum);
void register_write(unsigned regnum, int value);
void register_increment(unsigned regnum);
void register_decrement(unsigned regnum);
void register_clear(unsigned regnum);


#endif /* SPINET_SPI_REGS_H_ */
