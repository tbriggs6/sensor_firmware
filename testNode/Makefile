######################################
# testNode 
######################################
MAKE_MAC = MAKE_MAC_TSCH
#MAKE_ROUTING = MAKE_ROUTING_RPL_LITE
#MAKE_NET = MAKE_NET_IPV6

######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif

CSMA_CONF_ACK_WAIT_TIME=(RIMTER_SECOND / 200)
CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME=(RTIMER_SECOND / 750)

ifeq ($(TARGET),launchpad/cc1352p1)
CFLAGS += -DSET_CCFG_MODE_CONF_XOSC_CAP_MOD=0x0
CFLAGS += -DSET_CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA=0xC1
endif

######################################
CONTIKI_PROJECT = testNode
all: $(CONTIKI_PROJECT)

#ifdef CC1310_LRM
#CFLAGS += -DCC1310_LRM=1
#endif

MODULES += os/services/shell os/sys/energest 

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command
include ../modules/sensors/Makefile.sensors

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

CONTIKI = /home/contiki/contiki-ng
#CONTIKI = /home/contiki/lrm-contiki/contiki-ng

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
	
	@echo Programming L1001924 in $(UNIFLASH)
	cp testNode.hex $(UNIFLASH)/L1001924/user_files/images
	(cd $(UNIFLASH)/L1001924 && ./dslite.sh)
