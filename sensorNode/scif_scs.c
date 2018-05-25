/// \addtogroup module_scif_driver_setup
//@{
#include "scif_scs.h"
#include "scif_framework.h"
#undef DEVICE_FAMILY_PATH
#if defined(DEVICE_FAMILY)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/DEVICE_FAMILY/x>
#elif defined(DeviceFamily_CC26X0)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc26x0/x>
#elif defined(DeviceFamily_CC26X0R2)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc26x0r2/x>
#elif defined(DeviceFamily_CC13X0)
    #define DEVICE_FAMILY_PATH(x) <ti/devices/cc13x0/x>
#else
    #define DEVICE_FAMILY_PATH(x) <x>
#endif
#include DEVICE_FAMILY_PATH(inc/hw_types.h)
#include DEVICE_FAMILY_PATH(inc/hw_memmap.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_event.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_rtc.h)
#include DEVICE_FAMILY_PATH(inc/hw_aon_wuc.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_sce.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_smph.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_evctl.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_aiodio.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_timer.h)
#include DEVICE_FAMILY_PATH(inc/hw_aux_wuc.h)
#include DEVICE_FAMILY_PATH(inc/hw_event.h)
#include DEVICE_FAMILY_PATH(inc/hw_ints.h)
#include DEVICE_FAMILY_PATH(inc/hw_ioc.h)
#include <string.h>
#if defined(__IAR_SYSTEMS_ICC__)
    #include <intrinsics.h>
#endif


// OSAL function prototypes
uint32_t scifOsalEnterCriticalSection(void);
void scifOsalLeaveCriticalSection(uint32_t key);




