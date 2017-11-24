/*
 * fifo.h
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdbool.h>

typedef struct {
	int read_pos;
	int write_pos;
	int num_elements;
	int count_elements;
	int bytes_per_element;
	char *memory;
} fifo_t;

void fifo_init(fifo_t * __restrict__ const fifo,  char * __restrict__ const memory, int num_elements, int bytes_per_element);
bool fifo_empty(const fifo_t * const fifo);
bool fifo_full(const fifo_t * const fifo);
int fifo_size(const fifo_t * const fifo);

bool fifo_add(fifo_t * __restrict__ const fifo, const void * __restrict__ const element);
bool fifo_get(fifo_t * __restrict__ const fifo, void *  __restrict__ element);
void fifo_advance(fifo_t * const fifo);

char *fifo_peek(const fifo_t * const fifo);

#endif /* FIFO_H_ */
