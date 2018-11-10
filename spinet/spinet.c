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
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include <sys/etimer.h>


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



void toggle_interrupt( )
{
  // raise interrupt
    ti_lib_gpio_set_dio(27);
    clock_delay_usec(10);
    ti_lib_gpio_clear_dio(27);
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


volatile int dma_xfer_inprogress = 0;

static void dma_xfer(void *slv_out, void *slv_in, int len)
{
  if (dma_xfer_inprogress == 1) {
      LOG_ERR("Error - DMA xfer already in progress, aborting.\n");
      return;
  }

  char *cptr1 = slv_out;
  char *cptr2 = slv_in;
  printf("Starting DMA XFER %-2.2x%-2.2x%-2.2x%-2.2x  %-2.2x%-2.2x%-2.2x%-2.2x\n",
	 cptr1[0],cptr1[1],cptr1[2],cptr1[3],
	 cptr2[0],cptr2[1],cptr2[2],cptr2[3]);
  dma_xfer_inprogress = 1;

  uDMAChannelTransferSet(UDMA0_BASE,
			 UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
        		 UDMA_MODE_BASIC,
			 (void *) (SSI0_BASE | SSI_O_DR),
        		 slv_in, len);
  uDMAChannelTransferSet (UDMA0_BASE,
			  UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
			  UDMA_MODE_BASIC,
			  slv_out,
			  (void *) (SSI0_BASE | SSI_O_DR),
			  len);

  SSIDMAEnable(SSI0_BASE,SSI_DMA_TX | SSI_DMA_RX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_TX);
  uDMAChannelEnable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);
  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR);
  ti_lib_int_enable(INT_SSI0_COMB);

}

