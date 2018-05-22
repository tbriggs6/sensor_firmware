#ifndef UTILS_H
#define UTILS_H


#include <stdint.h>

// fill the addr struct from the text string:  a:b::c:d
int ip6addr_filladdr(uint16_t *addr, const char *addrstr);

#endif
