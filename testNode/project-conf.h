/*---------------------------------------------------------------------------*/
#ifndef SHARED_PROJECT_CONF_H_
#define SHARED_PROJECT_CONF_H_

#define DOT_15_4G_CONF_FREQUENCY_BAND_ID DOT_15_4G_FREQUENCY_BAND_915
#define RF_CONF_TXPOWER_BOOST_MODE 	1

#if (MAKE_MAC != MAKE_MAKE_CSMA)
#error Only CSMA supported now
#endif



// 902MHz + (channel * 200)
// channel = (915000 - 902200) / 200
#define IEEE802154_CONF_DEFAULT_CHANNEL      20
#define IEEE802154_CONF_PANID            0xDEED
#define RPL_CONF_MODE					RF_MODE_SUB_1_GHZ
#define RPL_CONF_WITH_PROBING                 1
#define UIP_CONF_TCP												  1

/* ----------------------------------------------- */
// disable watchdog for debugging
#define WATCHDOG_CONF_DISABLE									1
#define TI_I2C_CONF_ENABLE									  0
#define TI_UART_CONF_UART1_ENABLE						  0
#define TI_SPI_CONF_SPI0_ENABLE							  1
#define TI_SPI_CONF_SPI1_ENABLE								0
#define TI_SPI_CONF_I2C0_ENABLE								0
#define TI_NVS_CONF_NVS_INTERNAL_ENABLE		    1
#define TI_NVS_CONF_NVS_EXTERNAL_ENABLE			  0

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

/*******************************************************/
/************* Other system configuration **************/
/*******************************************************/

/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_WARN
//#define TSCH_LOG_CONF_PER_SLOT                     1

#endif /* SHARED_PROJECT_CONF_H_ */
