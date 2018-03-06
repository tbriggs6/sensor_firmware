/// \addtogroup module_scif_osal
//@{
#ifdef SCIF_INCLUDE_OSAL_C_FILE


#include <inc/hw_nvic.h>
#include <inc/hw_aux_evctl.h>
#include <inc/hw_aux_timer.h>

#include <driverlib/cpu.h>
#include <driverlib/interrupt.h>
#include "scif_osal_contiki.h"
#include "scif_scs.h"
#include "config.h"

//#include "sensor.h"
//#include "sender.h"

#include <contiki.h>
#include <dev/aux-ctrl.h>
#include <sys/pt.h>
#include <sys/pt-sem.h>
#include <lpm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ti-lib.h>

#ifndef BV
#define BV(n)           (1 << (n))
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif



/// MCU wakeup source to be used with the Sensor Controller task ALERT event, must not conflict with OS
#define OSAL_MCUWUSEL_WU_EV_S   AON_EVENT_MCUWUSEL_WU3_EV_S


/// The READY interrupt is implemented using INT_AON_AUX_SWEV0
#define INT_SCIF_CTRL_READY     INT_AON_AUX_SWEV0
/// The ALERT interrupt is implemented using INT_AON_AUX_SWEV1
#define INT_SCIF_TASK_ALERT     INT_AON_AUX_SWEV1


/// Calculates the NVIC register offset for the specified interrupt
#define NVIC_OFFSET(i)          (((i) - 16) / 32)
/// Calculates the bit-vector to be written or compared against for the specified interrupt
#define NVIC_BV(i)              (1 << ((i - 16) % 32))



// Function prototypes
static void osalCtrlReadyIsr(void);
static void osalTaskAlertIsr(void);

static bool osalPostedReady = false;

/** \brief Registers the control READY interrupt
  *
  * This registers the \ref osalCtrlReadyIsr() function with the \ref INT_SCIF_CTRL_READY interrupt
  * vector.
  *
  * The interrupt occurs at initial startup, and then after each control request has been serviced by the
  * Sensor Controller. The interrupt is pending (including source event flag) and disabled while the task
  * control interface is idle.
  */
static void osalRegisterCtrlReadyInt(void) {
    IntRegister(INT_SCIF_CTRL_READY, osalCtrlReadyIsr);
} // osalRegisterCtrlReadyInt




/** \brief Enables the control READY interrupt
  *
  * This function is called when sending a control REQ event to the Sensor Controller to enable the READY
  * interrupt. This is done after clearing the event source and then the READY interrupt, using
  * \ref osalClearCtrlReadyInt().
  */
static void osalEnableCtrlReadyInt(void) {
  //  HWREG(NVIC_EN0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
	IntEnable(INT_SCIF_CTRL_READY);
} // osalEnableCtrlReadyInt




/** \brief Disables the control READY interrupt
  *
  * This function is called when by the control READY ISR, \ref osalCtrlReadyIsr(), so that the interrupt
  * is disabled but still pending (including source event flag) while the task control interface is
  * idle/ready.
  */
static void osalDisableCtrlReadyInt(void) {
  //  HWREG(NVIC_DIS0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
	IntDisable(INT_SCIF_CTRL_READY);
} // osalDisableCtrlReadyInt




/** \brief Clears the task control READY interrupt
  *
  * This is done when sending a control request, after clearing the READY source event.
  */
static void osalClearCtrlReadyInt(void) {
  HWREG(NVIC_UNPEND0 + NVIC_OFFSET(INT_SCIF_CTRL_READY)) = NVIC_BV(INT_SCIF_CTRL_READY);
  HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGSCLR ) = AUX_EVCTL_EVTOAONFLAGSCLR_SWEV0;
  //IntPendClear(INT_SCIF_CTRL_READY);
} // osalClearCtrlReadyInt



static void osalIndicateCtrlReady(void);


PROCESS(ready_interrupt, "Ready Interrupt");
PROCESS_THREAD(ready_interrupt, ev, data)
{
	PROCESS_BEGIN( );

	while(1) {
		// poll_process(&ready_interrupt); allows this
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

		scifOsalEnableAuxDomainAccess();
		PRINTF("Received ready interrupt\n");
		osalIndicateCtrlReady( );
		osalPostedReady = true;
	}

	PROCESS_END();
}


bool osalIsReady( ) {
	return osalPostedReady;
}

