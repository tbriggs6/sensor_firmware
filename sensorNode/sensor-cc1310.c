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
#include <dev/leds.h>

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "SENSOR"
#define LOG_LEVEL LOG_LEVEL_INFO

volatile static data_t sensor_data;

PROCESS(sensor_timer, "Sensor Timer");
PROCESS_THREAD(sensor_timer, ev, data) {
	static struct etimer et;
	PROCESS_BEGIN( )
	;

	while (1) {
		etimer_set(&et, config_get_sensor_interval() * CLOCK_SECOND);

		PROCESS_WAIT_EVENT( )
		;
		// two ways out of here -- timer expired (then capture data)
		// or sensor interval changed

		if (ev == PROCESS_EVENT_TIMER) {
			LOG_DBG("Starting SCS\n");
			scifSwTriggerExecutionCodeNbl(1 << SCIF_SCS_READ_DATA_TASK_ID);
		}
	}

PROCESS_END();
}

PROCESS(sensor_sender, "Sensor Sender");
PROCESS_THREAD(sensor_sender, ev, data) {
static uip_ip6addr_t addr;

PROCESS_BEGIN( )
;

config_get_receiver(&addr);

while (1) {
	PROCESS_WAIT_EVENT()
	;

	if (ev == PROCESS_EVENT_POLL) {
		LOG_INFO("Sending data: seq=%d, temp=%d, cond=%d, amb=%d, hall=%d, rgb(%d,%d,%d,%d)\n",
				(int) sensor_data.sequence, sensor_data.temperature, sensor_data.adc[1],
				sensor_data.adc[0], sensor_data.adc[2], sensor_data.colors[0], sensor_data.colors[1],
				sensor_data.colors[2], sensor_data.colors[3] );

		// send the data
		messenger_send(&addr, (void *) &sensor_data, sizeof(sensor_data));

	}

	else if (sender_fin_event) {
		int sent_len, recv_len;
		uint8_t result[8];

		messenger_get_last_result(&sent_len, &recv_len, sizeof(result),	&result);
		if (sent_len < 0) {
			// there was an error sending data
			leds_on(LEDS_CONF_RED);
		} else {
			leds_off(LEDS_CONF_RED);
		}
	}
}

PROCESS_END();
}

void sensor_postdata(void *data) {
LOG_INFO("SCS Data is posted\n");
process_poll(&sensor_sender);
}

PROCESS(sensor_ready, "Sensor Ready");
PROCESS_THREAD(sensor_ready, ev, data) {
PROCESS_BEGIN( )
;
while (1) {
PROCESS_WAIT_EVENT()
;

if (ev == PROCESS_EVENT_POLL) {
	LOG_INFO("Received Ready callback\n");
}
}
PROCESS_END( );
}

PROCESS(sensor_alert, "Sensor Alert");
PROCESS_THREAD(sensor_alert, ev, data) {
PROCESS_BEGIN( )
;
while (1) {
PROCESS_WAIT_EVENT()
;

if (ev == PROCESS_EVENT_POLL) {
LOG_DBG("Received alert callback\n");

sensor_data.sequence++;
// copy the data from the sensor space
sensor_data.adc[0] = scifScsTaskData.readData.output.AmbLight;
sensor_data.adc[1] = scifScsTaskData.readData.output.Conductivity;
sensor_data.adc[2] = scifScsTaskData.readData.output.HallSensor;
sensor_data.adc[3] = 0;

sensor_data.temperature = scifScsTaskData.readData.output.TemperatureSensor;
sensor_data.battery = scifScsTaskData.readData.output.BatterySensor;

sensor_data.colors[0] = scifScsTaskData.readData.output.colorRed;
sensor_data.colors[1] = scifScsTaskData.readData.output.colorGreen;
sensor_data.colors[2] = scifScsTaskData.readData.output.colorBlue;
sensor_data.colors[3] = scifScsTaskData.readData.output.colorClear;

sensor_data.I2CError = scifScsTaskData.readData.output.I2CError;

scifClearAlertIntSource();
scifAckAlertEvents();
process_poll(&sensor_sender);
}
}
PROCESS_END( );
}

void sensor_ready_callback(void) {
process_poll(&sensor_ready);
}

void sensor_alert_callback(void) {
process_poll(&sensor_alert);
}

static aux_consumer_module_t aux_consumer;

static void sensor_aux_init() {

aux_consumer.clocks = AUX_WUC_ADI_CLOCK | AUX_WUC_ADI_CLOCK
| AUX_WUC_OSCCTRL_CLOCK | AUX_WUC_TDCIF_CLOCK | AUX_WUC_ANAIF_CLOCK
| AUX_WUC_TIMER_CLOCK | AUX_WUC_AIODIO0_CLOCK | AUX_WUC_AIODIO1_CLOCK
| AUX_WUC_SMPH_CLOCK | AUX_WUC_TDC_CLOCK | AUX_WUC_ADC_CLOCK
| AUX_WUC_REF_CLOCK;

aux_ctrl_register_consumer(&aux_consumer);
}

void sensor_init() {
sensor_data.sequence = 0;

sensor_aux_init();

process_start(&sensor_sender, NULL);
process_start(&sensor_timer, NULL);

process_start(&sensor_ready, NULL);
process_start(&sensor_alert, NULL);

LOG_DBG("Registering callbacks\n");
scifOsalRegisterCtrlReadyCallback(sensor_ready_callback);
scifOsalRegisterTaskAlertCallback(sensor_alert_callback);

LOG_DBG("Calling scifInit\n");
scifInit(&scifScsDriverSetup);

}
