/*
 * bcast-sink.h
 *
 *  Created on: Nov 25, 2017
 *      Author: contiki
 */


#ifndef BCAST_SERVICE_SINK_H
#define BCAST_SERVICE_SINK_H

#include <contiki.h>

#include "bcast-service.h"

extern struct process broadcast_sink;
void bcast_sink_init( void );

#endif

