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

static int regs[16];

int register_read(int regnum)
{
	if (regnum < 0) return -1;

	else if (regnum < 16) return regs[regnum];
	else if (regnum == 16) return 0x12345789;
	else if (regnum == 17) return to_pi_pktlen();
	else return -1;
}

void register_write(int regnum, int value)
{
	if (regnum < 0) return;
	else if (regnum < 16) regs[regnum] = value;
	else if (regnum == 17) uip_len = value;
	else return;
}