/** \brief Interrupt service route for ready interrupt
 */
static void osalCtrlReadyIsr(void) {

	process_poll(&ready_interrupt);
	scifOsalEnableAuxDomainAccess();
	osalClearCtrlReadyInt( );

}



/** \brief Registers the task ALERT interrupt
  *
  * This registers the \ref osalTaskAlertIsr() function with the \ref INT_SCIF_TASK_ALERT interrupt
  * vector.
  *
  * The interrupt occurs whenever a sensor controller task alerts the driver, to request data exchange,
  * indicate an error condition or that the task has stopped spontaneously.
  */
static void osalRegisterTaskAlertInt(void) {
    IntRegister(INT_SCIF_TASK_ALERT, osalTaskAlertIsr);
} // osalRegisterTaskAlertInt




/** \brief Enables the task ALERT interrupt
  *
  * The interrupt is enabled at startup. It is disabled upon reception of a task ALERT interrupt and re-
  * enabled when the task ALERT is acknowledged.
  */
static void osalEnableTaskAlertInt(void) {
  HWREG(NVIC_EN0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
	IntEnable(INT_SCIF_TASK_ALERT);
} // osalEnableTaskAlertInt




/** \brief Disables the task ALERT interrupt
  *
  * The interrupt is enabled at startup. It is disabled upon reception of a task ALERT interrupt and re-
  * enabled when the task ALERT is acknowledged.
  */
static void osalDisableTaskAlertInt(void) {
  //  HWREG(NVIC_DIS0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
	IntDisable(INT_SCIF_TASK_ALERT);
} // osalDisableTaskAlertInt




static void osalIndicateTaskAlert(void);

/** \brief Clears the task ALERT interrupt
  *
  * This is done when acknowledging the alert, after clearing the ALERT source event.
  */
static void osalClearTaskAlertInt(void) {
  HWREG(NVIC_UNPEND0 + NVIC_OFFSET(INT_SCIF_TASK_ALERT)) = NVIC_BV(INT_SCIF_TASK_ALERT);
  HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGSCLR ) = AUX_EVCTL_EVTOAONFLAGSCLR_SWEV1;
	IntPendClear(INT_SCIF_TASK_ALERT);
} // osalClearTaskAlertInt



PROCESS(alert_interrupt, "Alert Interrupt");
PROCESS_THREAD(alert_interrupt, ev, data)
{
	PROCESS_BEGIN( );

	while(1) {
		PRINTF("Waiting for next ALERT interrupt\n");
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

		PRINTF("\nReceived alert interrupt\n");
		scifOsalEnableAuxDomainAccess();
		PRINTF("Analog values: %d %d %d %d %d\r\n\n", scifScsTaskData.analogSensor.output.anaValues[0],
				scifScsTaskData.analogSensor.output.anaValues[1],
				scifScsTaskData.analogSensor.output.anaValues[2],
				scifScsTaskData.analogSensor.output.anaValues[3],
				scifScsTaskData.analogSensor.output.anaValues[4]);

		//sensors_send( );

		//PROCESS_YIELD_UNTIL(ev == sender_finish_event);


		// update the schedule
		scifScsTaskData.analogSensor.cfg.scheduleDelay = config_get_sensor_interval();

		//osalIndicateTaskAlert( );
		osalClearTaskAlertInt();
		scifAckAlertEvents();


		//TODO can I disable the aux domain?  do I need to?
//		scifOsalDisableAuxDomainAccess();



	}

	PROCESS_END();
}


/** \brief Interrupt service route for ready interrupt
 */
static void osalTaskAlertIsr(void) {

	scifClearAlertIntSource();

	PRINTF("Alert interrupt generated. Calling Contiki process...\r\n");
	process_poll(&alert_interrupt);


	scifOsalEnableAuxDomainAccess();
	//osalClearTaskAlertInt();

//    osalDisableTaskAlertInt();
//    osalIndicateTaskAlert();
}



/** \brief Enters a critical section by disabling hardware interrupts
  *
  * \return
  *     Whether interrupts were enabled at threadye time this function was called
  */
uint32_t scifOsalEnterCriticalSection(void) {
    //return !CPUcpsid();
		return IntMasterDisable();
} // scifOsalEnterCriticalSection




