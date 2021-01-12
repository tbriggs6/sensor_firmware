/*
 * sensors.h
 *
 *  Created on: Jan 12, 2021
 *      Author: contiki
 */

#ifndef MODULES_SENSORS_SENSORS_H_
#define MODULES_SENSORS_SENSORS_H_

#include <contiki.h>
#include <pt-sem.h>
#include <sys/log.h>

extern struct pt_sem mutexi2c;

void sensors_init( );


#define SENSOR_AQ(pt, handle, rc) \
	 PT_SEM_WAIT(pt, &mutexi2c); \
	  handle = i2c_arch_acquire (I2CBUS); \
	  if (handle == NULL) { \
	    LOG_ERR("%s:%d could not acquire i2c handle...\n", __FILE__, __LINE__); \
		  rc = false; \
	  }


#define SENSOR_REL(pt, handle) \
   PT_SEM_SIGNAL(pt, &mutexi2c); \
   i2c_arch_release(handle);



#endif /* MODULES_SENSORS_SENSORS_H_ */
