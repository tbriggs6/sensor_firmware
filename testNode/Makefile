FLAGS=-g
TARGET=srf06-cc26xx
BOARD=launchpad/cc2650
#BOARD=watersensor/cc1310


DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = testnode
APPS = powertrace 

all: $(CONTIKI_PROJECT)

#CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RIME = 1



#PROJECT_SOURCEFILES += testnode.c

CONTIKI = /home/tbriggs/contiki
include $(CONTIKI)/Makefile.include
