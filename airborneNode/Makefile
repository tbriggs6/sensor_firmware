######################################
# airborneNode 
######################################
MAKE_MAC = MAKE_MAC_CSMA
DOT_15_4G_CONF_FREQ_BAND_ID = DOT_15_4G_CONF_FREQ_BAND_915 

######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif

ifeq ($(BOARD),launchpad/cc1352p1)
CFLAGS += -DSET_CCFG_MODE_CONF_XOSC_CAP_MOD=0x0
CFLAGS += -DSET_CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA=0xC1
endif

######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif
######################################
CONTIKI_PROJECT = airborneNode
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell os/sys/energest 

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command
include ../modules/sensors/Makefile.sensors

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

CONTIKI = /home/contiki/contiki-ng

-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

setcap:
	@echo "Make sure to add"
	@echo
	@echo "contiki ALL = NOPASSWD: /sbin/setcap"
	@echo
	@echo "to /etc/sudoers"
	sudo setcap all=epi testNode.native

prog: all
	@echo Programming 2000063 in $(UNIFLASH)
	cp testNode.hex $(UNIFLASH)/2000063/user_files/images
	(cd /home/contiki/prog/2000063 && ./dslite.sh)
