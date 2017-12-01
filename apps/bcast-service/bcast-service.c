/**
 * @file bcast-service.c
 * @author Tom Briggs
 * @brief A broadcast service (source and sink)
 *
 * This is the top-level wrapper for a broad-cast service
 * that can listen (be a sink) for incoming broadcast requests
 * and can send (be a source) for outgoing broadcast messages.
 *
 */
#include <bcast-service.h>
#include <bcast-sink.h>
#include <bcast-source.h>


void bcast_init( void )
{

    //printf("Multicast Engine: '%s'\n", UIP_MCAST6.name);

    bcast_sink_init();
    bcast_source_init( );

}
