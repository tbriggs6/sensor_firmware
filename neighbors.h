/*
 * neighbors.h
 *
 *  Created on: Jan 11, 2017
 *      Author: tbriggs
 */

#ifndef NEIGHBORS_H_
#define NEIGHBORS_H_


void neighbors_init( );


int neighbors_getnum( );
void neighbors_get(int pos, uip_ipaddr_t *address);
int neighbors_get_rssi(int pos);
int neighbors_get_router(int pos);

#endif /* NEIGHBORS_H_ */
