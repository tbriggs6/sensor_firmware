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


static spi_device_t spidev = {
  .spi_controller = SPI_CONTROLLER,
  .pin_spi_sck = SPI_PIN_SCK,
  .pin_spi_miso = SPI_PIN_MISO,
  .pin_spi_mosi = SPI_PIN_MOSI,
  .pin_spi_cs = SPI_PIN_CS,
  .spi_bit_rate = 1000000,
  .spi_pha = 1,
  .spi_pol = 0,
  .spi_slave = 1
};

#define CC13XX_SSI_INTERRUPT_ALL (SSI_TXFF | SSI_RXFF | SSI_RXTO | SSI_RXOR )
#define CC13XX_SSI_INTERRUPT_RXONLY (SSI_RXFF | SSI_RXTO | SSI_RXOR )
#define CC13XX_SSI_INTERRUPT_TXONLY (SSI_TXFF )

/*---------------------------------------------------------------------------*/
#define MAX_CIRC_BYTES 32
static uint8_t circ_tx_store[MAX_CIRC_BYTES];
static uint8_t circ_rx_store[MAX_CIRC_BYTES];
circbuff_t txqueue, rxqueue;


void spi_init( )
{
  bool flag;

  flag = ti_lib_int_master_disable();

  if (spi_acquire(&spidev) != SPI_DEV_STATUS_OK) {
      LOG_ERR("Could not aquire SPI device.");
  }

  //power_and_clock( );

  ti_lib_ssi_int_enable(SSI0_BASE, CC13XX_SSI_INTERRUPT_RXONLY);
  ti_lib_int_enable(INT_SSI0_COMB);


  if(!flag)
      ti_lib_int_master_enable();
}


static void spinet_fb_init(void)
{

}

static int spinet_fb_output(void)
{
  return 0;
}


const struct uip_fallback_interface spinet_if = {
  spinet_fb_init,spinet_fb_output
};


typedef enum {
    SPI_IDLE,
    RDREG1, RDREG2, RDEND, RDSPIN,
    WRREG1, WRREG2, WREND, WRSPIN
} states_t;


void spi_send_byte(const char ch)
{
  uint32_t flags = ti_lib_ssi_status(SSI0_BASE);
  if (flags & SSI_TX_NOT_FULL) {
      ti_lib_rom_ssi_data_put_non_blocking(SSI0_BASE, ch);
  }
  else
    circbuff_addch(&txqueue, ch);
}

/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */
PROCESS(contiki_ng_br, "SPI Net");
AUTOSTART_PROCESSES(&contiki_ng_br);


PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();

  circbuff_init(&txqueue, circ_tx_store, MAX_CIRC_BYTES);
  circbuff_init(&rxqueue, circ_rx_store, MAX_CIRC_BYTES);

  LOG_INFO("\r\n\r\n\r\nContiki-NG SPI NET Starting\n");
  spi_init( );


  printf("Starting network routing\r\n");
  NETSTACK_ROUTING.root_set_prefix(NULL, NULL);
  NETSTACK_ROUTING.root_start();



  static int len = 0;
  static int regnum = 0;
  static int fakereg = 0;
  static char send_ch = 0;

  static states_t state = SPI_IDLE;
  // enter main loop
  while(1) {


      if (circbuff_isempty(&rxqueue)) {
	  // wait for an interrupt
	  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
	  continue;
      }

      // drain the rxqueue and run the slave state machine
      while(circbuff_hasdata(&rxqueue)) {

	  char ch = circbuff_getch(&rxqueue);
	  spi_send_byte(send_ch++);

	  switch(state) {
	    case SPI_IDLE:
	      if (ch == 0x10) {	// read register
		state = RDREG1;
	      }
	      if (ch == 0x11) {
		state = WRREG1;
	      }

	      break;

	    case RDREG1:
	      regnum = ch << 8;
	      state = RDREG2;
	      break;

	    case RDREG2:
	      regnum = regnum | ch;
	      state = RDEND;
	      break;

	    case RDEND:
	      printf("REG RD %x %x\n", regnum, fakereg);

	      // get the register value

	      //spi_send_byte((fakereg >> 24) & 0xff );
	      //spi_send_byte((fakereg >> 16) & 0xff );
	      //spi_send_byte((fakereg >> 8) & 0xff );
	      //spi_send_byte((fakereg >> 0) & 0xff );

	      len = 4;
	      state = RDSPIN;


//	      spi_send_byte(send_ch++);
//	      spi_send_byte(send_ch++);
//	      spi_send_byte(send_ch++);
//
	      break;

	    case RDSPIN:

	      if (--len == 0) state = IDLE;
	      break;

	    case WRREG1:
	      regnum = ch << 8;
	      state = WRREG2;
	      break;

	    case WRREG2:
	      regnum = regnum | ch;
	      len = 4;
	      state = WREND;
	      break;

	    case WREND:
	      state = WRSPIN;
	      break;

	    case WRSPIN:
	      if (--len == 0) state = IDLE;
	      if (len == 3) fakereg = ch << 24;
	      else if (len == 2) fakereg |= ch << 16;
	      else if (len == 1) fakereg |= ch << 8;
	      else if (len == 0) {
		  fakereg |= ch << 0;
		  printf("REG WR %x %x\n", regnum, fakereg);
	      }


	      break;


	  }


	  if (circbuff_hasdata(&txqueue)) {
	      ti_lib_ssi_int_enable(SSI0_BASE, CC13XX_SSI_INTERRUPT_TXONLY);
	  }

      }
  }
  PROCESS_END();
}



/**
 * Interrupt handler to manage the FIFO... there are two directions:
 * - MOSI (slave recv) - master sends bytes, hw FIFO stores them, triggers
 *   	an interrupt, the handler retrieves bytes from the FIFO, stores into
 *   	a circular queue
 * - MISO (slave tx) - the hw FIFO is used as a shift register to shift data
 *      out as master runs the clock.  As the hw FIFO drains, it triggers an
 *      interrupt, and this handler must read the circ queue and refill the
 *      FIFO.
 */
void SSI0IntHandler(void)
{
  uint32_t flags;
  uint32_t recvd_word;

  char ch;

  flags = ti_lib_ssi_int_status(SSI0_BASE, true);

  ti_lib_ssi_int_clear(SSI0_BASE, CC13XX_SSI_INTERRUPT_ALL);

  if (flags & CC13XX_SSI_INTERRUPT_RXONLY) {
      while (!circbuff_isfull(&rxqueue) && (ti_lib_ssi_status(SSI0_BASE) & SSI_RX_NOT_EMPTY)) {
	ti_lib_ssi_data_get_non_blocking(SSI0_BASE, &recvd_word);
	circbuff_addch(&rxqueue, (char) recvd_word);
      }
  }

  // we can send more data...
  if (flags & CC13XX_SSI_INTERRUPT_TXONLY) {
      while ((!circbuff_isempty(&txqueue)) && (ti_lib_ssi_status(SSI0_BASE) & SSI_TX_NOT_FULL)) {
	ch = circbuff_getch(&txqueue);
	ti_lib_ssi_data_put_non_blocking(SSI0_BASE, ch);
      }
      // either the FIFO is full or we've drained it...
      ti_lib_ssi_int_disable(SSI0_BASE, CC13XX_SSI_INTERRUPT_TXONLY);
  }

  // alert the polling process that the queues have changed.
  if (! (circbuff_isempty(&txqueue) && circbuff_isempty(&rxqueue)))
    process_poll(&contiki_ng_br);
}
