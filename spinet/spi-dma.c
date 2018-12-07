/*
 * dma.c
 *
 *  Created on: Nov 15, 2018
 *      Author: contiki
 */


#include "spi-dma.h"
#include "ti-lib.h"
#include "spinet.h"

#include "driverlib/prcm.h"
#include "driverlib/udma.h"
#include "sys/log.h"

#define LOG_MODULE "DMA"
#define LOG_LEVEL LOG_LEVEL_INFO


#define INTR_RXFIN (0x001)
#define INTR_TXFIN (0x002)
#define INTR_RXERR (0x004)

#define INTR_OVFLOW_RXFIN (0x0100)
#define INTR_OVFLOW_TXFIN (0x0200)
#define INTR_OVFLOW_RXERR (0x0400)

static unsigned int intr_reason = 0;
process_event_t dma_event;

#define event_flags intr_reason

void clear_event_all_flags( )
{
  event_flags = 0;
}


void clear_event_flag( unsigned int mask )
{
  event_flags = event_flags & ~(mask);
}

unsigned int get_event_flags( void )
{
  return event_flags;
}

/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */
PROCESS(dma_ctrl, "dma int handler");

PROCESS_THREAD(dma_ctrl, ev, data)
{

  PROCESS_BEGIN();


  // enter main loop
  while (1)
  {
      PROCESS_WAIT_EVENT();
      LOG_DBG("Received event: %x, overflow: %d rxerr: %x rxfin: %x txfin: %x\n",
	      intr_reason, intr_reason & (INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR),
	      intr_reason & INTR_RXERR, intr_reason & INTR_RXFIN, intr_reason & INTR_TXFIN);

      if (intr_reason & (INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR)) {
	  LOG_ERR("Error - interrupt overflow detected: %x\n", (unsigned int) intr_reason);
	  event_flags = intr_reason & (INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR);
      }

      if (intr_reason & INTR_RXERR) {
	  event_flags |= INTR_RXERR;
      }

      if (intr_reason & INTR_RXFIN) {
	  event_flags |= INTR_RXFIN;
	  LOG_DBG("Clearing SPI intr on rx fin\n");
	  clear_spi_intr();
      }

      if (intr_reason & INTR_TXFIN) {
	  event_flags |= INTR_TXFIN;

      }

      if (intr_reason != 0) {
	process_post(&spislv_ctrl, dma_event, (int * )&event_flags);
      }

  }

  PROCESS_END();
}

//void uDMAIntHandler(void)
void SSI0IntHandler(void)
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

	  ti_lib_int_disable(INT_SSI0_COMB );
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

  if (intr_reason != 0)
    process_poll(&dma_ctrl);

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


