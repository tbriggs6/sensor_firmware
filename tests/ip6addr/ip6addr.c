#include "../../utils/utils.h"
#include <stdint.h>

int main(int argc, char **argv)
{
	int i;

	uint16_t addr[8];

	ip6addr_filladdr((uint16_t*)&addr, "fd00::1:2:3");

	for (i = 0; i < 8; i++) {
		printf("%d - %-2.2x\n", i, addr[i]);
	}
}
