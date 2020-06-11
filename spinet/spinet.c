#include "contiki.h"

#include "sys/log.h"
#include "dev/leds.h"
#include "dev/spi.h"

#include "ti-lib.h"

#include <stdint.h>
#include "netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include <sys/etimer.h>
#include <sys/critical.h>
#include "rpl-if.h"
#include "spi-dma.h"

/* For RPL-specific commands */
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#endif


#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SPI_CONTROLLER    SPI_CONTROLLER_SPI0

#define SPI_PIN_SCK       30
#define SPI_PIN_MOSI      29
#define SPI_PIN_MISO      28
#define SPI_PIN_CS        1


void spi_init ()
{
  spi_device_t spidev = {
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


  if (spi_acquire (&spidev) != SPI_DEV_STATUS_OK)
    {
      LOG_ERR("Could not aquire SPI device.");
    }
  else {
      LOG_DBG("spi init OK");
  }
}

void spi_drain_rx( )
{
  uint32_t bob;
  uint32_t count = 0;
  while (ti_lib_ssi_data_get_non_blocking(SSI0_BASE, &bob)) count++;
  LOG_DBG("SPI drained %d bytes", (int) count);
}

void toggle_rx_interrupt( )
{
    // raise interrupt
    ti_lib_gpio_set_dio(27);
    clock_delay_usec(5);
    ti_lib_gpio_clear_dio(27);
}


void raise_spi_intr( )
{
  ti_lib_gpio_set_dio(26);
}

void clear_spi_intr( )
{
  ti_lib_gpio_clear_dio(26);
}



#define NUM_REGS 2
uint32_t regs[NUM_REGS];

uint32_t read_register(uint32_t reg)
{
  if ((reg >= 0) && (reg < NUM_REGS))
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
  if ((reg >= 0) && (reg < NUM_REGS))
    regs[reg] = val;
  else if (reg == 40) {
      NETSTACK_ROUTING.global_repair("Req");
  }
  else if (reg == 41) {
      NETSTACK_ROUTING.local_repair("Req");
  }
  else if (reg == 42) {
      rpl_refresh_routes("Shell");
  }

}


typedef enum {
  STATE_IDLE,
  STATE_FIN_RDREG,
  STATE_FIN_WRREG,
  STATE_FIN_RCV,
  STATE_FIN_XMT
} slv_state_t;


void show_buffer(const char *buff, int len)
{
  int i,k;

  char obuff[200];

  LOG_DBG("----------------------\n");
  obuff[0] = 0;
  k = 0;
  for(i = 0; i < len; i++) {
      if ((i > 0) && ((i % 8 == 0))) {
	  sprintf(obuff+k, "\n");
	  LOG_DBG(obuff);

	  k = 0;
	  obuff[0] = 0;
      }
      k += sprintf(obuff+k, " %-2.2x", buff[i]);
  }
  if (strlen(buff) > 0) {
      LOG_DBG(obuff);
  }
}

/**
 * Process a request from the SPI master
 * @return the next state that the slave state machine should enter
 */
slv_state_t process_command(const char *spi_in, char *spi_out, int max_len,
			    int *xfer_len, int *curr_reg)
{
  int spi_len = 0;
  slv_state_t next_state = STATE_IDLE;
  if (spi_in[3] != 0xff) {
      LOG_ERR("Not a valid command terminator, skipping\n");
      show_buffer(spi_in, 4);
  }
   if (spi_in[0] == 0x10) {
       *curr_reg = (spi_in[1] << 8) | spi_in[2];		// extract the reg number

       uint32_t val = read_register(*curr_reg);		// look up the reg value

       memcpy((void *) spi_out, (void *) &val, 4);	// store the value
       *xfer_len = 4;

       LOG_DBG("Rdreg, reg %d = %x\n", (int) *curr_reg, (int) val);
       LOG_DBG("    spi_out: %x %x %x %x\n", spi_out[0], spi_out[1], spi_out[2], spi_out[3]);


       next_state = STATE_FIN_RDREG;			// set the next state
   }

    else if (spi_in[0] == 0x11) {
	*curr_reg = (spi_in[1] << 8) | spi_in[2];

	memset((void *) spi_out, 0x10, 4);
	*xfer_len = 4;
	LOG_DBG("Writing register %x, starting dma xfer for state 1\n", (int) *curr_reg);

	// start next command
	next_state = STATE_FIN_WRREG;
    }

   // start receiving a packet from the raspberry pi
   else if (spi_in[0] == 0x20) {

	spi_len = spi_in[1] << 8 | spi_in[2];

	if (spi_len > max_len) {
	    LOG_ERR("Error - packet length exceeded %-4.4x / %d > %d\n", spi_len, max_len, UIP_BUFSIZE);
	    next_state = STATE_IDLE;
	}
	else if (xfer_len == NULL) {
	    LOG_ERR("ERROR - xfer_len isn't valid??: %p\n", xfer_len);
	    next_state = STATE_IDLE;
	}
	else {
	    *xfer_len = spi_len;
	    next_state = STATE_FIN_RCV;
	}
    }

   // write a packet to the pi
   else if (spi_in[0] == 0x21) {
        spi_len = spi_in[1] << 8 | spi_in[2];

//        LOG_DBG("Sending rcvd buffer to Pi %d bytes, start dma\n", (int) spi_len);
        if (regs[1] == 0) {
            LOG_ERR("Frame buffer is empty but master requests a frame...\n");
            next_state = STATE_IDLE;
        }
        else {
            int rcvd_len = rplif_copy_buffer(spi_out, max_len);


	  if (rcvd_len > spi_len) {
	      LOG_ERR("Error - rcvd packet length exceeds remote buffer %d > %d\n", (int) rcvd_len, (int) spi_len);
	      rcvd_len = spi_len;
	      next_state = STATE_IDLE;
	  }

	  else if (rcvd_len == 0) {
	      LOG_ERR("Error - rcvd_len is 0, returning to idle\n");
	      next_state = STATE_IDLE;
	  }

	  else {
  //	    LOG_DBG("Transmitting packet to Pi %d\n", spi_len);
	      *xfer_len = rcvd_len;
	      next_state = STATE_FIN_XMT;
	  }
        }
    }

    else {
	//LOG_ERR("Unknown command prefix: %x, returning to idle\n", spi_in[0]);
	next_state = STATE_IDLE;
    }


   return next_state;
}


slv_state_t process_writereg(const char *spi_in, int curr_reg)
{
  uint32_t value = (spi_in[0] << 24) | (spi_in[1] << 16) |
      (spi_in[2] << 8) | spi_in[3];

//  LOG_DBG("Writing register %X = %X\n", curr_reg, (unsigned int) value);
  write_register(curr_reg, value);
  return STATE_IDLE;
}


slv_state_t process_readreg( void )
{

  return STATE_IDLE;
}


slv_state_t process_finrcv(const char *spi_in, int xfer_len)
{
  if (!((xfer_len > 0) && (xfer_len < UIP_BUFSIZE))) {
      LOG_ERR("Error - transfer size out of bounds 0 > %d > %d\n", (int) xfer_len, (int) UIP_BUFSIZE);
  }
  else {


      memcpy(uip_buf, spi_in, xfer_len);
      uip_len = xfer_len;

      printf("tcpip input %d : %x %x %x %x\n", xfer_len, uip_buf[0],uip_buf[1],uip_buf[2],uip_buf[3]);
      tcpip_input();
  }
  return STATE_IDLE;
}


slv_state_t process_finxmt( )
{
  rplstat_reset_timer();

  return STATE_IDLE;
}


/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */
PROCESS(spislv_ctrl, "SPI Net");
AUTOSTART_PROCESSES(&spislv_ctrl);
PROCESS_THREAD(spislv_ctrl, ev, data)
{
  PROCESS_BEGIN();

  static uint32_t curr_reg = 0;
  static volatile slv_state_t slv_state = STATE_IDLE;
  static slv_state_t next_state = STATE_IDLE;

  static char spi_in[UIP_BUFSIZE];
  static char spi_out[UIP_BUFSIZE];
  static uint32_t xfer_len = 0;


  NETSTACK_MAC.off();

  LOG_INFO("Contiki-NG SPI NET Starting\n");
  LOG_INFO("Compile date %s %s\n", __DATE__, __TIME__);

  spi_init ();

  spi_drain_rx();
  dma_init( );
  rplstat_init();

  /*/\/\/\/ network routing /\/\/\/ */



  /*/\/\/\/ interrupt pin management /\/\/\ */
  ti_lib_gpio_clear_dio(27);
  GPIO_setOutputEnableDio(27, GPIO_OUTPUT_ENABLE);

  ti_lib_gpio_clear_dio(26);
  GPIO_setOutputEnableDio(26, GPIO_OUTPUT_ENABLE);

  dma_xfer_rxonly(&spi_in, 4);

  raise_spi_intr();

  // enter main loop
  while (1)
    {

      PROCESS_WAIT_EVENT_UNTIL(ev == dma_event );
      unsigned int intr_status = get_event_flags();
      LOG_DBG("Received dma_event %d", intr_status);

      if (intr_status & (INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR))
      {
	  clear_event_flag((INTR_OVFLOW_RXFIN | INTR_OVFLOW_TXFIN | INTR_OVFLOW_RXERR));
      }

      if (intr_status & INTR_RXERR)
      {
	  clear_event_flag(INTR_RXERR);
      }

      if (intr_status & INTR_TXFIN) {
	  clear_event_flag(INTR_TXFIN);
      }

      // every path through here *MUST* start a new DMA transfer
      if (intr_status & INTR_RXFIN) {
	  LOG_DBG("Processing INTR_RXFIN\n");
	  clear_event_flag(INTR_RXFIN);


	  switch(slv_state) {
	    case STATE_IDLE:
	      next_state = process_command(spi_in, spi_out, sizeof(spi_out),
					   (int *) &xfer_len, (int *) &curr_reg);
	      break;

	    case STATE_FIN_WRREG:
	      next_state = process_writereg(spi_in, curr_reg);
	      break;
	    case STATE_FIN_RDREG:
	      next_state = process_readreg( );
	      break;

	    case STATE_FIN_RCV:
	      next_state = process_finrcv(spi_in, xfer_len);
	      break;

	    case STATE_FIN_XMT:
	      next_state = process_finxmt( );
	      break;
	  }

	  LOG_DBG("State: %d Next state: %d xfer len: %d\n", (int) slv_state, (int) next_state, (int) xfer_len);
	  switch(next_state) {
	    case STATE_IDLE:
	      dma_xfer_rxonly(spi_in,  4);
	      break;

	    case STATE_FIN_WRREG:
	      dma_xfer_rxonly(spi_in, 4);
	      break;

	    case STATE_FIN_RDREG:
	      dma_xfer(&spi_out, spi_in, 4);
	      break;

	    case STATE_FIN_RCV:
	      dma_xfer_rxonly(spi_in, xfer_len);
	      break;

	    case STATE_FIN_XMT:
	      dma_xfer(spi_out, spi_in, xfer_len);
	      break;

	    default:
	      LOG_ERR("Reached the default case!\n");
	      break;
	  }

	  slv_state = next_state;
      }
//		// complete a packet transfer
//	else if (slv_state == 3) {
//
//	    memset(&uip_buf, 0, sizeof(uip_buf));
//	    memcpy(&uip_buf, (void *) &packet_in, rcvd_len);
//	    uip_len = rcvd_len;
//
//	    memset((void *) slv_out, 0xff, 4);
//
//	    // start next command
//	    dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
//	    slv_state = 0;
//
//	    tcpip_input( );
//	}
//
//	// complete a packet transfer to master
//	else if (slv_state == 4) {
//	    memset((void *) slv_out, 0xff, 4);
//
//	    // start next command
//	    dma_xfer(&slv_out, &slv_in, sizeof(slv_in));
//	    slv_state = 0;
//	    rplstat_reset_timer();
//	}
//      }

//
//	   switch(next_state) {
//	     // send nothing, just read next command
//	     case STATE_IDLE:
//	       dma_xfer_rxonly(&slv_out, 4);
//	       break;
//
//	       // send the contents of the register
//	       case STATE_FIN_RDREG:
//		 dma_xfer(&slv_out, &slv_in, 4);
//		 break;
//
//		 // read the next 4 bytes into the registr
//	       case STATE_FIN_WRREG:
//		 dma_xfer_rxonly(&slv_out, 4);
//		 break;
//
//		 // receive the pkt from the pi
//	       case STATE_FIN_RCV:
//		 dma_xfer_rxonly(&pkt_in, 4);
//		 break;
//
//		 // send the radio packet to the pi
//	       case STATE_FIN_XMT:
//		 dma_xfer(&pkt_out, &pkt_in, spi_len);
//		 break;
//
//
//	       default:
//		 dma_xfer_rxonly(&slv_out, 4);
//		 break;
//	   }



//      critical_exit(status);

    }
PROCESS_END();
}

