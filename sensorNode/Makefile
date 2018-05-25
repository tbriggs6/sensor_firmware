
CONTIKI_PROJECT = sensorNode
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell os/sys/energest 

#MAKE_MAC=MAKE_MAC_NULLMAC

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command




ifeq ($(TARGET),srf06-cc26xx)
PROJECT_SOURCEFILES += scif_framework.c scif_osal_contiki.c scif_scs.c  sensor-cc1310.c
endif

#PROJECT_SOURCEFILES += utils.c 

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

patch:
	P=`pwd`
	@echo Applying patches to $(CONTIKI)
	cd $(CONTIKI)/arch/cpu/cc26xx-cc13xx
	patch < $P/cc1310-config.patch 
	

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
	
	
	
