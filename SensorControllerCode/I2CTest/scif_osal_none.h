/** \addtogroup module_scif_osal Operating System Abstraction Layer
  *
  * \section section_osal_overview Overview
  * The OSAL provides a set of functions for internal use by the SCIF driver, which allows it to
  * interface seamlessly with the real-time operating system or other run-time framework running on the
  * System CPU. These functions are listed below.
  *
  * The OSAL also implements OS-dependent functions that must be called by the application. These
  * functions are also listed below.
  *
  * The OSAL C source file can, but does not need to, be included in the application project.
  *
  *
  * \section section_osal_implementation Implementation "None"
  * This OSAL uses the following basic mechanisms:
  * - Hardware event interrupts to implement the SCIF READY and ALERT callbacks
  * - Critical sections by disabling interrupts globally
  *
  * This OSAL does not implement the \ref scifWaitOnNbl() timeout mechanism.
  *
  * The application must call the \ref scifOsalEnableAuxDomainAccess() and
  * \ref scifOsalDisableAuxDomainAccess() functions to enable and disable access to the AUX domain. See
  * \ref section_scif_aux_domain_access for more information.
  *
  *
  * \section section_osal_app_func Application Functions
  * This SCIF OSAL implementation provides the following functions for use by the application:
  * - AUX domain access control:
  *     - \ref scifOsalEnableAuxDomainAccess()
  *     - \ref scifOsalDisableAuxDomainAccess()
  * - Callback registering:
  *     - \ref scifOsalRegisterCtrlReadyCallback()
  *     - \ref scifOsalRegisterTaskAlertCallback()
  *
  * \section section_osal_int_func Mandatory Internal Functions
  * The SCIF OSAL must provide the following functions for internal use by the driver:
  * - Task control READY interrupt:
  *     - \ref osalCtrlReadyIsr()
  *     - \ref osalRegisterCtrlReadyInt()
  *     - \ref osalUnregisterCtrlReadyInt()
  *     - \ref osalEnableCtrlReadyInt()
  *     - \ref osalDisableCtrlReadyInt()
  *     - \ref osalClearCtrlReadyInt()
  * - Task ALERT interrupt:
  *     - \ref osalTaskAlertIsr()
  *     - \ref osalRegisterTaskAlertInt()
  *     - \ref osalUnregisterTaskAlertInt()
  *     - \ref scifOsalEnableTaskAlertInt()
  *     - \ref scifOsalDisableTaskAlertInt()
  *     - \ref osalClearTaskAlertInt()
  * - Thread-safe operation:
  *     - \ref scifOsalEnterCriticalSection()
  *     - \ref scifOsalLeaveCriticalSection()
  *     - \ref osalLockCtrlTaskNbl()
  *     - \ref osalUnlockCtrlTaskNbl()
  * - Task control support:
  *     - \ref osalWaitOnCtrlReady()
  * - Application notifications:
  *     - \ref osalIndicateCtrlReady()
  *     - \ref osalIndicateTaskAlert()
  *
  * @{
  */
#ifndef SCIF_OSAL_NONE_H
#define SCIF_OSAL_NONE_H


// OSAL "None": Callback registering functions
void scifOsalRegisterCtrlReadyCallback(SCIF_VFPTR callback);
void scifOsalRegisterTaskAlertCallback(SCIF_VFPTR callback);

// OSAL "None": Functions for temporarily disabling ALERT interrupts/callbacks
void scifOsalEnableTaskAlertInt();
void scifOsalDisableTaskAlertInt();

// OSAL "None": AUX domain access functions
void scifOsalEnableAuxDomainAccess(void);
void scifOsalDisableAuxDomainAccess(void);


#endif
//@}


// Generated by MCT163S09 at 2018-05-01 17:47:36.911