/** \brief Leaves a critical section by reenabling hardware interrupts if previously enabled
  *
  * \param[in]      key
  *     The value returned by the previous corresponding call to \ref scifOsalEnterCriticalSection()
  */
void scifOsalLeaveCriticalSection(uint32_t key) {
    //if (key) CPUcpsie();
		if (key) IntMasterEnable();
} // scifOsalLeaveCriticalSection




///// Stores whether task control non-blocking functions have been locked
volatile bool osalCtrlTaskNblLocked = false;

int osalWaitNonBlock(int timeoutTicks)
{
	if (osalCtrlTaskNblLocked) return false;
	else return true;
}

/** \brief Locks use of task control non-blocking functions
  *
  * This function is used by the non-blocking task control to allow safe operation from multiple threads.
  *
  * The function shall attempt to set the \ref osalCtrlTaskNblLocked flag in a critical section.
  * Implementing a timeout is optional (the task control's non-blocking behavior is not associated with
  * this critical section, but rather with completion of the task control request).
  *
  * \return
  *     Whether the critical section could be entered (true if entered, false otherwise)
  */
static bool osalLockCtrlTaskNbl() {
    uint32_t key = scifOsalEnterCriticalSection();

    osalCtrlTaskNblLocked = osalWaitNonBlock(0);

    scifOsalLeaveCriticalSection(key);

    return osalCtrlTaskNblLocked;
} // osalLockCtrlTaskNbl




/** \brief Unlocks use of task control non-blocking functions
  *
  * This function will be called once after a successful \ref osalLockCtrlTaskNbl().
  */
static void osalUnlockCtrlTaskNbl(void) {
		osalCtrlTaskNblLocked = false;
} // osalUnlockCtrlTaskNbl




/** \brief Waits until the task control interface is ready/idle
  *
  * This indicates that the task control interface is ready for the first request or that the last
  * request has been completed. If a timeout mechanisms is not available, the implementation may be
  * simplified.
  *
  * \note For the OSAL "None" implementation, a non-zero timeout corresponds to infinite timeout.
  *
  * \param[in]      timeoutUs
  *     Minimum timeout, in microseconds
  *
  * \return
  *     Whether the task control interface is now idle/ready
  */
static bool osalWaitOnCtrlReady(uint32_t timeoutUs) {
//    if (timeoutUs) {
//        while (!(HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M));
//        return true;
//    } else {
//        return (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M);
//    }

	// first, ensure that the AUX_EVCTL_BASE module is available
	aux_ctrl_power_up();
	if (timeoutUs == 0)
		return ti_lib_aon_event_aux_wake_up_get(AON_EVENT_AUX_WU0) & AON_EVENT_AUX_SWEV0;

	clock_time_t start = clock_time();
	while( (clock_time() - start) > timeoutUs) {
			if (HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOAONFLAGS) & AUX_EVCTL_EVTOAONFLAGS_SWEV0_M)
				return true;
	}
	return false;
} // osalWaitOnCtrlReady




/// OSAL "None": Application-registered callback function for the task control READY interrupt
static SCIF_VFPTR osalIndicateCtrlReadyCallback = NULL;
/// OSAL "None": Application-registered callback function for the task ALERT interrupt
static SCIF_VFPTR osalIndicateTaskAlertCallback = NULL;




/** \brief Called by \ref osalCtrlReadyIsr() to notify the application
  *
  * This shall trigger a callback, generate a message/event etc.
  */
static void osalIndicateCtrlReady(void) {
    if (osalIndicateCtrlReadyCallback) {
        osalIndicateCtrlReadyCallback();
    }
} // osalIndicateCtrlReady




/** \brief Called by \ref osalTaskAlertIsr() to notify the application
  *
  * This shall trigger a callback, generate a message/event etc.
  */
static void osalIndicateTaskAlert(void) {
    if (osalIndicateTaskAlertCallback) {
        osalIndicateTaskAlertCallback();
    }
} // osalIndicateTaskAlert


