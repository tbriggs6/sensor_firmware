#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#include "net/ipv6/multicast/uip-mcast6-engines.h"

/* Change this to switch engines. Engine codes in uip-mcast6-engines.h */
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_ROLL_TM

#define NETSTACK_CONF_RDC nullrdc_driver
#define RF_CORE_CONF_CHANNEL 25


/* For Imin: Use 16 over NullRDC, 64 over Contiki MAC */
#define ROLL_TM_CONF_IMIN_1         64

#undef UIP_CONF_IPV6_RPL
#undef UIP_CONF_ND6_SEND_RA
#undef UIP_CONF_ROUTER
#define UIP_CONF_ND6_SEND_RA         0
#define UIP_CONF_ROUTER              1
#define UIP_MCAST6_ROUTE_CONF_ROUTES 1


#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0

/* Code/RAM footprint savings so that things will fit on our device */
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#undef UIP_CONF_MAX_ROUTES
#define NBR_TABLE_CONF_MAX_NEIGHBORS  10
#define UIP_CONF_MAX_ROUTES           10

//TODO: Test code for enabling the encryption
//#undef LLSEC802154_CONF_ENABLED
//#define LLSEC802154_CONF_ENABLED          1
//#undef NETSTACK_CONF_FRAMER
//#define NETSTACK_CONF_FRAMER              noncoresec_framer
//#undef NETSTACK_CONF_LLSEC
//#define NETSTACK_CONF_LLSEC               noncoresec_driver //doesont exist
//#undef NONCORESEC_CONF_SEC_LVL
//#define NONCORESEC_CONF_SEC_LVL           5
//
//#define NONCORESEC_CONF_KEY { 0x00 , 0x01 , 0x02 , 0x03 ,
//                              0x04 , 0x05 , 0x06 , 0x07 ,
//                              0x08 , 0x09 , 0x0A , 0x0B ,
//                              0x0C , 0x0D , 0x0E , 0x0F }

//End Encryption test code

#endif /* PROJECT_CONF_H_ */
