
CONTIKI_PROJECT = sensorNode
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell os/sys/energest 

include ../modules/config/Makefile.config
include ../modules/echo/Makefile.echo
include ../modules/messenger/Makefile.messenger
include ../modules/command/Makefile.command


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
	
foo:
	@echo $(TARGET)
	@echo $(PROJECT_SOURCEFILES)
