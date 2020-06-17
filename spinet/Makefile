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

PROJECT_SOURCEFILES += rpl-iface.c spi-iface.c spi-regs.c

MODULES += os/services/shell os/sys/energest 

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