#define DRIVERLIB_NOROM 1
void dma_init( )
{

  uint32_t bob;
  while (ti_lib_rom_ssi_data_get_non_blocking(SSI0_BASE, &bob));

  PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
  PRCMPeripheralRunEnable(PRCM_PERIPH_UDMA);
  PRCMPeripheralSleepEnable(PRCM_PERIPH_UDMA);
  PRCMLoadSet( );
  uDMAControlBaseSet(UDMA0_BASE, &pui8ControlTable);

  uDMAEnable(UDMA0_BASE);

  //ti_lib_ssi_dma_enable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);

  uDMAChannelAttributeDisable(
      UDMA0_BASE,
      UDMA_CHAN_SSI0_RX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY
	  | UDMA_ATTR_REQMASK);

  NOROM_uDMAChannelControlSet(
      UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);

//  uDMAChannelTransferSet(
//      UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT,
//			  UDMA_MODE_BASIC, (void *) (SSI0_BASE | SSI_O_DR),
//			  rxcmd, 8);

  uDMAChannelAttributeDisable(
      UDMA0_BASE, UDMA_CHAN_SSI0_TX,
      UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

//  uDMAChannelAttributeEnable(
//      UDMA0_BASE, UDMA_CHAN_SSI0_TX,
//       UDMA_ATTR_USEBURST);

  NOROM_uDMAChannelControlSet (
      UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
      UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_1);

//  uDMAChannelTransferSet (UDMA0_BASE, UDMA_CHAN_SSI0_TX | UDMA_PRI_SELECT,
//			  UDMA_MODE_BASIC, txres, (void *) (SSI0_BASE | SSI_O_DR),
//			  8);

//  uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_RX);
//  uDMAChannelEnable (UDMA0_BASE, UDMA_CHAN_SSI0_TX);
//  SSIIntEnable(SSI0_BASE, SSI_RXTO | SSI_RXOR );

//  ti_lib_int_enable(INT_SSI0_COMB );
//  ti_lib_int_master_enable( );


}


uint32_t regs[32];
uint32_t read_register(uint32_t reg)
{
  if ((reg >= 0) && (reg < 32))
    return regs[reg];

  else if (reg == 33)
    return 0x01234567;

  if (reg == 34)
    return 0xb00b;

  else
    return 0xffff;
}

void write_register(uint32_t reg, uint32_t val)
{
  if ((reg >= 0) && (reg < 32))
    regs[reg] = val;
}

void set_prefix( )
{
  uip_ip6addr_t addr;
  uiplib_ip6addrconv("fd00::1", &addr);

  NETSTACK_ROUTING.root_set_prefix (&addr, NULL);
  NETSTACK_ROUTING.root_start ();
}

static volatile uint8_t slv_in[4];
static volatile uint8_t slv_out[4];
static int slv_state = 0;

static char uip_buffer[UIP_BUFSIZE];
static uint32_t uip_buff_len = 0;

static char packet_in[UIP_BUFSIZE];
static char packet_out[UIP_BUFSIZE];
static uint32_t rcvd_len = 0;


///* Declare and auto-start this file's process */
//PROCESS(intject, "INtJECT");
//
//PROCESS_THREAD(intject, ev, data)
//{
//  static struct etimer timer;
//
//
//  PROCESS_BEGIN();
//  etimer_set(&timer, 10 * CLOCK_SECOND);
//  while(1) {
//      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
//      printf("Toggling interrupt\n");
//      toggle_interrupt();
//      etimer_reset(&timer);
//  }
//  PROCESS_END();
//}

/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */
PROCESS(spislv_ctrl, "SPI Net");
AUTOSTART_PROCESSES(&spislv_ctrl);
PROCESS_THREAD(spislv_ctrl, ev, data)
{

  static uint32_t curr_reg = 0;

  PROCESS_BEGIN();

//  process_start(&intject, NULL);

  NETSTACK_MAC.off();

  LOG_INFO("\r\n\r\n\r\nContiki-NG SPI NET Starting\n");
  LOG_INFO("Compile date %s %s", __DATE__, __TIME__);
  spi_init ();
  dma_init( );

  /*/\/\/\/ network routing /\/\/\/ */
  set_prefix( );

  NETSTACK_MAC.on();


  /*/\/\/\/ interrupt pin management /\/\/\ */
  ti_lib_gpio_clear_dio(27);
  GPIO_setOutputEnableDio(27, GPIO_OUTPUT_ENABLE);

  //
  memset((void *)slv_out, 0xff, 4);
  memset((void *)slv_in, 0x00, 4);
  slv_state = 0;

  dma_xfer(&slv_out, &slv_in, sizeof(slv_in));

  // enter main loop
  while (1)
    {

      PROCESS_WAIT_EVENT();

//      if (etimer_expired(&int_timer)) {
//	  printf("Toggling interrupt\n");
//	  toggle_interrupt();
//	  etimer_reset(&int_timer);
//	  continue;
//      }

      printf("Rcvd Event - slave state %d\n",slv_state);

      // begin a command
      if (slv_state == 0) {

	  if (slv_in[0] == 0x10) {
	      curr_reg = (slv_in[1] << 8) | slv_in[2];
	      uint32_t val = read_register(curr_reg);

	      printf("Reading register %x = %x\n", (unsigned int)curr_reg, (unsigned int) val);
	      memcpy((void *) &slv_out, (void *) &val, 4);

	      // start next command
	      dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
	      slv_state = 2;
	  }
	  else if (slv_in[0] == 0x11) {
	      curr_reg = (slv_in[1] << 8) | slv_in[2];
	      printf("Writing register %x\n", (unsigned int) curr_reg);
	      memset((void *) slv_out, 0xff, 4);

	      // start next command
	      dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
	      slv_state = 1;
	  }

	  else if (slv_in[0] == 0x20) {
	      printf("%-2.2x %-2.2x %-2.2x %-2.2x",
		     slv_in[0], slv_in[1], slv_in[2], slv_in[3]);

	      uint32_t len = slv_in[1] << 8 | slv_in[2];
	      if (len > UIP_BUFSIZE) {
		  printf("Error - packet length exceeded\n");
		  slv_state = 0;
	      }
	      else {
		  printf("Recv packet from master %d bytes\n", (int) len);
		  rcvd_len = len;
		  dma_xfer(&packet_out, &packet_in, len);
		  slv_state = 3;
	      }
	  }

	  else if (slv_in[0] == 0x21) {
	      printf("%-2.2x %-2.2x %-2.2x %-2.2x",
	      		 slv_in[0], slv_in[1], slv_in[2], slv_in[3]);

	      uint32_t len = slv_in[1] << 8 | slv_in[2];

	      if (len > UIP_BUFSIZE) {
		  printf("Error - packet length exceeded %d\n", (int) len);
		  slv_state = 0;
	      }
	      else {
		  printf("Sending packet to master %d bytes", (int) len);
		  memcpy(&packet_out, uip_buffer, len);
		  rcvd_len = len;
		  dma_xfer(&packet_out, &packet_in, len);
		  slv_state = 4;
	      }
	  }

      }

      // complete a write command
      else if (slv_state == 1) {
	  uint32_t val = (slv_in[0] << 24) |
	      (slv_in[1] << 16) | (slv_in[2] << 8) | (slv_in[3]);
	  printf("Updating register %x = %x\n", (unsigned int) curr_reg, (unsigned int) val);
	  write_register(curr_reg, val);

	  // start next command
	  memset((void *) slv_out, 0xff, 4);
	  dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
	  slv_state = 0;
      }

      // complete a reg-read
      else if (slv_state == 2) {
	  memset((void *) slv_out, 0xff, 4);

	  // start next command
	  dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
	  slv_state = 0;
      }

      // complete a packet transfer
      else if (slv_state == 3) {
	  memcpy(&uip_buf, (void *) &slv_in, rcvd_len);

	  memset((void *) slv_out, 0xff, 4);

	  // start next command
	  dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
	  slv_state = 0;
	  tcpip_input( );
      }

      // complete a packet transfer to master
      else if (slv_state == 4) {
      	  memset((void *) slv_out, 0xff, 4);

      	  // start next command
      	  dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
      	  slv_state = 0;
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
  if (flags == SSI_RXTO) {
      slv_state = 0;
      process_poll (&spislv_ctrl);
  }
  else {
      // get the current DMA for the RX channel
      mode = uDMAChannelModeGet(UDMA0_BASE, UDMA_CHAN_SSI0_RX | UDMA_PRI_SELECT);

      // we've received what we expected, notify parent process
      if(mode == UDMA_MODE_STOP)
      {
	  SSIDMADisable(SSI0_BASE,SSI_DMA_RX);
	  uDMAChannelDisable(UDMA0_BASE, UDMA_CHAN_SSI0_RX);

	  dma_xfer_inprogress = 0;
	  ti_lib_int_disable(INT_SSI0_COMB );

	  // notify slave controller process
	  process_poll (&spislv_ctrl);
      }
      else {
	  SSIDMADisable(SSI0_BASE,SSI_DMA_TX);
	  uDMAChannelDisable(UDMA0_BASE, UDMA_CHAN_SSI0_TX);
      }

      uDMAIntClear(UDMA0_BASE, 0xffffffff);
  }

}


void rpl_init(void)
{

}

int rpl_output(void)
{
  printf("Received packet from RF %d : \n", uip_len);
  for (int i = 0; i < uip_len; i++) {
      if ((i > 0) && ((i % 16) == 0)) printf("\n");
      printf(" %-2.2x ", uip_buf[i]);

  }
  printf("\n");


  regs[1] = uip_len;

  uip_buff_len = uip_len;
  memcpy(uip_buffer, &uip_buf, uip_len);

  toggle_interrupt();

  return 0;
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface spi_rpl_interface = {
  rpl_init, rpl_output
};
