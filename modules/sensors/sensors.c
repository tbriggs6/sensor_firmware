/*
 * sensors.c
 *
 *  Created on: Jan 12, 2021
 *      Author: contiki
 */


#include <contiki.h>
#include <pt-sem.h>

struct pt_sem mutexi2c;



void sensors_init( )
{
	PT_SEM_INIT(&mutexi2c, 1);
}


