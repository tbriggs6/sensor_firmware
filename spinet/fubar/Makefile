##############################
# testNode 
##############################

include ../Makefile.config

ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif

#CLFLAGS+=-Os

CONTIKI_PROJECT = spinet
all: $(CONTIKI_PROJECT)

CONTIKI = /home/contiki/contiki-ng

PROJECT_SOURCEFILES += spi-dma.c rpl-if.c

CFLAGS+=-DUIP_FALLBACK_INTERFACE=spi_rpl_interface

include $(CONTIKI)/Makefile.identify-target


include $(CONTIKI)/Makefile.include

ifeq ($(PREFIX),)
 PREFIX = fd00::1/64
endif

UNIFLASH=/home/contiki/prog

