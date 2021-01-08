######################################
# airborneNode 
######################################
MAKE_MAC = MAKE_MAC_TSCH

######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif
######################################
CONTIKI_PROJECT = waterNode
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
