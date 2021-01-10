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

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();


  printf("NVS / Coffee FS test\n");
  int rc = nvs_init( );
  if (rc < 0)
  		goto fail;

  config_init(0);

  printf("list config files\n");
  config_list();

  printf("\n Done reading\n");




fail:

  printf("*** Test Finished ***\n");
  PROCESS_END();
}

