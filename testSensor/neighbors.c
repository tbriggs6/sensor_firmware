/*
 * neighbors.c
 *
 *  Created on: Jan 11, 2017
 *      Author: tbriggs
 */




#include <contiki.h>
#include <contiki-lib.h>
#include <contiki-net.h>
#include <stdio.h>
#include <net/ip/uip-debug.h>
#include <net/ipv6/sicslowpan.h>


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


#include "config.h"
#define UDP_PORT 8383


static struct simple_udp_connection neighbor_connection;

#define MAX_NEIGHBORS (16)
typedef struct {
	uip_ipaddr_t address;
	int rssi;
	int router;
} neighbor_t;

static int num_neighbors = 0;
static neighbor_t neighbors[MAX_NEIGHBORS];

int neighbors_getnum( )
{
	return num_neighbors;
}


void neighbors_get(int pos, uip_ipaddr_t *address)
{
	if (pos < num_neighbors) {
		memcpy(address, &neighbors[pos], sizeof(uip_ipaddr_t));
	}
}

int neighbors_get_rssi(int pos)
{
	if (pos < num_neighbors)
		return neighbors[pos].rssi;
	else return -200;
}

int neighbors_get_router(int pos)
{
	if (pos < num_neighbors)
		return neighbors[pos].router;
	else return 0;
}

static void neighbors_receiver(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port,
	const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const uint8_t *data, uint16_t length)
{
	int v = sicslowpan_get_last_rssi();

#if (DEBUG > 0)
	printf("RSSI ");
	uip_debug_ipaddr_print(sender_addr);
	printf(" = %d\n", v);
#endif

}


static void print_neighbors( )
{

	uip_ds6_nbr_t *nbr;
	char ping[4];

	int count = 0;


	PRINTF("[PRINT NEIGHBORS] Neighbors:\r\n");
	nbr = nbr_table_head(ds6_neighbors);
	while (nbr != NULL) {

		simple_udp_sendto_port(&neighbor_connection,
				(char *) &ping, sizeof(ping), &(nbr->ipaddr), UDP_PORT);

		int rssi = sicslowpan_get_last_rssi();

		if (count < MAX_NEIGHBORS) {
			memcpy(&neighbors[count].address, &(nbr->ipaddr), sizeof(uip_ipaddr_t));
			neighbors[count].rssi = rssi;
			neighbors[count].router = nbr->isrouter;
			count++;
		}

#if (DEBUG > 0)
		PRINTF("   %d ",nbr->isrouter);
		uip_debug_ipaddr_print(&(nbr->ipaddr));
		PRINTF(" rssi=%d\r\n", rssi);
#endif
		nbr = nbr_table_next(ds6_neighbors, nbr);
	}
	num_neighbors = count;

}


PROCESS(neighbors_proc, "Show neighbors");
PROCESS_THREAD(neighbors_proc, ev, data)
{
	static struct etimer et;
	PROCESS_BEGIN( );

	while(1) {
		print_neighbors();

		etimer_set(&et, config_get_neighbor_interval());
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
	}

	PROCESS_END();
}

void neighbors_init( )
{
	simple_udp_register(&neighbor_connection, 0, NULL, UDP_PORT, neighbors_receiver);

	process_start(&neighbors_proc, NULL);
}
