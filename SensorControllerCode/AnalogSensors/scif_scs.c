/// \addtogroup module_scif_driver_setup
//@{
#include "scif_scs.h"
#include "scif_framework.h"
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_aon_event.h>
#include <inc/hw_aon_rtc.h>
#include <inc/hw_aon_wuc.h>
#include <inc/hw_aux_sce.h>
#include <inc/hw_aux_smph.h>
#include <inc/hw_aux_evctl.h>
#include <inc/hw_aux_aiodio.h>
#include <inc/hw_aux_timer.h>
#include <inc/hw_aux_wuc.h>
#include <inc/hw_event.h>
#include <inc/hw_ints.h>
#include <inc/hw_ioc.h>
#include <string.h>
#if defined(__IAR_SYSTEMS_ICC__)
    #include <intrinsics.h>
#endif


// OSAL function prototypes
uint32_t scifOsalEnterCriticalSection(void);
void scifOsalLeaveCriticalSection(uint32_t key);




/// Firmware image to be uploaded to the AUX RAM
static const uint16_t pAuxRamImage[] = {
    /*0x0000*/ 0x1408, 0x040C, 0x1408, 0x042C, 0x1408, 0x0447, 0x1408, 0x044D, 0x4436, 0x2437, 0xAEFE, 0xADB7, 0x6442, 0x7000, 0x7C6B, 0x6870, 
    /*0x0020*/ 0x0068, 0x1425, 0x6871, 0x0069, 0x1425, 0x6872, 0x006A, 0x1425, 0x786B, 0xF801, 0xFA01, 0xBEF2, 0x786E, 0x6870, 0xFD0E, 0x6872, 
    /*0x0040*/ 0xED92, 0xFD06, 0x7C6E, 0x642D, 0x0451, 0x786B, 0x8F1F, 0xED8F, 0xEC01, 0xBE01, 0xADB7, 0x8DB7, 0x6630, 0x6542, 0x0000, 0x186E, 
    /*0x0060*/ 0x9D88, 0x9C01, 0xB60D, 0x1067, 0xAF19, 0xAA00, 0xB609, 0xA8FF, 0xAF39, 0xBE06, 0x0C6B, 0x8869, 0x8F08, 0xFD47, 0x9DB7, 0x086B, 
    /*0x0080*/ 0x8801, 0x8A01, 0xBEEC, 0x262F, 0xAEFE, 0x4630, 0x0451, 0x5527, 0x6642, 0x0000, 0x0C6B, 0x140B, 0x0451, 0x6742, 0x86FF, 0x03FF, 
    /*0x00A0*/ 0x0C6D, 0x786C, 0xCD47, 0x686D, 0xCD06, 0xB605, 0x0000, 0x0C6C, 0x7C6F, 0x652D, 0x0C6D, 0x786D, 0xF801, 0xE92B, 0xFD0E, 0xBE01, 
    /*0x00C0*/ 0x6436, 0xBDB7, 0x241A, 0xA6FE, 0xADB7, 0x641A, 0xADB7, 0x0000, 0x007C, 0x0084, 0x00C7, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 
    /*0x00E0*/ 0x0000, 0x0000, 0x0000, 0x0007, 0x0006, 0x0005, 0x0004, 0x0064, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6073, 0xFF1E, 0x14C8, 
    /*0x0100*/ 0x8801, 0x8A04, 0xAEFA, 0xADB7, 0x7000, 0x14D5, 0x702F, 0x6009, 0x14DD, 0x705D, 0x6003, 0x14DD, 0x7041, 0x1462, 0xFB4D, 0x8609, 
    /*0x0120*/ 0x7101, 0x6431, 0x2531, 0xA6FE, 0xFB00, 0x7018, 0xFB4C, 0x7003, 0xFB4C, 0xFD47, 0xFB4C, 0x1465, 0x6403, 0x0000, 0x1000, 0x6073, 
    /*0x0140*/ 0xFF1E, 0x14C8, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x9D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x9D26, 0x6403, 0x601F, 
    /*0x0160*/ 0xEB2C, 0xFDB1, 0xE902, 0x9D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x9D26, 0x9DAA, 0x6078, 0x9F3E, 0x8801, 0x8A04, 0xAEDE, 
    /*0x0180*/ 0x14E3, 0x7000, 0x14EE, 0x086C, 0x8201, 0x0C6C, 0xADB7, 0xADB7, 0xF007, 0x1462, 0x86FF, 0x63F8, 0xEB51, 0x8680, 0x6000, 0xED8F, 
    /*0x01A0*/ 0xEB49, 0xFD47, 0xEB49, 0x1465, 0xADB7, 0xED47, 0xEDAB, 0xE814, 0xF007, 0x5001, 0xDD87, 0xDF26, 0xADB7, 0xFB0C, 0xEDA4, 0xEB09, 
    /*0x01C0*/ 0x640B, 0xCDB1, 0xADB7, 0x1462, 0x7079, 0xFB55, 0x71FB, 0xFB54, 0xFD47, 0xFB54, 0x1465, 0x4431, 0x4400, 0xADB7, 0xED47, 0xEDAB, 
    /*0x01E0*/ 0xE816, 0xF007, 0x5001, 0xDD87, 0xDF26, 0xADB7
};


