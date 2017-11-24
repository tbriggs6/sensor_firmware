/*
 * fifo.c
 *
 * Maintain a FIFO fixed over an array
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#include <contiki.h>
#include <string.h>

#include "../fifo.h"

void fifo_init(fifo_t * __restrict__ const fifo, char * __restrict__ const memory, int num_elements, int bytes_per_element)
{
	fifo->read_pos = 0;
	fifo->write_pos = 0;
	fifo->num_elements = num_elements;
	fifo->count_elements = 0;
	fifo->bytes_per_element = bytes_per_element;
	fifo->memory = memory;
}

int fifo_size(const fifo_t * const fifo)
{
	return fifo->count_elements;
}


bool fifo_empty(const fifo_t * const fifo)
{

	return (fifo->count_elements == 0);
}

bool fifo_full(const fifo_t * const fifo)
{
	return (fifo->count_elements == fifo->num_elements);
}

#define FIFO_ADDR(f,i) (f->memory + (i * f->bytes_per_element))

bool fifo_add(fifo_t * const __restrict__ fifo, const void * __restrict__ const element)
{
	if (fifo_full(fifo))
		return false;

	 memcpy(FIFO_ADDR(fifo, fifo->write_pos), element, fifo->bytes_per_element);
	 fifo->write_pos = (fifo->write_pos + 1) % fifo->num_elements;
	 fifo->count_elements++;

	 return true;
}

bool fifo_get(fifo_t * const __restrict__ fifo, void * __restrict__ element)
{
	if (fifo_empty(fifo))
		return false;

	memcpy(element, FIFO_ADDR(fifo, fifo->read_pos), fifo->bytes_per_element);

	return true;
}

void fifo_advance(fifo_t * const fifo)
{
	if (fifo->count_elements == 0) return;

	fifo->read_pos = (fifo->read_pos + 1) % fifo->num_elements;
	fifo->count_elements--;
}


char *fifo_peek(const fifo_t *const fifo)
{
	if (fifo_empty(fifo)) return NULL;

	return FIFO_ADDR(fifo, fifo->read_pos);
}

