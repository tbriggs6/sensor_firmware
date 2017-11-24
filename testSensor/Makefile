
CFLAGS=-g 

#DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = testSensor



APPDIRS += /home/contiki/git/firmware/apps/ 
APPS += bcast-service	

#include /home/contiki/git/firmware/apps/shared/Makefile.shared


all: info $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6 = 1

#PROJECT_SOURCEFILES += ../bcast-service.c



CONTIKI = /home/contiki/contiki
include $(CONTIKI)/Makefile.include

.PHONY: mrclean
mrclean: clean
	rm -f ../shared/*.o
	rm -f *.o
	rm -rf obj_cooja COOJA.log testsensor.cooja symbols.c symbols.h org
	rm -rf *.native
