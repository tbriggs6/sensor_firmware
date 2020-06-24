######################################
# spinet router
######################################
######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif
######################################
CONTIKI_PROJECT = spinet
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += rpl-iface.c spi-iface.c spi-regs.c print_buff.c

# turn on experimental long range mode
CC1310_LRM=1
ifdef CC1310_LRM
CFLAGS += -DCC1310_LRM=1
endif

CFLAGS+=-DUIP_FALLBACK_INTERFACE=spi_rpl_interface 

#MODULES += os/services/shell os/sys/energest 

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

#CONTIKI = /home/contiki/contiki-ng
CONTIKI = /home/contiki/lrm-contiki/contiki-ng

-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

