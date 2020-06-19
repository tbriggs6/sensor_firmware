/*---------------------------------------------------------------------------*/
#ifndef SHARED_PROJECT_CONF_H_
#define SHARED_PROJECT_CONF_H_

#define DOT_15_4G_CONF_FREQUENCY_BAND_ID DOT_15_4G_FREQUENCY_BAND_915
// 902MHz + (channel * 200)
// channel = (915000 - 902200) / 200
#define IEEE802154_CONF_DEFAULT_CHANNEL      20
#define IEEE802154_CONF_PANID            0xDEED
#define RPL_CONF_MODE					RF_MODE_SUB_1_GHZ
#define RPL_CONF_WITH_PROBING                 1


/* ----------------------------------------------- */
// disable watchdog for debugging
#define WATCHDOG_CONF_DISABLE									1

#define LPM_MODE_MAX_SUPPORTED_CONF LPM_MODE_AWAKEr4

//#define TI_I2C_CONF_ENABLE									  0
//#define TI_UART_CONF_UART1_ENABLE						  0
#define TI_SPI_CONF_SPI0_ENABLE							  1
#define TI_SPI_CONF_SPI1_ENABLE								0
//#define TI_SPI_CONF_I2C0_ENABLE								0
#define TI_NVS_CONF_NVS_INTERNAL_ENABLE		    0
#define TI_NVS_CONF_NVS_EXTERNAL_ENABLE			  0


/*---------------------------------------------------------------------------*/
/* Enable the ROM bootloader */
//#define ROM_BOOTLOADER_ENABLE                 1

/*---------------------------------------------------------------------------*/

//#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_DBG
//#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_DBG

#endif /* SHARED_PROJECT_CONF_H_ */
