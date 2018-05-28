/*
 * sensor-cc1310.c
 *
 *  Created on: May 25, 2018
 *      Author: contiki
 */

#include "scif_framework.h"
#include "scif_osal_none.h"
#include "scif_scs.h"

#include <contiki.h>
#include "sensor.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/message.h"

#include "arch/cpu/cc26xx-cc13xx/lpm.h"
#include "arch/cpu/cc26xx-cc13xx/dev/aux-ctrl.h"

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
      scifSwTriggerExecutionCodeNbl(1 << SCIF_SCS_READ_DATA_TASK_ID);
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
		  //LOG_INFO("Sending data");
		  // send the data
		  //messenger_send(&addr, &data, sizeof(data));
		}
	}

	PROCESS_END();
}


void sensor_postdata(void *data)
{
	LOG_INFO("SCS Data is posted\n");
	process_poll(&sensor_sender);
}


PROCESS(sensor_ready,"Sensor Ready");
PROCESS_THREAD(sensor_ready, ev, data)
{
  PROCESS_BEGIN( );
  while(1) { 
	PROCESS_WAIT_EVENT();

	if (ev == PROCESS_EVENT_POLL) {
	  LOG_INFO("Received Ready callback\n");
        }
  }
  PROCESS_END( );
}


PROCESS(sensor_alert,"Sensor Alert");
PROCESS_THREAD(sensor_alert, ev, data)
{
  PROCESS_BEGIN( );
  while(1) { 
	PROCESS_WAIT_EVENT();

	if (ev == PROCESS_EVENT_POLL) {
	  LOG_INFO("Received alert callback\n");
	  LOG_INFO("Clearing ACK events\n");
	  scifClearAlertIntSource( );
	  scifAckAlertEvents( );
        }
  }
  PROCESS_END( );
}


void sensor_ready_callback( void )
{
  process_poll(&sensor_ready);
}

void sensor_alert_callback( void )
{
   process_poll(&sensor_alert);
}

uint8_t sensor_req_max_pm(void) {
  return LPM_MODE_AWAKE;
}

void sensor_shutdown(uint8_t mode) {

}

void sensor_wakeup(void) {
  LOG_INFO("Received LPM wakeup\n");
}

LPM_MODULE(sensor_lpm, sensor_req_max_pm, sensor_shutdown, sensor_wakeup, LPM_DOMAIN_NONE);

static aux_consumer_module_t aux_consumer;

static void sensor_aux_init( )
{
  
  aux_consumer.clocks =
    AUX_WUC_ADI_CLOCK |
    AUX_WUC_ADI_CLOCK |
    AUX_WUC_OSCCTRL_CLOCK |
    AUX_WUC_TDCIF_CLOCK |
    AUX_WUC_ANAIF_CLOCK |
    AUX_WUC_TIMER_CLOCK |
    AUX_WUC_AIODIO0_CLOCK |
    AUX_WUC_AIODIO1_CLOCK |
    AUX_WUC_SMPH_CLOCK |
    AUX_WUC_TDC_CLOCK |
    AUX_WUC_ADC_CLOCK |
    AUX_WUC_REF_CLOCK;
  
  aux_ctrl_register_consumer(&aux_consumer);
}

void sensor_init( )
{
  sensor_aux_init();
  
  process_start(&sensor_sender, NULL);
  process_start(&sensor_timer, NULL);
  
  process_start(&sensor_ready,NULL);
  process_start(&sensor_alert,NULL);

  LOG_INFO("Registering callbacks\n");
  scifOsalRegisterCtrlReadyCallback(sensor_ready_callback);
  scifOsalRegisterTaskAlertCallback(sensor_alert_callback);

  LOG_INFO("Enabling AON domain\n");
  scifOsalEnableAuxDomainAccess( );
  
  LOG_INFO("Calling scifInit\n");
  scifInit(&scifScsDriverSetup);
  
  LOG_INFO("Enabling task alert\n");
  scifOsalEnableTaskAlertInt( );
  scifSetWakeOnAlertInt( true );
  scifAckAlertEvents( );

  lpm_register_module(&sensor_lpm);
}
