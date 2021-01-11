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
#include "../modules/sensors/si7210.h"
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
  static si7210_data_t sdata = { 0 };
  static clock_t start = 0;
  static clock_t now = 0;

  PROCESS_BEGIN();

  // show a millisecond in clock ticks
  start = clock_time( );
  clock_delay_usec(1000);
  now = clock_time( );


  printf("1ms = %u\n", (unsigned)(now - start));

  config_init(000);

  vaux_enable();
  start = clock_time( );



  printf("1ms = %u\n", (unsigned)(now - start));

  etimer_set(&et,  5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  // start the sensor readings
  process_start(&ms5637_proc, (void *)&mdata);
  completions |= (1 << 0);

  process_start(&si7210_proc, (void *)&sdata);
  completions |= (1 << 1);

  while (completions != 0) {
  	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_EXITED);
  	if (data == &ms5637_proc) completions &= ~(1 << 0);
  	if (data == &si7210_proc) completions &= ~(1 << 1);
  }

  now = clock_time( );

  printf("***********************\n");
  printf("Time: %u\n", (unsigned)(now-start));

  printf("***********************\n");
  printf("RC: %d\n", mdata.status);
  printf("PRESS: %u\n", (unsigned) mdata.pressure);
  printf("TEMP: %u\n", (unsigned) mdata.temperature);
  printf("***********************\n");
  printf("RC: %d\n", sdata.rc);
  printf("Field: %d\n", sdata.magfield);
  printf("***********************\n");

  vaux_disable();

  printf("*** Test Finished ***\n");
  PROCESS_END();
}

