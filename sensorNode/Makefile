
CONTIKI_PROJECT = sensorNode
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell

MODULES_REL += /home/contiki/git/firmware/apps/messenger
MODULES_REL += /home/contiki/git/firmware/apps/echo
MODULES_REL += $(TARGET)

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include