//
//
///** \brief Sensor Controller READY interrupt service routine
//  *
//  * The ISR simply disables the interrupt and notifies the application.
//  *
//  * The interrupt flag is cleared and reenabled when sending the next task control request (by calling
//  * \ref scifExecuteTasksOnceNbl(), \ref scifStartTasksNbl() or \ref scifStopTasksNbl()).
//  */
//static void osalCtrlReadyIsr(void) {
//    osalDisableCtrlReadyInt();
//    osalIndicateCtrlReady();
//} // osalCtrlReadyIsr
//
//
//
//
///** \brief Sensor Controller ALERT interrupt service routine
//  *
//  * The ISR disables further interrupt generation and notifies the application. To clear the interrupt
//  * source, the application must call \ref scifClearAlertIntSource. The CPU interrupt flag is cleared and
//  * the interrupt is reenabled when calling \ref scifAckAlertEvents() to generate ACK.
//  */
//static void osalTaskAlertIsr(void) {
//    osalDisableTaskAlertInt();
//    osalIndicateTaskAlert();
//} // osalTaskAlertIsr
//
//


/** \brief OSAL "None": Registers the task control READY interrupt callback
  *
  * Using this callback is normally optional. See \ref osalIndicateCtrlReady() for details.
  *
  * \param[in]      callback
  *     Callback function pointer "void func(void)"
  */
void scifOsalRegisterCtrlReadyCallback(SCIF_VFPTR callback) {
    osalIndicateCtrlReadyCallback = callback;
} // scifOsalRegisterCtrlReadyCallback




/** \brief OSAL "None": Registers the task ALERT interrupt callback
  *
  * Using this callback is normally required. See \ref osalIndicateTaskAlert() for details.
  *
  * \param[in]      callback
  *     Callback function pointer "void func(void)"
  */
void scifOsalRegisterTaskAlertCallback(SCIF_VFPTR callback) {
    osalIndicateTaskAlertCallback = callback;
} // scifOsalRegisterTaskAlertCallback




/** \brief OSAL "None": Enables the AUX domain and Sensor Controller for access from the MCU domain
  *
  * This function must be called before accessing/using any of the following:
  * - Oscillator control registers
  * - AUX ADI registers
  * - AUX module registers and AUX RAM
  * - SCIF API functions, except \ref scifOsalEnableAuxDomainAccess()
  * - SCIF data structures
  *
  * The application is responsible for:
  * - Registering the last set access control state
  * - Ensuring that this control is thread-safe
  */
void scifOsalEnableAuxDomainAccess(void) {

		aux_ctrl_power_up();

//    // Force on AUX domain clock and bus connection
//    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) |= AON_WUC_AUXCTL_AUX_FORCE_ON_M;
//    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
//
//    // Wait for it to take effect
//    while (!(HWREG(AON_WUC_BASE + AON_WUC_O_PWRSTAT) & AON_WUC_PWRSTAT_AUX_PD_ON_M));

} // scifOsalEnableAuxDomainAccess




/** \brief OSAL "None": Disables the AUX domain and Sensor Controller for access from the MCU domain
  *
  * The application is responsible for:
  * - Registering the last set access control state
  * - Ensuring that this control is thread-safe
  */
void scifOsalDisableAuxDomainAccess(void) {

//    // Force on AUX domain bus connection
//    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) &= ~AON_WUC_AUXCTL_AUX_FORCE_ON_M;
//    HWREG(AON_RTC_BASE + AON_RTC_O_SYNC);
//
//    // Wait for it to take effect
//    while (HWREG(AON_WUC_BASE + AON_WUC_O_PWRSTAT) & AON_WUC_PWRSTAT_AUX_PD_ON_M);

	aux_ctrl_power_down(false);

} // scifOsalDisableAuxDomainAccess

/* ******************************************************************* */
static aux_consumer_module_t aux_consumer;


int sensor_aux_init( )
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

	// start-up the interrupt threads
	process_start(&ready_interrupt, NULL);
	process_start(&alert_interrupt, NULL);

	int rc = scifInit(&scifScsDriverSetup);
	if (rc != SCIF_SUCCESS) {
		PRINTF("Error during scifInit - %d\n", rc);
		return -1;
	}

//	rc = scifStartTasksNbl(BV(SCIF_SCS_ANALOG_SENSOR_TASK_ID));
//	if (rc != SCIF_SUCCESS) {
//		PRINTF("Error during scif control start: %d\n", rc);
//	} else {
//		PRINTF("Scif control start SUCCESSFUL\r\n");
//	}


	// using timer 1, enable reload mode
	HWREG(AUX_TIMER_BASE + AUX_TIMER_O_T1CFG) |= AUX_TIMER_T1CFG_RELOAD; // auto-reload


	return SCIF_SUCCESS;

}

#endif
//@}
