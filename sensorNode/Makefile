FLAGS=-g -DCONFIG_TYPE=200
TARGET=srf06-cc26xx
#BOARD=launchpad/cc2650
#BOARD=watersensor/cc1310
BOARD=launchpad/cc1310

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = sensor
APPS = powertrace

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6 = 1
#CONTIKI_WITH_RIME = 1

#define LLSEC802154_CONF_ENABLED 1
##define NETSTACK_CONF_LLSEC noncoresec_driver
##define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 1

PROJECT_SOURCEFILES += scif_framework.c scif_scs.c scif_osal_contiki.c \
	../config.c ../rtc.c ../message.c ../neighbors.c sender_udp.c command_udp.c

CONTIKI = /home/tbriggs/contiki
include $(CONTIKI)/Makefile.include
