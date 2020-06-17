#ifndef _DMA_H
#define _DMA_H

#include <contiki.h>


#define INTR_RXFIN (0x001)
#define INTR_TXFIN (0x002)
#define INTR_RXERR (0x004)

#define INTR_OVFLOW_RXFIN (0x0100)
#define INTR_OVFLOW_TXFIN (0x0200)
#define INTR_OVFLOW_RXERR (0x0400)

void dma_init( );
void dma_xfer(void *slv_out, void *slv_in, int len);
void dma_xfer_rxonly(void *slv_in, int len);
int dma_in_progress( );

void clear_event_flag( unsigned int mask );
void clear_event_all_flags( );
unsigned int get_event_flags( void );

extern process_event_t dma_event;

#endif

