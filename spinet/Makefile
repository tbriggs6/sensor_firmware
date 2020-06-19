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

CFLAGS+=-DUIP_FALLBACK_INTERFACE=spi_rpl_interface

MODULES += os/services/shell os/sys/energest 

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

