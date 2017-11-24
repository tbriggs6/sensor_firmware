/*
 * sender.c
 *
 *  Created on: Jan 2, 2017
 *      Author: tbriggs
 */

#include <contiki.h>
#include <core/sys/pt.h>
#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>
#include <simple-udp.h>
#include <sys/node-id.h>
#include <random.h>
#include <dev/leds.h>
#include <net/packetbuf.h>
#include <sys/etimer.h>

#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/rpl/rpl-ns.h"
#include "net/ip/uip-udp-packet.h"
#include "net/netstack.h"

// one of the net debug functions has this in it...
#ifdef PRINTF
#undef PRINTF
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_SERVER_PORT (1234)
#define UDP_CLIENT_PORT (1235)
#define BCAST_PORT (8053)

#define MAXLEN 32
typedef struct
{
  uint16_t seq;
  char data[MAXLEN];
} message_t;

typedef struct
{
  uint16_t ackseq;
  uint16_t status;
} ack_t;


process_event_t ack_event;
process_event_t send_fin_event;

/* \/\/\/\/\/\/\/\/\/| SERVER PROCESS |/\/\/\/\/\/\/\/\/\ */
static struct uip_udp_conn *server_conn;

void
become_root ()
{
  uip_ipaddr_t ipaddr;

  if (server_conn == NULL)
    {
      printf ("Error - not a server process\n");
      return;
    }

  uip_ds6_addr_add (&ipaddr, 0, ADDR_MANUAL);
  uip_ds6_addr_t *root_if = uip_ds6_addr_lookup (&ipaddr);

  if (root_if != NULL)
    {
      rpl_dag_t *dag;
      dag = rpl_set_root (RPL_DEFAULT_INSTANCE, (uip_ip6addr_t *) &ipaddr);
      uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
      rpl_set_prefix (dag, &ipaddr, 64);
      PRINTF("created a new RPL dag\n");
    }
  else
    {
      PRINTF("RPL already has a DAG\n");
    }
}

void
repair_root ()
{
  if (server_conn == NULL)
    {
      printf ("Error - not a server process\n");
      return;
    }

  PRINTF("RPL Repair started\n");
  rpl_repair_root (RPL_DEFAULT_INSTANCE);
}

int
isserver ()
{
  return (server_conn != NULL);
}

