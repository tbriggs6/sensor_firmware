/*
 * utils.c -
 *
 * Basic utilities to make life easier for the rest of the project.
 *
 *  Created on: Nov 16, 2017
 *      Author: tbriggs
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <net/ip/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ip/uip-debug.h>

#include "utils.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

int ishex(char ch)
{
  if ((ch >= '0') && (ch <= '9')) return 1;
  if ((ch >= 'A') && (ch <= 'F')) return 1;
  if ((ch >= 'a') && (ch <= 'f')) return 1;
  return 0;
}

void uip_from_string(const char *str, uip_ipaddr_t *addr)
{
  if (strlen(str) > 40) {
      printf("ERR-0 : address is too long\r\n");
      return;
  }

  char buff[41];
  strcpy(buff, str);
  char *ptr = buff;

  // store the 128-bit address
  uint16_t u16[8];

  // find if there is a double colon
  int i = 0;
  int curr_field = 0;
  int double_pos = -1;
  int state = 0;
  int start = 0;

  while (ptr[i] != 0x00) {

      PRINTF("State: %d ptr[i]: %c curr_field: %d\r\n", state, ptr[i], curr_field);
      switch(state) {
	case 0:
	  if (ishex(ptr[i])) state = 0;
	  else if (ptr[i] == ':') {
	      ptr[i] = 0x00;
	      u16[curr_field++] = strtol(&ptr[start], NULL, 16);
	      state = 1;
	  }
	  else {
	      printf("ERR-00 : invalid address\r\n");
	      goto error;
	  }
	  break;

	case 1:
	  if (ishex(ptr[i])) {
	      start = i;
	      state = 2;
	  }
	  else if (ptr[i] == ':') {
	      double_pos = curr_field;
	      state = 3;
	  }
	  else {
	      printf("ERR-01 : invalid address string\r\n");
	      goto error;
	  }
	  break;

	case 2:
	  if (ishex(ptr[i])) {
	      state = 2;
	  }
	  else if ((ptr[i] == ':') && (curr_field < 7)) {
	      ptr[i] = 0x00;
	      u16[curr_field++] = strtol(&ptr[start], NULL, 16);
	      state = 1;
	  }
	  else {
	      printf("ERR-02: invalid address string\r\n");
	      goto error;
	  }
	  break;

	case 3:
	  if (ishex(ptr[i])) {
	      state = 4;
	      start = i;
	  }
	  else {
	      printf("ERR-03 : invalid address string\r\n");
	      goto error;
	  }
	  break;

	case 4:
	  if (ishex(ptr[i])) {
	      state = 4;
	  }
	  else if ((ptr[i] == ':') && (curr_field < 7)) {
	      ptr[i] = 0x00;
	      u16[curr_field++] = strtol(&ptr[start], NULL, 16);
	      state = 3;
	  }
      } // end switch

      i++;
  } // end while

  if ((state != 2) && (state != 4)) {
      printf("ERR-05 : invalid address string\r\n");
      goto error;
  }
  u16[curr_field++] = strtol(&ptr[start], NULL, 16);

  // at this point, we need to (possibly) slide the terms in the u16 array down
  if (double_pos > 0) {

      PRINTF("Handling device address shift due to :: curr(%d) double(%d)\r\n", curr_field, double_pos);
      for (int i = 0; i < (curr_field - double_pos ); i++) {
	  PRINTF("   moving u16[%d] = %x to u16[%d]\r\n", curr_field - i -1 , u16[curr_field - i - 1],7 -i);
	  u16[7 - i] = u16[curr_field - i - 1];
      }
      for (int i = 0; i < 8 - curr_field; i++) {
	  PRINTF("   filling u16[%d] = 0\r\n", i);
	  u16[i + double_pos] = 0;
      }

  }

   uip_ip6addr(addr,
	       u16[0], u16[1], u16[2], u16[3],
	       u16[4], u16[5], u16[6], u16[7]);

error:
  return;
}



