######################################
# testNode 
######################################
######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif
######################################
CONTIKI_PROJECT = testNode
all: $(CONTIKI_PROJECT)

ifdef CC1310_LRM
CFLAGS += -DCC1310_LRM=1
endif

MODULES += os/services/shell os/sys/energest 

CFLAGS += -ggdb

UNIFLASH=/home/contiki/prog

#CONTIKI = /home/contiki/contiki-ng
CONTIKI = /home/contiki/lrm-contiki/contiki-ng

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
