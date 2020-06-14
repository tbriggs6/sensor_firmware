########################
###  sensorNode Makefile
########################


ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif



CONTIKI_PROJECT = sensorNode
all: scsdir $(CONTIKI_PROJECT)

MODULES += os/services/shell os/sys/energest 

#MAKE_MAC=MAKE_MAC_NULLMAC

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command


ifeq ($(TARGET),cc26x0-cc13x0)
CFLAGS += -I$(CURDIR)/scs
PROJECT_SOURCEFILES += scs/scif_framework.c scs/scif_scs.c scs/scif_osal_none.c sensor-cc1310.c
endif

#PROJECT_SOURCEFILES += utils.c 

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

scsdir:
	@echo "Ensure scs dir"
	mkdir -p build/cc26x0-cc13x0/launchpad/cc1310/obj/scs

setcap:
	@echo "Make sure to add"
	@echo
	@echo "contiki ALL = NOPASSWD: /sbin/setcap"
	@echo
	@echo "to /etc/sudoers"
	sudo setcap all=epi sensorNode.native

prog: all
	@echo Programming 2000063 in $(UNIFLASH)
	cp sensorNode.hex $(UNIFLASH)/2000063/user_files/images
	(cd /home/contiki/prog/2000063 && ./dslite.sh)
	
	@echo Programming 2000063 in $(UNIFLASH)
	cp sensorNode.hex $(UNIFLASH)/L1001924/user_files/images
	(cd $(UNIFLASH)/L1001924 && ./dslite.sh)
	
	
	
