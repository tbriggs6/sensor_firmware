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

#include "../modules/sensors/sensors.h"
#include "../modules/sensors/ms5637.h"
#include "../modules/sensors/si7210.h"
#include "../modules/sensors/vaux.h"
#include "../modules/sensors/tcs3472.h"
#include "../modules/sensors/pic32drvr.h"

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
  static tcs3472_data_t cdata = { 0 };
  static conductivity_t pdata = { 0 } ;

  static clock_t start = 0;
  static clock_t now = 0;

  PROCESS_BEGIN();


  sensors_init( );

  // show a millisecond in clock ticks
  start = clock_time( );
  for (int i = 0; i < 10; i++)
  	clock_delay_usec(10000);
  now = clock_time( );




  config_init(000);

  vaux_enable();
  start = clock_time( );

  etimer_set(&et,  5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  // start the sensor readings
  process_start(&ms5637_proc, (void *)&mdata);
  completions |= (1 << 0);

  process_start(&si7210_proc, (void *)&sdata);
  completions |= (1 << 1);

  process_start(&tcs3472_proc, (void *)&cdata);
  completions |= (1 << 2);

  printf("PDATA: %p\n", &pdata);

  process_start(&pic32_proc, (void *) &pdata);
  completions |= (1 << 3);

  etimer_set(&et, CLOCK_SECOND);

  while (completions != 0) {
  	PROCESS_YIELD( );
  	if ((completions & (1 << 0)) && !process_is_running(&ms5637_proc)) {
  		completions &= ~(1 << 0);
  		now = clock_time();
  	  printf("ms5637 %lu ticks, still running: %x\n", (now-start), completions);
  	}

  	if ((completions & (1 << 1)) && !process_is_running(&si7210_proc)) {
  		completions &= ~(1 << 1);
  		now = clock_time();
  		printf("si7210 %lu ticks, still running: %x\n", (now-start), completions);
  	}
  	if ((completions & (1 << 2)) && !process_is_running(&tcs3472_proc)) {
  		completions &= ~(1 << 2);
  		now = clock_time();
  		printf("tcs3472 %lu ticks, still running: %x\n", (now-start), completions);
  	}
  	if ((completions & (1 << 3)) && !process_is_running(&pic32_proc)) {
    		completions &= ~(1 << 3);
    		now = clock_time();
    		printf("pic32 %lu ticks, still running: %x\n", (now-start), completions);
    	}

  	if (etimer_expired(&et)) {
  		etimer_set(&et, CLOCK_SECOND);
  		if (completions & (1 << 0)) printf("ms5637 still running: %d\n", process_is_running(&ms5637_proc));
  		if (completions & (1 << 1)) printf("si7210 still running: %d\n", process_is_running(&si7210_proc));
  		if (completions & (1 << 2)) printf("tcs3472 still running: %d\n", process_is_running(&tcs3472_proc));
  		if (completions & (1 << 3)) printf("pic32 still running: %d\n", process_is_running(&pic32_proc));
  	}
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
  printf("RC: %d\n", cdata.rc);
  printf("RGBC: <%d,%d,%d,%d>\n", (int)cdata.red,(int)cdata.green,(int)cdata.blue,(int)cdata.clear);
  printf("***********************\n");
  printf("RC: %d\n", pdata.rc);
  printf("PDATA: %p\n", &pdata);
  printf("RANGE: <%d,%d,%d>\n", pdata.range[0], pdata.range[1], pdata.range[2]);
  printf("***********************\n");

  vaux_disable();

  printf("*** Test Finished ***\n");
  PROCESS_END();
}