/// Look-up table that converts from AUX I/O index to MCU IOCFG offset
static const uint8_t pAuxIoIndexToMcuIocfgOffsetLut[] = {
    120, 116, 112, 108, 104, 100, 96, 92, 28, 24, 20, 16, 12, 8, 4, 0
};


/** \brief Look-up table of data structure information for each task
  *
  * There is one entry per data structure (\c cfg, \c input, \c output and \c state) per task:
  * - [31:24] Data structure size (number of 16-bit words)
  * - [23:16] Buffer count (when 2+, first data structure is preceded by buffering control variables)
  * - [15:0] Address of the first data structure
  */
static const uint32_t pScifTaskDataStructInfoLut[] = {
//  cfg         input       output      state       
    0x005010E6, 0x00000000, 0x004010F0, 0x00000000  // AnalogSensor
};




// No task-specific initialization functions




// No task-specific uninitialization functions




/** \brief Initilializes task resource hardware dependencies
  *
  * This function is called by the internal driver initialization function, \ref scifInit().
  */
static void scifTaskResourceInit(void) {
    scifInitIo(7, AUXIOMODE_ANALOG, -1, 0);
    scifInitIo(6, AUXIOMODE_ANALOG, -1, 0);
    scifInitIo(5, AUXIOMODE_ANALOG, -1, 0);
    scifInitIo(4, AUXIOMODE_ANALOG, -1, 0);
    scifInitIo(0, AUXIOMODE_OPEN_DRAIN, -1, 0);
} // scifTaskResourceInit




/** \brief Uninitilializes task resource hardware dependencies
  *
  * This function is called by the internal driver uninitialization function, \ref scifUninit().
  */
static void scifTaskResourceUninit(void) {
    scifUninitIo(7, -1);
    scifUninitIo(6, -1);
    scifUninitIo(5, -1);
    scifUninitIo(4, -1);
    scifUninitIo(0, 1);
} // scifTaskResourceUninit




/** \brief Re-initializes I/O pins used by the specified tasks
  *
  * It is possible to stop a Sensor Controller task and let the System CPU borrow and operate its I/O
  * pins. For example, the Sensor Controller can operate an SPI interface in one application state while
  * the System CPU with SSI operates the SPI interface in another application state.
  *
  * This function must be called before \ref scifExecuteTasksOnceNbl() or \ref scifStartTasksNbl() if
  * I/O pins belonging to Sensor Controller tasks have been borrowed System CPU peripherals.
  *
  * \param[in]      bvTaskIds
  *     Bit-vector of task IDs for the task I/Os to be re-initialized
  */
void scifScsReinitTaskIo(uint32_t bvTaskIds) {
    if (bvTaskIds & (1 << SCIF_SCS_ANALOG_SENSOR_TASK_ID)) {
        scifReinitIo(7, -1);
        scifReinitIo(6, -1);
        scifReinitIo(5, -1);
        scifReinitIo(4, -1);
        scifReinitIo(0, -1);
    }
} // scifScsReinitTaskIo




/// Driver setup data, to be used in the call to \ref scifInit()
const SCIF_DATA_T scifScsDriverSetup = {
    (volatile SCIF_INT_DATA_T*) 0x400E00D6,
    (volatile SCIF_TASK_CTRL_T*) 0x400E00DC,
    (volatile uint16_t*) 0x400E00CE,
    0x0000,
    sizeof(pAuxRamImage),
    pAuxRamImage,
    pScifTaskDataStructInfoLut,
    pAuxIoIndexToMcuIocfgOffsetLut,
    scifTaskResourceInit,
    scifTaskResourceUninit
};




// No task-specific API available


//@}
