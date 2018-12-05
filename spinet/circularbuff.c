#include <stdlib.h>
#include <stdint.h>
#include "circularbuff.h"
#include <contiki.h>
#include <sys/critical.h>

void circbuff_init(circbuff_t *buff, uint8_t *store, int len)
{
    buff->buff = store;
    buff->count = 0;
    buff->max_size = len;
    buff->rd_pos = 0;
    buff->wr_pos = 0;
}

int circbuff_addch(circbuff_t *buff, char ch)
{
    if (buff->count == buff->max_size)
        return -1;

    int cs = critical_enter( );

    buff->buff[buff->wr_pos++] = ch;
    buff->count++;
    buff->wr_pos %= buff->max_size;

    critical_exit(cs);

    return 0;
}

int circbuff_getch(circbuff_t *buff)
{

    if (buff->count == 0) return -1;

    int cs = critical_enter( );

    char ch = buff->buff[buff->rd_pos++];
    buff->count--;
    buff->rd_pos %= buff->max_size;
    
    critical_exit(cs);

    return ch;
}
int circbuff_isfull(circbuff_t *buff)
{
    return (buff->count == buff->max_size);
}

int circbuff_isempty(circbuff_t *buff)
{
    return (buff->count == 0);
}

int circbuff_hasdata(circbuff_t *buff)
{
    return (buff->count > 0);
}

int circbuff_almostfull(circbuff_t *buff)
{
    return (buff->max_size - buff->count) < 10;
}

int circbuff_length(circbuff_t *buff)
{
  return buff->count;
}
