#include "contiki.h"
#include "sys/log.h"



void print_hex_dump_fn(const char *prefix_str,
			   int rowsize, const char *buf, size_t len)
 {
	int col = 0;
	int i = 0;

	printf("\n%s : %p : %d\n", prefix_str, buf, len);

	while (i < len) {
		printf("%-5.5d ", i);
		col = 0;
		while ((col < rowsize) && (i < len)) {
			printf("%-2.2x ", buf[i]);
			i++;
			col++;
		}
		printf("\n");
	}

 }
