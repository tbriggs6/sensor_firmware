######################################
# spinet router
######################################
MAKE_MAC = MAKE_MAC_CSMA
MAKE_MAC = MAKE_MAC_CSMA
DOT_15_4G_CONF_FREQ_BAND_ID = DOT_15_4G_CONF_FREQ_BAND_915 
#MAKE_ROUTING = MAKE_ROUTING_RPL_LITE
#MAKE_NET = MAKE_NET_IPV6

######################################
ifndef TARGET
$(error TARGET is not set)
endif

ifndef BOARD
$(error BOARD is not set)
endif
######################################
CONTIKI_PROJECT = spinet
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += rpl-iface.c spi-iface.c spi-regs.c print_buff.c

# turn on experimental long range mode
#CC1310_LRM=1
#ifdef CC1310_LRM
#CFLAGS += -DCC1310_LRM=1
#endif

CFLAGS+=-DUIP_FALLBACK_INTERFACE=spi_rpl_interface 

MODULES += os/services/shell os/services/energest 

CFLAGS += -Os

UNIFLASH=/home/contiki/prog

CONTIKI = /home/contiki/contiki-ng
#CONTIKI = /home/contiki/lrm-contiki/contiki-ng

-include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.include

