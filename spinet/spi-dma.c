/*
 * dma.c
 *
 *  Created on: Nov 15, 2018
 *      Author: Tom Briggs
 */
#include "contiki.h"
#include "process.h"

#include "spi-dma.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/udma.h)
#include DeviceFamily_constructPath(driverlib/ssi.h)


#include "sys/log.h"

#define INTR_RXFIN (0x001)
#define INTR_TXFIN (0x002)
#define INTR_RXERR (0x004)

#define INTR_OVFLOW_RXFIN (0x0100)
#define INTR_OVFLOW_TXFIN (0x0200)
#define INTR_OVFLOW_RXERR (0x0400)

process_event_t spidma_event;

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_DBG

static unsigned int intr_reason = 0;

struct process *proc_dma_notify = NULL;

void dma_set_callback (struct process *process)
{
	proc_dma_notify = process;
}

void dma_clear_event_all_flags ()
{
	intr_reason = 0;
}

void clear_event_flag (unsigned int mask)
{
	intr_reason = intr_reason & ~(mask);
}

unsigned int get_intr_reason (void)
{
	return intr_reason;
}

void dma_clr_oflow ()
{
	clear_event_flag (INTR_OVFLOW_RXFIN);
	clear_event_flag (INTR_OVFLOW_TXFIN);
	clear_event_flag (INTR_OVFLOW_RXERR);
}

void dma_clr_rxerr ()
{
	clear_event_flag ( INTR_RXERR);
}

void dma_clr_txfin ()
{
	clear_event_flag (INTR_TXFIN);
}

void dma_clr_rxfin ()
{
	clear_event_flag (INTR_RXFIN);
}

int dma_did_oflow ()
{
	if (intr_reason
			& (INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR))
		return 1;
	else
		return 0;
}

int dma_did_rxerr ()
{
	if (intr_reason & INTR_RXERR)
		return 1;
	else
		return 0;
}

int dma_did_txfin ()
{
	if (intr_reason & INTR_TXFIN)
		return 1;
	else
		return 0;
}

int dma_did_rxfin ()
{
	if (intr_reason & INTR_RXFIN)
		return 1;
	else
		return 0;
}



//void uDMAIntHandler(void)
void dmaSSI0IntHandler (void)
{
	  // clear the SSI interrupts (should already be cleared?)
	  uint32_t flags = SSIIntStatus(SSI0_BASE, 1);
	  uint32_t mode = 0;

	  SSIIntClear(SSI0_BASE, flags);

	  // there was a timeout - the last command aborted....
	  if ((flags & SSI_RXTO) || (flags & SSI_RXOR)) {
	      if (intr_reason & INTR_RXERR)
		intr_reason |= INTR_OVFLOW_RXERR;

	      intr_reason |= INTR_RXERR;
	  }
	  else {
	      // get the current DMA for the RX channel
	      mode = uDMAChannelModeGet(UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT);

	      // we've received what we expected, notify parent process
	      if(mode == UDMA_MODE_STOP)
	      {
		  SSIDMADisable(SSI0_BASE,SSI_DMA_RX);
		  uDMAChannelDisable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);

		  IntDisable(INT_SSI0_COMB );
		  if (intr_reason & INTR_RXFIN)
		    intr_reason |= INTR_OVFLOW_RXFIN;
		  intr_reason |= INTR_RXFIN;
	      }
	      else {
		  SSIDMADisable(SSI0_BASE,SSI_DMA_TX);
		  uDMAChannelDisable(UDMA0_BASE, UDMA_CHAN_SSI0_TX);

		  if (intr_reason & INTR_TXFIN)
		    intr_reason |= INTR_OVFLOW_TXFIN;
		  intr_reason |= INTR_TXFIN;
	      }

	      uDMAIntClear(UDMA0_BASE, 0xffffffff);
	  }

		if ((intr_reason != 0) && (proc_dma_notify != NULL))
		{
			IntPendClear(INT_SSI0_COMB);
			IntDisable(INT_SSI0_COMB );
			process_poll (proc_dma_notify);
		}

	}



void SSI0IntHandler (void)
{
	dmaSSI0IntHandler();
}

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif


static void dma_xfer_both (void *miso, void *mosi, int len)
{
	uDMAChannelControlSet (UDMA0_BASE,
    UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

	uDMAChannelTransferSet (UDMA0_BASE,
		UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
		UDMA_MODE_BASIC,
		(void*) (SSI0_BASE | SSI_O_DR),
	  mosi, len);

	uDMAChannelControlSet (UDMA0_BASE,
		UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

	uDMAChannelTransferSet (UDMA0_BASE,
		UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
		UDMA_MODE_BASIC,
	  miso,
		(void*) (SSI0_BASE | SSI_O_DR),
		len);

	SSIDMAEnable (SSI0_BASE, SSI_DMA_RX);
	SSIDMAEnable (SSI0_BASE, SSI_DMA_TX | SSI_DMA_RX);
	uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_TX);
	uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_RX);
	SSIIntEnable (SSI0_BASE, SSI_RXTO | SSI_RXOR);
	IntEnable(INT_SSI0_COMB);

}

