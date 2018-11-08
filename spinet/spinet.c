/*
 * Copyright (c) 201, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"

/* Log configuration */
#include "sys/log.h"
#include "dev/leds.h"
#include "dev/spi.h"

#include "ti-lib.h"
#include "driverlib/prcm.h"
#include "driverlib/udma.h"

#include <stdint.h>
#include "circularbuff.h"
#include "netstack.h"
#include "net/routing/routing.h"

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SPI_CONTROLLER    SPI_CONTROLLER_SPI0

#define SPI_PIN_SCK       30
#define SPI_PIN_MOSI      29
#define SPI_PIN_MISO      28
#define SPI_PIN_CS        1

static spi_device_t spidev =
  { .spi_controller = SPI_CONTROLLER, .pin_spi_sck = SPI_PIN_SCK,
      .pin_spi_miso = SPI_PIN_MISO, .pin_spi_mosi = SPI_PIN_MOSI, .pin_spi_cs =
	  SPI_PIN_CS, .spi_bit_rate = 1000000, .spi_pha = 1, .spi_pol = 0,
      .spi_slave = 1 };

void
spi_init ()
{
  if (spi_acquire (&spidev) != SPI_DEV_STATUS_OK)
    {
      LOG_ERR("Could not aquire SPI device.");
    }

}

static void
spinet_fb_init (void)
{

}

static int
spinet_fb_output (void)
{
  return 0;
}

const struct uip_fallback_interface spinet_if =
  { spinet_fb_init, spinet_fb_output };



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


void dma_callback_fxn (void);

char rxcmd[16];
char txres[16];


#define DRIVERLIB_NOROM 1
void dma_init( )
{

  memset(txres, 0xff, 16);
  memset(rxcmd, 0xff, 16);

  uint32_t bob;
  while (ti_lib_rom_ssi_data_get_non_blocking(SSI0_BASE, &bob));

  PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
  PRCMPeripheralRunEnable(PRCM_PERIPH_UDMA);
  PRCMPeripheralSleepEnable(PRCM_PERIPH_UDMA);
  PRCMLoadSet( );
  uDMAControlBaseSet(UDMA0_BASE, &pui8ControlTable);

  uDMAEnable(UDMA0_BASE);
  //uDMADisable(UDMA0_BASE);
  // two buffers to transfer the command & result

  ti_lib_ssi_dma_enable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

  uDMAChannelAttributeDisable(
      UDMA0_BASE,
      UDMA_CHAN_SSI0_RX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY
	  | UDMA_ATTR_REQMASK);

  NOROM_uDMAChannelControlSet(
      UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

  uDMAChannelTransferSet(
      UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
			  UDMA_MODE_BASIC, (void *) (SSI0_BASE | SSI_O_DR),
			  rxcmd, 4);

  uDMAChannelAttributeDisable(
      UDMA0_BASE, UDMA_CHAN_SSI0_TX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

//  uDMAChannelAttributeEnable(
//      UDMA0_BASE, UDMA_CHAN_SSI0_TX,
//       UDMA_ATTR_USEBURST);

  NOROM_uDMAChannelControlSet (
      UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

  uDMAChannelTransferSet (UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
			  UDMA_MODE_BASIC, txres, (void *) (SSI0_BASE | SSI_O_DR),
			  4);

  uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_TX);
  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR );

  ti_lib_int_enable(INT_SSI0_COMB );
  ti_lib_int_master_enable( );


}


/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */
PROCESS(contiki_ng_br, "SPI Net");
AUTOSTART_PROCESSES(&contiki_ng_br);

PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("\r\n\r\n\r\nContiki-NG SPI NET Starting\n");
  spi_init ();

  printf ("Starting network routing\r\n");
  NETSTACK_ROUTING.root_set_prefix (NULL, NULL);
  NETSTACK_ROUTING.root_start ();

  dma_init( );

  // enter main loop
  while (1)
    {


//      if (circbuff_isempty(&rxqueue)) {
//	  // wait for an interrupt
      PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
      printf("Received DMA interrupt callback\n");

    }
PROCESS_END();
}

//void uDMAIntHandler(void)
void SSI0IntHandler(void)
{
  //uDMAIntClear (UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_CHAN_SSI0_RX);

  uint32_t flags = SSIIntStatus(SSI0_BASE, 1);
  SSIIntClear(SSI0_BASE, flags);


  uint32_t ui32Mode = uDMAChannelModeGet(UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT);

  if(ui32Mode == UDMA_MODE_STOP)
  {
      uDMAChannelTransferSet(
            UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
      			  UDMA_MODE_BASIC, (void *) (SSI0_BASE | SSI_O_DR),
      			  rxcmd, 4);

      uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  }


  SSIDMADisable(SSI0_BASE,SSI_DMA_TX);

  uDMAIntClear(UDMA0_BASE, 0xffffffff);


  process_poll (&contiki_ng_br);
}

