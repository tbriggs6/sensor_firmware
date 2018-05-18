
CONTIKI_PROJECT = sensorNode
all: $(CONTIKI_PROJECT)

CONTIKIDIRS += /home/contiki/git/firmware/apps/
MODULES += message-service-tcp
MODULES += os/services/shell

CONTIKI = /home/contiki/contiki-ng
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)
MODULES_REL += /home/contiki/git/firmware/apps/

#PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul native

include $(CONTIKI)/Makefile.include