/// Firmware image to be uploaded to the AUX RAM
static const uint16_t pAuxRamImage[] = {
    /*0x0000*/ 0x1408, 0x040C, 0x1408, 0x042C, 0x1408, 0x0447, 0x1408, 0x044D, 0x4436, 0x2437, 0xAEFE, 0xADB7, 0x6442, 0x7000, 0x7C6B, 0x6872, 
    /*0x0020*/ 0x0068, 0x1425, 0x6873, 0x0069, 0x1425, 0x6874, 0x006A, 0x1425, 0x786B, 0xF801, 0xFA01, 0xBEF2, 0x7870, 0x6872, 0xFD0E, 0x6874, 
    /*0x0040*/ 0xED92, 0xFD06, 0x7C70, 0x642D, 0x0450, 0x786B, 0x8F1F, 0xED8F, 0xEC01, 0xBE01, 0xADB7, 0x8DB7, 0x6630, 0x6542, 0x0000, 0x1870, 
    /*0x0060*/ 0x9D88, 0x9C01, 0xB60D, 0x1067, 0xAF19, 0xAA00, 0xB609, 0xA8FF, 0xAF39, 0xBE06, 0x0C6B, 0x8869, 0x8F08, 0xFD47, 0x9DB7, 0x086B, 
    /*0x0080*/ 0x8801, 0x8A01, 0xBEEC, 0x262F, 0xAEFE, 0x4630, 0x0450, 0x5527, 0x6642, 0x0000, 0x0C6B, 0x140B, 0x0450, 0x6742, 0x03FF, 0x0C6D, 
    /*0x00A0*/ 0x786C, 0x686D, 0xED37, 0xB605, 0x0000, 0x0C6C, 0x7C71, 0x652D, 0x0C6D, 0x786D, 0x686E, 0xFD0E, 0xF801, 0xE92B, 0xFD0E, 0xBE01, 
    /*0x00C0*/ 0x6436, 0xBDB7, 0x241A, 0xA6FE, 0xADB7, 0x641A, 0xADB7, 0x0000, 0x0088, 0x0089, 0x0227, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 
    /*0x00E0*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0007, 0x0006, 0x0005, 0x0004, 0x0003, 0x0008, 0x0009, 0x0000, 0x0000, 0x0000, 0x0000, 
    /*0x0100*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xADB7, 0x0000, 0x0C81, 0x787A, 0x1628, 0x7014, 0x600D, 0x1630, 
    /*0x0120*/ 0x0080, 0x6000, 0x163C, 0x6C87, 0x7052, 0x6887, 0x1653, 0x6C87, 0xFD40, 0xF212, 0x6887, 0x1653, 0x6C87, 0x6887, 0x1636, 0x6C87, 
    /*0x0140*/ 0x7053, 0x6887, 0x1653, 0x6C87, 0x7001, 0x6887, 0x1671, 0x6C87, 0x7C7F, 0x6887, 0x1690, 0x6C87, 0x0887, 0x8A00, 0xB603, 0x0881, 
    /*0x0160*/ 0x8201, 0x0C81, 0x0080, 0x6000, 0x163C, 0x6C87, 0x7052, 0x6887, 0x1653, 0x6C87, 0xFD40, 0xF201, 0x6887, 0x1653, 0x6C87, 0x70F6, 
    /*0x0180*/ 0x6887, 0x1653, 0x6C87, 0x6887, 0x1690, 0x6C87, 0x0887, 0x8A00, 0xB603, 0x0881, 0x8202, 0x0C81, 0x0080, 0x6000, 0x163C, 0x6C87, 
    /*0x01A0*/ 0x7052, 0x6887, 0x1653, 0x6C87, 0xFD40, 0xF20F, 0x6887, 0x1653, 0x6C87, 0x7001, 0x6887, 0x1653, 0x6C87, 0x6887, 0x1690, 0x6C87, 
    /*0x01C0*/ 0x0887, 0x8A00, 0xB603, 0x0881, 0x8204, 0x0C81, 0x0080, 0x6000, 0x163C, 0x6C87, 0x7052, 0x6887, 0x1653, 0x6C87, 0xFD40, 0x6887, 
    /*0x01E0*/ 0x1653, 0x6C87, 0x7001, 0x6887, 0x1653, 0x6C87, 0x6887, 0x1690, 0x6C87, 0x0887, 0x8A00, 0xB603, 0x0881, 0x8208, 0x0C81, 0x787B, 
    /*0x0200*/ 0x169B, 0x0000, 0x7002, 0x600E, 0x1630, 0x8801, 0x8A05, 0xAEFA, 0x0080, 0x6000, 0x163C, 0x6C87, 0x7052, 0x6887, 0x1653, 0x6C87, 
    /*0x0220*/ 0xFD40, 0x6887, 0x1653, 0x6C87, 0x7003, 0x6887, 0x1653, 0x6C87, 0x6887, 0x1690, 0x6C87, 0x0887, 0x8A00, 0xB603, 0x0881, 0x8210, 
    /*0x0240*/ 0x0C81, 0x0000, 0x7002, 0x600E, 0x1630, 0x8801, 0x8A18, 0xAEFA, 0x0080, 0x6000, 0x163C, 0x6C87, 0x7052, 0x6887, 0x1653, 0x6C87, 
    /*0x0260*/ 0xFD40, 0xF214, 0x6887, 0x1653, 0x6C87, 0x6887, 0x1636, 0x6C87, 0x7053, 0x6887, 0x1653, 0x6C87, 0x7000, 0x6887, 0x1671, 0x6C87, 
    /*0x0280*/ 0x8D47, 0x7000, 0x6887, 0x1671, 0x6C87, 0xFDA0, 0xFD08, 0x7C84, 0x7000, 0x6887, 0x1671, 0x6C87, 0x8D47, 0x7000, 0x6887, 0x1671, 
    /*0x02A0*/ 0x6C87, 0xFDA0, 0xFD08, 0x7C86, 0x7000, 0x6887, 0x1671, 0x6C87, 0x8D47, 0x7000, 0x6887, 0x1671, 0x6C87, 0xFDA0, 0xFD08, 0x7C85, 
    /*0x02C0*/ 0x7000, 0x6887, 0x1671, 0x6C87, 0x8D47, 0x7001, 0x6887, 0x1671, 0x6C87, 0xFDA0, 0xFD08, 0x7C83, 0x6887, 0x1690, 0x6C87, 0x787B, 
    /*0x02E0*/ 0x1628, 0x702F, 0x6009, 0x1630, 0xF502, 0xFD47, 0xFD47, 0xFD47, 0x7041, 0x1462, 0xFB4D, 0x8609, 0x7101, 0x6431, 0x2531, 0xA6FE, 
    /*0x0300*/ 0xFB00, 0x7078, 0xFB54, 0x7018, 0xFB4C, 0x7003, 0xFB4C, 0xFD47, 0xFB4C, 0x1465, 0x6403, 0x001F, 0x8B2C, 0xFDB1, 0x8902, 0x0000, 
    /*0x0320*/ 0x7875, 0x16A3, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 
    /*0x0340*/ 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x8DAA, 0x0C7E, 0x0000, 0x7876, 0x16A3, 0x6403, 
    /*0x0360*/ 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 
    /*0x0380*/ 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x8DAA, 0x0C82, 0x0000, 0x7877, 0x16A3, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 
    /*0x03A0*/ 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 
    /*0x03C0*/ 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x8DAA, 0x0C7C, 0x0000, 0x7878, 0x16A3, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 
    /*0x03E0*/ 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 
    /*0x0400*/ 0x8D26, 0x8DAA, 0x0C7D, 0x0000, 0x7879, 0x16A3, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 
    /*0x0420*/ 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x6403, 0x601F, 0xEB2C, 0xFDB1, 0xE902, 0x8D26, 0x8DAA, 0x0C80, 
    /*0x0440*/ 0x16B0, 0x787A, 0x169B, 0x086C, 0x8201, 0x0C6C, 0xADB7, 0xADB7, 0xED47, 0xEDAB, 0xE816, 0xF007, 0x5001, 0xDD87, 0xDF26, 0xADB7, 
    /*0x0460*/ 0xFB0C, 0xEDA4, 0xEB09, 0x640B, 0xCDB1, 0xADB7, 0x53C6, 0x1641, 0x670F, 0x1644, 0x53C6, 0x1641, 0x460F, 0x53CA, 0x1641, 0x470F, 
    /*0x0480*/ 0xADB7, 0xD802, 0xDEFE, 0xADB7, 0x5338, 0x2713, 0xAE0B, 0x2713, 0xAE09, 0x2713, 0xAE07, 0x2713, 0xAE05, 0x2713, 0xAE03, 0xD801, 
    /*0x04A0*/ 0xBEF4, 0xE202, 0xADB7, 0xEA00, 0xBE1B, 0xB50E, 0xFDA1, 0x8601, 0xFC00, 0xB602, 0x660F, 0x8E02, 0x460F, 0xFD47, 0x53CC, 0x1641, 
    /*0x04C0*/ 0x670F, 0x1644, 0x53CF, 0x1641, 0x470F, 0x660F, 0x53C6, 0x1641, 0x670F, 0x1644, 0x53CC, 0x1641, 0x2613, 0xA601, 0xE201, 0x470F, 
    /*0x04E0*/ 0xADB7, 0xEA00, 0xBE1C, 0xB50B, 0x660F, 0x53C7, 0x1641, 0x670F, 0x1644, 0x53D1, 0x1641, 0xFDA1, 0x2613, 0xA601, 0xF201, 0x470F, 
    /*0x0500*/ 0x8601, 0xFC00, 0xB602, 0x660F, 0x8E02, 0x460F, 0xFD47, 0x53CB, 0x1641, 0x670F, 0x1644, 0x53CF, 0x1641, 0x470F, 0xF0FF, 0xADB7, 
    /*0x0520*/ 0x460F, 0x53C6, 0x1641, 0x670F, 0x1644, 0x53CF, 0x1641, 0x660F, 0x53C7, 0x1641, 0xADB7, 0xED47, 0xEDAB, 0xE814, 0xF007, 0x5001, 
    /*0x0540*/ 0xDD87, 0xDF26, 0xADB7, 0xF007, 0x1462, 0x86FF, 0x63F8, 0xEB51, 0x8680, 0x6000, 0xED8F, 0xEB49, 0xFD47, 0xEB49, 0x1465, 0xADB7, 
    /*0x0560*/ 0x1462, 0x7079, 0xFB55, 0x71FB, 0xFB54, 0xFD47, 0xFB54, 0x1465, 0x4431, 0x4400, 0xADB7
};


