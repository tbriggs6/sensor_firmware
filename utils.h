/*
 * utils.h
 *
 *  Created on: Nov 16, 2017
 *      Author: tbriggs
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>

void uip_from_string(const char *str, uip_ipaddr_t *addr);
int ishex(char ch);

#endif /* UTILS_H_ */
