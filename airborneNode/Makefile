
CONTIKI_PROJECT = airborneNode
all: scsdir $(CONTIKI_PROJECT)

MODULES += os/services/shell os/sys/energest 

MAKE_MAC=MAKE_MAC_NULLMAC

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command


ifeq ($(TARGET),srf06-cc26xx)
CFLAGS += -I$(CURDIR)/scs
PROJECT_SOURCEFILES += scs/scif_framework.c scs/scif_scs.c scs/scif_osal_none.c airborne-cc1310.c
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
	
scsdir:
	@echo "Ensure scs dir"
	mkdir -p obj_srf06-cc26xx/scs


setcap:
	@echo "Make sure to add"
	@echo
	@echo "contiki ALL = NOPASSWD: /sbin/setcap"
	@echo
	@echo "to /etc/sudoers"
	sudo setcap all=epi sensorNode.native



prog: all
	@echo Programming 2000063 in $(UNIFLASH)
	cp airborneNode.hex $(UNIFLASH)/2000063/user_files/images
	(cd /home/contiki/prog/2000063 && ./dslite-ab.sh)
	
#	@echo Programming 2000063 in $(UNIFLASH)
#	cp sensorNode.hex $(UNIFLASH)/L1001924/user_files/images
	#(cd $(UNIFLASH)/L1001924 && ./dslite.sh)
	
	
	
