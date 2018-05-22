
CONTIKI_PROJECT = sensorNode
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell

MODULES_REL += /home/contiki/git/firmware/apps/messenger
MODULES_REL += /home/contiki/git/firmware/apps/echo
MODULES_REL += $(TARGET)

PROJECT_SOURCEFILES += utils.c 

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include


setcap:
	@echo "Make sure to add"
	@echo
	@echo "contiki ALL = NOPASSWD: /sbin/setcap"
	@echo
	@echo "to /etc/sudoers"
	sudo setcap all=epi sensorNode.native
	