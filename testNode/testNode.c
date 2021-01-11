/*
 * sensorNode.c
 *
 *  Created on: May 18, 2018
 *      Author: contiki
 */
#include <contiki.h>
#include <stdio.h> /* For printf() */


#include "config_nvs.h"
#include "config.h"


// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "TEST"
#define LOG_LEVEL LOG_LEVEL_DBG

#include "../modules/sensors/ms5637.h"
#include "../modules/sensors/vaux.h"
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{

  static struct etimer et = { 0 };
  static unsigned int completions = 0;
  static ms5637_data_t mdata = { 0 };

  PROCESS_BEGIN();

  vaux_enable();

  etimer_set(&et,  5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  // start the sensor readings
  process_start(&ms5637_proc, (void *)&mdata);
  completions |= (1 << 0);


  while (completions != 0) {
  	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_EXITED);
  	if (data == &ms5637_proc) completions &= ~(1 << 0);
  }

  printf("RC: %d\n", mdata.status);
  printf("PRESS: %u\n", (unsigned) mdata.pressure);
  printf("TEMP: %u\n", (unsigned) mdata.temperature);

  vaux_disable();

  printf("*** Test Finished ***\n");
  PROCESS_END();
}

