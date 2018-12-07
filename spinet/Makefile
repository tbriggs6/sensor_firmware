CONTIKI_PROJECT = spinet
all: $(CONTIKI_PROJECT)

CONTIKI = /home/contiki/contiki-ng

PROJECT_SOURCEFILES += spi-dma.c rpl-if.c

CFLAGS+=-DUIP_FALLBACK_INTERFACE=spi_rpl_interface

include $(CONTIKI)/Makefile.identify-target
#MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include

ifeq ($(PREFIX),)
 PREFIX = fd00::1/64
endif

UNIFLASH=/home/contiki/prog

#prog: all
#	@echo Programming L20006P8 in $(UNIFLASH)
#	cp border-router.hex $(UNIFLASH)/L20006P8/user_files/images
#	(cd /home/contiki/prog/L20006P8 && ./dslite.sh)
	

