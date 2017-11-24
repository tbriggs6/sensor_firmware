CFLAGS+=-g -DCONFIG_TYPE=200

ifndef TARGET
TARGET=srf06-cc26xx
#BOARD=launchpad/cc2650
#BOARD=watersensor/cc1310
BOARD=launchpad/cc1310
endif

ifeq ($(TARGET), srf06-cc26xx)
CFLAGS+=-DCFGRADIO -DCFGPERIPH
endif

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = testNode
APPS = powertrace

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6 = 1
#CONTIKI_WITH_RIME = 1

#define LLSEC802154_CONF_ENABLED 1
##define NETSTACK_CONF_LLSEC noncoresec_driver
##define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 1

PROJECT_SOURCEFILES += sender_udp.c ../utils.c 

#PROJECT_SOURCEFILES += scif_framework.c scif_scs.c scif_osal_contiki.c \
#	../config.c ../rtc.c ../message.c ../neighbors.c sender_udp.c command_udp.c


ifndef HOME
HOME=/home/contiki
endif

CONTIKI = $(HOME)/contiki
include $(CONTIKI)/Makefile.include

.PHONY: mrclean
mrclean: clean
	rm -rf *.o *.log 