void dma_xfer(void *slv_out, void *slv_in, int len)
{
  static int trash_in, trash_out;

  if (len == 0) {
      LOG_ERR("Error - dma transfers must be > 0\n");
      return;
  }
  if (dma_in_progress()) {
      LOG_ERR("Error - DMA xfer already in progress, aborting.\n");
      return;
  }

  char *ptr = (char *) slv_out;
  LOG_DBG("Initiating xfer: %d\n", len);
  LOG_DBG("   %x %x %x %x\n", ptr[0], ptr[1], ptr[2], ptr[3]);
  if (len >= 8)  LOG_DBG("   %x %x %x %x\n", ptr[4], ptr[5], ptr[6], ptr[7]);
  if (len >= 12)  LOG_DBG("   %x %x %x %x\n", ptr[8], ptr[9], ptr[10], ptr[11]);
  if (len >= 16)  LOG_DBG("   %x %x %x %x\n", ptr[12], ptr[13], ptr[14], ptr[15]);


  if (slv_in == NULL) {
      // we don't care at all about the data coming in... but we need to clear the FIFO
      uDMAChannelControlSet(
          UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE | UDMA_ARB_1);

      uDMAChannelTransferSet(UDMA0_BASE,
    			 UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
            		 UDMA_MODE_BASIC,
    			 (void *) (SSI0_BASE | SSI_O_DR),
            		 &trash_in, len);

  }
  else {
      uDMAChannelControlSet(
          UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

      uDMAChannelTransferSet(UDMA0_BASE,
    			 UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
            		 UDMA_MODE_BASIC,
    			 (void *) (SSI0_BASE | SSI_O_DR),
            		 slv_in, len);

  }


  if (slv_out == NULL) {
      trash_out = 0xffff;

      uDMAChannelControlSet (
               UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
               UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE | UDMA_ARB_1);

      uDMAChannelTransferSet (UDMA0_BASE,
   			  UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
   			  UDMA_MODE_BASIC,
   			  &trash_out,
   			  (void *) (SSI0_BASE | SSI_O_DR),
   			  len);


  }
  else {
      uDMAChannelControlSet (
            UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
            UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

      uDMAChannelTransferSet (UDMA0_BASE,
			  UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
			  UDMA_MODE_BASIC,
			  slv_out,
			  (void *) (SSI0_BASE | SSI_O_DR),
			  len);
  }
  SSIDMAEnable(SSI0_BASE,SSI_DMA_RX);
  SSIDMAEnable(SSI0_BASE,SSI_DMA_TX | SSI_DMA_RX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_TX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR);
  ti_lib_int_enable(INT_SSI0_COMB);

  LOG_DBG("Raising SPI intr on dma_xfer\n");
  clear_spi_intr();
  clock_delay_usec(5);
  raise_spi_intr();
}

void dma_xfer_rxonly(void *slv_in, int len)
{
  if (len == 0) {
      LOG_ERR("Error - dma transfers must be > 0\n");
      return;
  }
  if (dma_in_progress()) {

      LOG_ERR("Error - DMA xfer already in progress, aborting.\n");
      return;
  }

  LOG_DBG("Initiating xfer rxonly: %d\n", len);


  uDMAChannelTransferSet(UDMA0_BASE,
			 UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
        		 UDMA_MODE_BASIC,
			 (void *) (SSI0_BASE | SSI_O_DR),
        		 slv_in, len);

  // start configuring the SSI0 RX & TX channels
      uDMAChannelAttributeDisable(
          UDMA0_BASE,
          UDMA_CHAN_SSI0_RX,
          UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY
    	  | UDMA_ATTR_REQMASK);

      uDMAChannelControlSet(
          UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

      uDMAChannelAttributeDisable(
          UDMA0_BASE, UDMA_CHAN_SSI0_TX,
          UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

      uDMAChannelControlSet (
          UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
          UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  // start configuring the SSI0 RX & TX channels
    uDMAChannelAttributeDisable(
        UDMA0_BASE,
        UDMA_CHAN_SSI0_RX,
        UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY
  	  | UDMA_ATTR_REQMASK);

    uDMAChannelControlSet(
        UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
        UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

    uDMAChannelAttributeDisable(
        UDMA0_BASE, UDMA_CHAN_SSI0_TX,
        UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    uDMAChannelControlSet (
        UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
        UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  SSIDMAEnable(SSI0_BASE,SSI_DMA_RX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR);
  ti_lib_int_enable(INT_SSI0_COMB);

  LOG_DBG("Raising SPI intr on dma_xfer_rxonly\n");
  clear_spi_intr();
  clock_delay_usec(5);
  raise_spi_intr();
}

void dma_init( )
{
  LOG_DBG("Initializing uDMA controller\n");
  // configure the PRCM power domains
  PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
  PRCMPeripheralRunEnable(PRCM_PERIPH_UDMA);
  PRCMPeripheralSleepEnable(PRCM_PERIPH_UDMA);
  PRCMLoadSet( );

  // configure dma base registers
  uDMAControlBaseSet(UDMA0_BASE, &pui8ControlTable);

  // enable the DMA controller module
  uDMAEnable(UDMA0_BASE);

  // start configuring the SSI0 RX & TX channels
  uDMAChannelAttributeDisable(
      UDMA0_BASE,
      UDMA_CHAN_SSI0_RX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY
	  | UDMA_ATTR_REQMASK);

  uDMAChannelControlSet(
      UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

  uDMAChannelAttributeDisable(
      UDMA0_BASE, UDMA_CHAN_SSI0_TX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

  uDMAChannelControlSet (
      UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  dma_event = process_alloc_event();
  process_start(&dma_ctrl, NULL);

}


int dma_in_progress( )
{
  int state = (uDMAGetStatus(UDMA0_BASE) >> 4) & 0x0f;
  if ((state == 0x00) || (state == 0x08)) return 0;
  else return 1;
}


