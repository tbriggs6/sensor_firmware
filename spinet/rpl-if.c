/*
 * rpl-status.c
 *
 *  Created on: Nov 15, 2018
 *      Author: contiki
 */

#include "sys/log.h"
#include "net/routing/routing.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "rpl-if.h"

#include "spinet.h"

#define LOG_MODULE "RPLSTAT"
#define LOG_LEVEL LOG_LEVEL_INFO

void rplstat_set_prefix( )
{
  uip_ip6addr_t addr;
  uiplib_ip6addrconv("fd00::", &addr);

  NETSTACK_ROUTING.root_set_prefix (&addr, NULL);
  NETSTACK_ROUTING.root_start ();
}

static struct etimer idle_timer;

/*---------------------------------------------------------------------------*/
/* Declare and auto-start this file's process */


PROCESS(rplstat_ctrl, "rpl statusr");
PROCESS_THREAD(rplstat_ctrl, ev, data)
{
  PROCESS_BEGIN();

  LOG_DBG("RPL Idle status process starting\n");
  etimer_set(&idle_timer, RPLSTAT_AUTOREPAIR);

  // enter main loop
  while (1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&idle_timer));

      if (etimer_expired(&idle_timer)) {
	  LOG_INFO("Idle time out, repairing dags\n");
	  NETSTACK_ROUTING.global_repair("Req");
	  etimer_restart(&idle_timer);

	  continue;
      }

  }

  PROCESS_END( );

}


void rplstat_init( )
{
  rplstat_set_prefix( );
  NETSTACK_MAC.on();

  NETSTACK_ROUTING.global_repair("Req");

  process_start(&rplstat_ctrl, NULL);
}

void rplstat_reset_timer( void )
{
  etimer_restart(&idle_timer);
}


/* -------------------------- */
/* handle the RPL callbacks */
/* -------------------------- */
#define ETHER2_HEADER_SIZE (14)

static uint32_t frame_len = 0;
process_event_t rpl_event;

///* ---------------------------------------------------*/
///* Declare and auto-start this file's process */
//PROCESS(rpl_postr, "rpl poster");
//PROCESS_THREAD(rpl_postr, ev, data)
//{
//  PROCESS_BEGIN();
//  LOG_DBG("RPL poster process starting\n");
//
//  rpl_event = process_alloc_event();
//
//  // enter main loop
//  while (1) {
//      PROCESS_WAIT_EVENT( );
//
//      // toggle the interrupt
//
//  }
//
//  PROCESS_END( );
//
//}
//



void rpl_init(void)
{
  LOG_DBG("RPL Call back poster started\n");
//  rpl_event = process_alloc_event();
//  process_start(&rpl_postr, NULL);

}



int rpl_output(void)
{

  if (uip_len == 0) {
      LOG_ERR("Err - uip_len = 0, no frame!\n");
      return 0;
  }

  if (frame_len > 0) {
      LOG_ERR("Err - there is already a frame in the buffer, clobbering\n");
  }

  LOG_DBG("Toggling RX intr\n");
  frame_len = uip_len+ETHER2_HEADER_SIZE;
  write_register(1,frame_len);

  toggle_rx_interrupt();
  //process_poll (&rpl_postr);
  return 0;
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface spi_rpl_interface = {
  rpl_init, rpl_output
};

unsigned int rplif_copy_buffer(void *dest, int max_len)
{
  int total_bytes = uip_len + ETHER2_HEADER_SIZE;

  if (total_bytes > max_len) {
      LOG_WARN("frame buffer (%d) is larger than maximum length (%d), discarding\n",
	       (int) frame_len, (int) max_len);
      frame_len = 0;
      return 0;
  }

  uint8_t *buff = (uint8_t *) dest;
  const char dest_mac[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
  for (int i = 0; i < 6; i++) {
      buff[i] = dest_mac[i];
  }
  for (int i = 0; i < 6; i++) {
      buff[i+6] = uip_lladdr.addr[i];
  }
  buff[12] = 0x08;
  buff[13] = 0x00;

  memcpy(buff+ETHER2_HEADER_SIZE, &uip_buf, uip_len);

  return total_bytes;
}
