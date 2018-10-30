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
#include "airborne.h"
#include "../modules/messenger/message-service.h"
#include "../modules/command/message.h"

#include "arch/cpu/cc26xx-cc13xx/lpm.h"
#include "arch/cpu/cc26xx-cc13xx/dev/aux-ctrl.h"
#include <dev/leds.h>

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_INFO

volatile static airborne_t sensor_data;
volatile static airborne_cal_t cal_data;
volatile static int first_callback = 0;

PROCESS(sensor_timer, "Sensor Timer");
PROCESS_THREAD(sensor_timer, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN( )
  ;

  while (1)
    {
      etimer_set (&et, config_get_sensor_interval () * CLOCK_SECOND);

      PROCESS_WAIT_EVENT( );

      // two ways out of here -- timer expired (then capture data)
      // or sensor interval changed

      if (ev == PROCESS_EVENT_TIMER)
	{
	  LOG_DBG("Starting SCS\n");
	  scifSwTriggerExecutionCodeNbl (1 << SCIF_SCS_READ_SENSORS_TASK_ID);
	}
    }

  PROCESS_END();
}

PROCESS(sensor_sender, "Sensor Sender");
PROCESS_THREAD(sensor_sender, ev, data)
{
static uip_ip6addr_t addr;

  PROCESS_BEGIN( ) ;

  LOG_INFO("Stored destination address: ");
  config_get_receiver (&addr);
  LOG_6ADDR(LOG_LEVEL_INFO, &addr);
  LOG_INFO_("\n");

  uiplib_ip6addrconv ("fd00::1", &addr);

  while (1)
    {
      PROCESS_WAIT_EVENT()
      ;

      if (ev == PROCESS_EVENT_POLL)
	{


	  leds_off (LEDS_CONF_RED);
	  leds_off (LEDS_CONF_GREEN);

	  if (first_callback == 0) {
	      LOG_INFO("Sending calibration data to server\n");
	      messenger_send (&addr, (void *) &cal_data, sizeof(cal_data));
	  }
	  else {
	    LOG_INFO(
		"Sending data: seq=%u, ms5637(press=%u,temp=%u), si7020(humid=%u,temp=%u)\n",
		(unsigned int) sensor_data.sequence,
		(unsigned int) sensor_data.ms5637_pressure, (unsigned int) sensor_data.ms5637_temp,
		(unsigned int) sensor_data.si7020_humid, (unsigned int) sensor_data.si7020_temp);

	    LOG_INFO("to: ");
	    LOG_6ADDR(LOG_LEVEL_INFO, &addr);
	    LOG_INFO_("\n");

	    // send the data
	    messenger_send (&addr, (void *) &sensor_data, sizeof(sensor_data));
	  }
	}

      else if (sender_fin_event)
	{
	  int sent_len, recv_len;
	  uint8_t result[8];

	  messenger_get_last_result (&sent_len, &recv_len, sizeof(result),
				     &result);

	  if (first_callback == 0)
	 	first_callback = 1;

	  if (sent_len < 0)
	    {
	      // there was an error sending data
	      leds_on (LEDS_CONF_RED);
	    }
	  else
	    {

	      leds_on (LEDS_CONF_GREEN);
	      leds_off (LEDS_CONF_RED);
	    }
	}
    }

PROCESS_END();
}

void
sensor_postdata (void *data)
{
LOG_INFO("SCS Data is posted\n");
process_poll (&sensor_sender);
}

PROCESS(sensor_ready, "Sensor Ready");
PROCESS_THREAD(sensor_ready, ev, data)
{
PROCESS_BEGIN( )
;

while (1)
  {
    PROCESS_WAIT_EVENT()
    ;

    if (ev == PROCESS_EVENT_POLL)
      {
	LOG_INFO("Received Ready callback\n");
      }
  }
PROCESS_END( );
}



PROCESS(sensor_alert, "Sensor Alert");
PROCESS_THREAD(sensor_alert, ev, data)
{
PROCESS_BEGIN( )
;
while (1)
{
  PROCESS_WAIT_EVENT()
  ;

  if (ev == PROCESS_EVENT_POLL)
    {
      LOG_DBG("Received alert callback\n");

      cal_data.header = AIRBORNE_CAL_HEADER;
      cal_data.sequence++;
      for (int i = 0; i < 7; i++) {
	  cal_data.caldata[i] = scifScsTaskData.readSensors.output.ms5637cal[i];
      }


      sensor_data.header = AIRBORNE_HEADER;
      sensor_data.sequence++;
// copy the data from the sensor space
      sensor_data.ms5637_pressure = scifScsTaskData.readSensors.output.ms5637press[2] << 16;
      sensor_data.ms5637_pressure |= scifScsTaskData.readSensors.output.ms5637press[1] << 8;
      sensor_data.ms5637_pressure |= scifScsTaskData.readSensors.output.ms5637press[0];

      sensor_data.ms5637_temp = scifScsTaskData.readSensors.output.ms5637temp[2] << 16;
      sensor_data.ms5637_temp |= scifScsTaskData.readSensors.output.ms5637temp[1] << 8;
      sensor_data.ms5637_temp |= scifScsTaskData.readSensors.output.ms5637temp[0];

      sensor_data.si7020_humid = scifScsTaskData.readSensors.output.si7020humid[1] << 8;
      sensor_data.si7020_humid |= scifScsTaskData.readSensors.output.si7020humid[0] << 0;

      sensor_data.si7020_temp = scifScsTaskData.readSensors.output.si7020temp[1] << 8;
      sensor_data.si7020_temp |= scifScsTaskData.readSensors.output.si7020temp[0] << 0;

      sensor_data.battery = scifScsTaskData.readSensors.output.vbat << 0;
      sensor_data.i2cerror = scifScsTaskData.readSensors.output.i2cerror << 0;


      scifClearAlertIntSource ();
      scifAckAlertEvents ();
      process_poll (&sensor_sender);
    }
}
PROCESS_END( );
}

void
sensor_ready_callback (void)
{
process_poll (&sensor_ready);
}

void
sensor_alert_callback (void)
{
process_poll (&sensor_alert);
}

static aux_consumer_module_t aux_consumer;

static void
sensor_aux_init ()
{

aux_consumer.clocks = AUX_WUC_ADI_CLOCK | AUX_WUC_ADI_CLOCK

| AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_TDCIF_CLOCK | AUX_WUC_ANAIF_CLOCK

| AUX_WUC_TIMER_CLOCK | AUX_WUC_AIODIO0_CLOCK | AUX_WUC_AIODIO1_CLOCK
| AUX_WUC_SMPH_CLOCK | AUX_WUC_TDC_CLOCK | AUX_WUC_ADC_CLOCK
| AUX_WUC_REF_CLOCK;

aux_ctrl_register_consumer (&aux_consumer);
}

void
sensor_init ()
{
sensor_data.sequence = 0;

sensor_aux_init ();

process_start (&sensor_sender, NULL);
process_start (&sensor_timer, NULL);

process_start (&sensor_ready, NULL);
process_start (&sensor_alert, NULL);

LOG_DBG("Registering callbacks\n");
scifOsalRegisterCtrlReadyCallback (sensor_ready_callback);
scifOsalRegisterTaskAlertCallback (sensor_alert_callback);

LOG_DBG("Calling scifInit\n");
scifInit (&scifScsDriverSetup);

}
