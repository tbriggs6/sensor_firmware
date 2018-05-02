
CFLAGS=-g 

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = testsensor



APPDIRS += /home/contiki/git/firmware/apps/ 
APPS += bcast-service message-service powertrace

#include /home/contiki/git/firmware/apps/shared/Makefile.shared


all:  $(CONTIKI_PROJECT)

MODULES += core/net/ipv6/multicast

CONTIKI_WITH_IPV6 = 1

PROJECT_SOURCEFILES += neighbors.c config.c commandhandler.c datahandler.c

ifeq ($(DEPLOYABLE), yes)
	PROJECT_SOURCEFILES += scif_framework.c scif_scs.c scif_osal_contiki.c
	TARGET = srf06-cc26xx
	BOARD = launchpad/cc1310
endif

CONTIKI = /home/contiki/contiki
include $(CONTIKI)/Makefile.include


.PHONY: mrclean
mrclean: clean
	rm -f ../shared/*.o
	rm -f *.o
	rm -rf obj_cooja COOJA.log testsensor.cooja symbols.c symbols.h org
	rm -rf *.native