static void
set_global_address (void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  printf ("********** set global address ************\n");
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid (&ipaddr, &uip_lladdr);
  uip_ds6_addr_add (&ipaddr, 0, ADDR_AUTOCONF);

  printf ("IPv6 addresses: ");
  for (i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
      state = uip_ds6_if.addr_list[i].state;
      if (uip_ds6_if.addr_list[i].isused
	  && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
	{
	  uip_debug_ipaddr_print (&uip_ds6_if.addr_list[i].ipaddr);
	  printf ("\n");
	}
    }

  printf ("********** set global address ************\n");
}

static void
server_receive_data (uip_ipaddr_t *from, message_t *recvd, int16_t *rssi)
{
  printf ("Received data\n");
  if (uip_newdata())
    {
      memset (recvd, 0, sizeof(message_t));
      memcpy (recvd, uip_appdata,
	      (uip_datalen() < sizeof(message_t)) ?
	      uip_datalen() : sizeof(message_t));

      memcpy (from, &(UIP_IP_BUF->srcipaddr), sizeof(uip_ipaddr_t));
      *rssi = (int16_t) packetbuf_attr (PACKETBUF_ATTR_RSSI);

    }
}

static void
server_ack (uip_ipaddr_t *from, uint16_t seq, uint16_t result)
{
  static ack_t ack;
  ack.ackseq = seq;
  ack.status = result;

  printf("Sending ACK seq: %u status: %u\n", seq, result);
  uip_udp_packet_sendto (server_conn, &ack, sizeof(ack_t), from,
			 UIP_HTONS(UDP_CLIENT_PORT));
}

static void
server_handler ()
{
  static message_t recvd;
  static int16_t rssi;
  static uip_ipaddr_t from;

  server_receive_data (&from, &recvd, &rssi);
  server_ack (&from, recvd.seq, 0);

  //TODO this will cause a bug after 65k messages
//  if (recvd.seq > last_seq)
//    {
      printf ("****** MESSAGE *******\n");
      printf ("SEQ: %u DATA: %s\n", recvd.seq, recvd.data);
      printf ("FROM: ");
      uip_debug_ipaddr_print (&from);
      printf ("\n");
      printf ("RSSI: %d\n", rssi);
      printf ("**********************\n");
//    }

}

PROCESS(server_process, "Server Process");
PROCESS_THREAD(server_process, ev, data)
{

  PROCESS_BEGIN( )
  ;

//  set_global_address();
//  become_root( );

  server_conn = udp_new (NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  if (server_conn == NULL)
    {
      PRINTF("No UDP connection available, exiting the process!\n");
      PROCESS_EXIT()
      ;
    }

  //bring the connection to server's local port
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

  while (1)
    {
      PROCESS_YIELD()
      ;
      if (ev == tcpip_event)
	{
	  server_handler ();
	}
    }

PROCESS_END( );

}

void
start_server ()
{
set_global_address ();
process_start (&server_process, NULL);
}

//\/\/\/\/\/\/| CLIENT PROCESS |/\/\/\/\/\/\/\/\/

static struct uip_udp_conn *client_conn;

volatile static uint16_t last_ack = 0;

PROCESS(sender_process, "Packet Sender Process");
PROCESS(client_process, "Client Process");


int isclient ()
{
  return (client_conn != NULL);
}

static void client_receive_data (uip_ipaddr_t *from, ack_t *ack_recvd, int16_t *rssi)
{
if (uip_newdata())
  {
    memset (ack_recvd, 0, sizeof(ack_t));
    memcpy (ack_recvd, uip_appdata,
	    (uip_datalen() < sizeof(ack_t)) ?
	    uip_datalen() :  sizeof(ack_t));

    memcpy (from, &(UIP_IP_BUF->srcipaddr), sizeof(uip_ipaddr_t));
    *rssi = (int16_t) packetbuf_attr (PACKETBUF_ATTR_RSSI);

  }
}

static void client_handler ()
{
uip_ipaddr_t from;
ack_t ack_recvd;
int16_t rssi = 0;

  client_receive_data (&from, &ack_recvd, &rssi);

  printf ("*************************************\n");
  printf ("Received ACK: %u/%u, RSSI: %d\n", ack_recvd.ackseq, ack_recvd.status,
	  rssi);
  printf ("*************************************\n");
  last_ack = ack_recvd.ackseq;

  process_post(&sender_process, ack_event, (int *) &last_ack);
}


PROCESS_THREAD(client_process, ev, data)
{

  PROCESS_BEGIN();

  ack_event = process_alloc_event();
  send_fin_event = process_alloc_event();

  /* new connection with remote host */
  client_conn = udp_new (NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  if (client_conn == NULL)
    {
      PRINTF("No UDP connection available, exiting the process!\n");
      PROCESS_EXIT()
      ;
    }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n", UIP_HTONS(client_conn->lport),
	 UIP_HTONS(client_conn->rport));

  while (1)
    {
      PROCESS_YIELD();

      if (ev == tcpip_event)
	{
	  // data has been received back from the server -- it should be an ACK
	  client_handler ();
	}
    }

  PROCESS_END();
}

typedef struct
{
  uip_ipaddr_t *server;
  message_t *message;
} sender_process_t;


PROCESS_THREAD(sender_process, ev, data)
{
static struct etimer et;
static sender_process_t *proc = NULL;
static int count = 0;
static int status = 0;

PROCESS_BEGIN( );


etimer_set (&et, CLOCK_SECOND * 5);

while (1)
{
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);


  proc = (sender_process_t *) data;
  count = 0;


   // send first packet
   uip_udp_packet_sendto (client_conn, proc->message, sizeof(message_t),
		     proc->server, UIP_HTONS(UDP_SERVER_PORT));
   etimer_reset(&et);

   // handle retries
   while (last_ack != proc->message->seq)
    {
       PROCESS_WAIT_EVENT( );
       if (etimer_expired(&et)) {
	   if (count++ > 20) {
	       printf ("Error - giving up - did not receive the ACK\n");
	       status = 0;
	       break;
	   }
	   else {
	       printf("\n\nTimed out... retrying attempt #%d\n", count);
	       uip_udp_packet_sendto (client_conn, proc->message, sizeof(message_t),
	       		     proc->server, UIP_HTONS(UDP_SERVER_PORT));
	       etimer_set (&et, CLOCK_SECOND * 5);
	   }
       }
       else if (ev == ack_event) {
	   printf("***********DATA ACK'd*************\n");
	   printf("last ack: %d seq: %d\n", last_ack, proc->message->seq);
	   printf("**********************************\n");
	   status = 1;
       }

       else {
	  printf("Unknown event %d\n", ev);
       }

    } // end while last ack

    printf("Done sending, count: %d\n", count);
    process_post(PROCESS_BROADCAST, send_fin_event, &status);
} // end while(1)

PROCESS_END();
}





void start_client( )
{
  if (process_is_running(&client_process)) {
      printf("Client is already running\n");
      return;
  }

  set_global_address ();
  process_start(&client_process, NULL);
  process_start(&sender_process, NULL);
}



void send_packet(uip_ipaddr_t *server, const char *ptr)
{
  if (client_conn == NULL) {
      printf("Error - no connection started\n");
      return;
  }


  static message_t message;
  memset(&message, 0, sizeof(message));
  message.seq = last_ack + 1;
  strncpy(message.data, ptr, 32);

  static sender_process_t proc;
  proc.message = &message;
  proc.server = server;


  printf("Posting data %p\n", &proc);
  process_post_synch(&sender_process, PROCESS_EVENT_CONTINUE, &proc);
}

void send_string(uip_ipaddr_t *server, const char *str)
{
  if (client_conn == NULL) {
        printf("Error - no connection created\n");
    }
    else {
	send_packet(server, str);
    }
}


void broadcast_string(const char *str, int len)
{

  if (client_conn == NULL) {
      printf("Error - no connection created\n");
  }
  else {
    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);

    send_packet(&addr, str);
  }
}

