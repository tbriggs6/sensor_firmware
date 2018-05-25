/*
 * sensor-cc1310.c
 *
 *  Created on: May 25, 2018
 *      Author: contiki
 */




#include "scif_framework.h"
#include "scif_osal_contiki.h"
#include "scif_scs.h"

#include <contiki.h>
#include "sensor.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/message.h"


#ifndef BV
#define BV(n)   (1 << (n))
#endif



// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_INFO

volatile static data_t data;

PROCESS(sensor_timer, "Sensor Timer");
PROCESS_THREAD(sensor_timer, ev, data)
{
	static struct etimer et;
	PROCESS_BEGIN( );


	while(1) {
		etimer_set(&et, config_get_sensor_interval() * CLOCK_SECOND);

	    PROCESS_WAIT_EVENT( );
	    // two ways out of here -- timer expired (then capture data)
	    // or sensor interval changed

	    if (ev == PROCESS_EVENT_TIMER) {
	    	LOG_INFO("Starting SCS\n");
    		scifSwTriggerExecutionCodeNbl(BV(SCIF_SCS_READ_DATA_TASK_ID));
	    }
	}

	PROCESS_END();
}



PROCESS(sensor_sender,"Sensor Sender");
PROCESS_THREAD(sensor_sender, ev, data)
{
	static uip_ip6addr_t addr;


	PROCESS_BEGIN( );
	uiplib_ip6addrconv("fd00::1", &addr);

	while(1) {
		PROCESS_WAIT_EVENT();

		if (ev == PROCESS_EVENT_POLL) {
			LOG_INFO("Sending data");
			// send the data
			messenger_send(&addr, &data, sizeof(data));
		}
	}

	PROCESS_END();
}


void sensor_postdata(void *data)
{
	LOG_INFO("SCS Data is posted\n");
	process_poll(&sensor_sender);
}


void sensor_init( )
{
	sensor_aux_init();
	process_start(&sensor_sender, NULL);
	process_start(&sensor_timer, NULL);
}
