/*
 * spi-regs.c
 *
 *  Created on: Jun 17, 2020
 *      Author: contiki
 */
#include "contiki.h"
#include "net/ipv6/uiplib.h"
#include "spi-regs.h"
#include "spi-iface.h"

static int regs[REG_NUM_RW];

int register_read(unsigned regnum)
{
	if (regnum < REG_NUM_RW) return regs[regnum];
	else if (regnum == REG_SELFID) return 0x12345789;
	else if (regnum == REG_UIPLEN) return to_pi_pktlen();
	else return -1;
}

void register_write(unsigned regnum, int value)
{
	if (regnum < REG_NUM_RW) regs[regnum] = value;
	else if (regnum == REG_UIPLEN) uip_len = value;
	else return;
}

void register_increment(unsigned regnum)
{
	if (regnum < REG_NUM_RW)
		regs[regnum]++;
}

void register_decrement(unsigned regnum)
{
	if (regnum < REG_NUM_RW)
		regs[regnum]--;
}

void register_clear(unsigned regnum)
{
	if (regnum < REG_NUM_RW)
		regs[regnum]--;
}
