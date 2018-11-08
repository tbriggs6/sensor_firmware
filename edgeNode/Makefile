CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = /home/contiki/contiki-ng


MAKE_MAC=MAKE_MAC_NULLMAC

# The BR is either native or embedded, and in the latter case must support SLIP
PLATFORMS_EXCLUDE = nrf52dk

# Include RPL BR module
MODULES += os/services/rpl-border-router
# Include webserver module
MODULES_REL += webserver
# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include


ifeq ($(PREFIX),)
 PREFIX = fd00::1/64
endif

UNIFLASH=/home/contiki/prog

prog: all
	@echo Programming L20006P8 in $(UNIFLASH)
	cp border-router.hex $(UNIFLASH)/L20006P8/user_files/images
	(cd /home/contiki/prog/L20006P8 && ./dslite.sh)
	

#$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
#	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)
#
#connect-router:	$(CONTIKI)/tools/tunslip6
#	sudo $(CONTIKI)/tools/tunslip6 -s /dev/ttyACM0 -v $(PREFIX)
#
#connect-router-cooja:	$(CONTIKI)/tools/tunslip6
#	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 $(PREFIX)

