/*
 * utils.c
 *
 *  Created on: May 21, 2018
 *      Author: contiki
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static int ishex(char ch)
{
	if ((ch >= '0') && (ch <= '9')) return 1;
	if ((ch >= 'a') && (ch <= 'f')) return 1;
	if ((ch >= 'A') && (ch <= 'F')) return 1;

	return 0;
}

// fill the addr struct from the text string:  a:b::c:d
int ip6addr_filladdr(uint16_t *addr, const char *addrstr)
{
	char tokens[8][5];
	int start, end, i;
	int state = 0;
	int idx = 0;

	memset(&tokens, 0, sizeof(tokens));

	i = 0;
	while ((state >= 0) && (state != 7)) {
		char ch = addrstr[i];
		i = (state < 4) ? i+1 : i-1;
		switch(state) {
		case 0:
			if (ishex(ch)) {
				start = 0;
				end = 1;
				state = 1;
			}
			else state = 1;
			break;

		case 1:
			if (ishex(ch)) {
				end++;
				state = 1;
			}
			else if (ch == ':') {
				if (idx >= 8) state = -2;
				else if ((end-start) > 4) state = -2;
				else {
					strncpy(tokens[idx++],addrstr+start,(end-start));
					start = end = i;
					state = 2;
				}
			}
			else if (ch == 0) {
				if (idx != 7) state = -2;
				else if ((end-start) > 4) state = -2;
				else {
					strncpy(tokens[idx++],addrstr+start,(end-start));
					state = -10;
				}
			}

			else state = -1;
			break;

		case 2:
			if (ishex(ch)) {
				start = i;
				state = 1;
			}
			else if (ch == ':') {
				state = 4;
				end = strlen(addrstr) - 1;
				i = start = end ;
				idx = 7;
			}
			else {
				state = -2;
			}
			break;

		case 4:
			if (ishex(ch)) {
				state = 5;
				start--;
			}
			else {
				state = -2;
			}
			break;

		case 5:
			if (ishex(ch)) {
				state = 5;
				start--;
			}
			else if (ch == ':') {
				if (idx <= 0) state = -2;
				else if ((end-start) > 4) state = -2;
				else {
					strncpy(tokens[idx--],addrstr+start+1,(end-start));
					start = end = i;
					state = 6;
				}
			}
			else {
				state = -2;
			}
			break;

		case 6:
			if (ishex(ch)) {
				start--;
				state = 5;
			}
			else if (ch == ':') {
				state = -10;
			}
			else {
				state = -2;
			}
			break;
		}
	}

	// valid string
	if (state == -10) {
		for (i = 0; i < 8; i++) {
			addr[i] = strtoul(tokens[i],NULL, 16);
		}
		return 1;
	}
	else {
		return 0;
	}

}