static void dma_xfer_recv_only (void *mosi, int len)
{


  // start configuring the SSI0 RX & TX channels
  uDMAChannelAttributeDisable(UDMA0_BASE,
    UDMA_CHAN_SSI0_RX,
    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

  uDMAChannelControlSet(UDMA0_BASE,
		UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
    UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);


  uDMAChannelTransferSet(UDMA0_BASE,
  		UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
      UDMA_MODE_BASIC,
  		(void *) (SSI0_BASE | SSI_O_DR),
      mosi, len);

//  uDMAChannelAttributeDisable(UDMA0_BASE,
//		UDMA_CHAN_SSI0_TX,
//    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);
//
//  uDMAChannelControlSet (UDMA0_BASE,
//	  UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
//    UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  SSIDMAEnable(SSI0_BASE,SSI_DMA_RX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  //SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR);
  SSIIntEnable(SSI0_BASE, SSI_RXOR);

  IntEnable(INT_SSI0_COMB);

}

static void dma_xfer_xmit_only (void *miso, int len)
{
  uDMAChannelTransferSet(UDMA0_BASE,
		UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
    UDMA_MODE_BASIC,
		(void *) (SSI0_BASE | SSI_O_DR),
    miso, len);

  // start configuring the SSI0 RX & TX channels
  uDMAChannelAttributeDisable(UDMA0_BASE,
    UDMA_CHAN_SSI0_TX,
    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

  uDMAChannelControlSet(UDMA0_BASE,
		UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
    UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  uDMAChannelAttributeDisable(UDMA0_BASE,
		UDMA_CHAN_SSI0_RX,
    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

  uDMAChannelControlSet (UDMA0_BASE,
	  UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
    UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

  SSIDMAEnable(SSI0_BASE,SSI_DMA_TX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_TX);
  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR);
  IntEnable(INT_SSI0_COMB);

}

// if slv_out is null, then recv only
// if slv_in is null, then xmit only
int dma_xfer (void *miso, void *mosi, int len)
{
	intr_reason = 0;

	if (len == 0)
	{
		LOG_ERR("Error - dma transfers must be > 0\n");
		return -1;
	}
	if (dma_in_progress ())
	{
		LOG_ERR("Error - DMA xfer already in progress, aborting.\n");
		return -1;
	}

	// there are four types of DMA transactions
	if ((miso == NULL) && (mosi == NULL)) {
		LOG_ERR("Error - cannot use DMA with neither input nor output\n");
		return -1;
	}
	else if (miso == NULL) {
		LOG_DBG("Recv only started %d bytes pi -> %p\n", len, mosi);
		dma_xfer_recv_only(mosi, len);
	}
	else if (mosi == NULL) {
		LOG_DBG("Xmit only started %p -> pi %d bytes\n", miso, len);
		dma_xfer_xmit_only(miso, len);
	}
	else {
		LOG_DBG("Bidir started %p -> pi -> %p %d\n", miso, mosi, len);
		dma_xfer_both(miso, mosi, len);
	}

	return 0;
}

void dma_init ()
{
	LOG_DBG("Initializing uDMA controller\n");
	// configure the PRCM power domains

	LOG_DBG("Enabling PRCM power domain UDMA\n");
	PRCMPeripheralRunEnable(PRCM_PERIPH_UDMA);
	PRCMPeripheralSleepEnable(PRCM_PERIPH_UDMA);
	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_UDMA);
	PRCMLoadSet();
	while (PRCMLoadGet() == 0) ;


	// configure dma base registers
	uDMAControlBaseSet (UDMA0_BASE, &pui8ControlTable);

	// enable the DMA controller module
	uDMAEnable (UDMA0_BASE);

	// start configuring the SSI0 RX & TX channels
	uDMAChannelAttributeDisable (UDMA0_BASE,
		UDMA_CHAN_SSI0_RX,
		UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY| UDMA_ATTR_REQMASK);

	uDMAChannelControlSet (UDMA0_BASE,
	  UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

	uDMAChannelAttributeDisable (UDMA0_BASE,
	 UDMA_CHAN_SSI0_TX,
	 UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

	uDMAChannelControlSet (	UDMA0_BASE,
	  UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
		UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

	spidma_event = process_alloc_event ();

	SSIIntRegister(SSI0_BASE, dmaSSI0IntHandler);
	SSIIntEnable(SSI0_BASE, SSI_RXOR | SSI_RXTO );
}

int dma_in_progress ()
{
	int state = (uDMAGetStatus (UDMA0_BASE) >> 4) & 0x0f;
	if ((state == 0x00) || (state == 0x08))
		return 0;
	else
		return 1;
}