/// Look-up table that converts from AUX I/O index to MCU IOCFG offset
static const uint8_t pAuxIoIndexToMcuIocfgOffsetLut[] = {
    120, 116, 112, 108, 104, 100, 96, 92, 28, 24, 20, 16, 12, 8, 4, 0
};


/** \brief Look-up table of data structure information for each task
  *
  * There is one entry per data structure (\c cfg, \c input, \c output and \c state) per task:
  * - [31:20] Data structure size (number of 16-bit words)
  * - [19:12] Buffer count (when 2+, first data structure is preceded by buffering control variables)
  * - [11:0] Address of the first data structure
  */
static const uint32_t pScifTaskDataStructInfoLut[] = {
//  cfg         input       output      state       
    0x007010EA, 0x00000000, 0x00B010F8, 0x0010110E  // ReadData
};




// No run-time logging task data structure signatures needed in this project




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
    scifInitIo(3, AUXIOMODE_ANALOG, -1, 0);
    scifInitIo(8, AUXIOMODE_OUTPUT, -1, 0);
    scifInitIo(9, AUXIOMODE_OUTPUT, -1, 0);
    scifInitIo(11, AUXIOMODE_OPEN_DRAIN_WITH_INPUT, -1, 1);
    HWREG((IOC_BASE + IOC_O_IOCFG0) + pAuxIoIndexToMcuIocfgOffsetLut[11]) |= IOC_IOCFG0_HYST_EN_M;
    scifInitIo(10, AUXIOMODE_OPEN_DRAIN_WITH_INPUT, -1, 1);
    HWREG((IOC_BASE + IOC_O_IOCFG0) + pAuxIoIndexToMcuIocfgOffsetLut[10]) |= IOC_IOCFG0_HYST_EN_M;
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
    scifUninitIo(3, -1);
    scifUninitIo(8, 0);
    scifUninitIo(9, 0);
    scifUninitIo(11, -1);
    scifUninitIo(10, -1);
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
    if (bvTaskIds & (1 << SCIF_SCS_READ_DATA_TASK_ID)) {
        scifReinitIo(7, -1);
        scifReinitIo(6, -1);
        scifReinitIo(5, -1);
        scifReinitIo(4, -1);
        scifReinitIo(3, -1);
        scifReinitIo(8, -1);
        scifReinitIo(9, -1);
        scifReinitIo(11, -1);
        HWREG((IOC_BASE + IOC_O_IOCFG0) + pAuxIoIndexToMcuIocfgOffsetLut[11]) |= IOC_IOCFG0_HYST_EN_M;
        scifReinitIo(10, -1);
        HWREG((IOC_BASE + IOC_O_IOCFG0) + pAuxIoIndexToMcuIocfgOffsetLut[10]) |= IOC_IOCFG0_HYST_EN_M;
    }
} // scifScsReinitTaskIo




/// Driver setup data, to be used in the call to \ref scifInit()
const SCIF_DATA_T scifScsDriverSetup = {
    (volatile SCIF_INT_DATA_T*) 0x400E00D6,
    (volatile SCIF_TASK_CTRL_T*) 0x400E00E0,
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


// Generated by TBRIGGS-PC at 2018-05-25 16:21:42.563
