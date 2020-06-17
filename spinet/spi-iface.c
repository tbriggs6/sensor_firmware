#include "contiki.h"
#include "dev/spi.h"
#include "sys/log.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"


#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/gpio.h)
#include DeviceFamily_constructPath(driverlib/ssi.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include <ti/drivers/Board.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "Board.h"

#include <stdint.h>

#include "spi-iface.h"
#include "spi-regs.h"

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_DBG

#define SPI_CONTROLLER    SPI_CONTROLLER_SPI0

#define SPI_PIN_SCK       30
#define SPI_PIN_MOSI      29
#define SPI_PIN_MISO      28
#define SPI_PIN_CS        1

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_DBG

static SPI_Handle spiHandle;
static SPI_Params spiParams;
static SPI_Transaction spiTrans;

static char spi_data[4] =	{ 0 };
static char spi_pkt_data[512];
static int spi_pkt_data_len = 0;

static volatile uint16_t regnum = 0;
static volatile uint16_t req_pkt_len = 0;


/**
 * Start a transfer on the SPI bus for 4 bytes, and wait
 * for it to signal completion.
 */
PROCESS(spi_rdcmd, "SPI read command");
PROCESS(spi_rdreg, "SPI read reg");
PROCESS(spi_wrreg, "SPI write reg");
PROCESS(spi_rcvpkt, "SPI rcv packet");
PROCESS(spi_xmtpkt, "SPI xmit packet");


process_event_t spi_event;


static void raise_spi_intr ()
{
	GPIO_setDio(CC1310_LAUNCHXL_PIN_SPIRDY);


}

static void clear_spi_intr ()
{
	GPIO_clearDio(CC1310_LAUNCHXL_PIN_SPIRDY);
}

static void toggle_spi_intr ()
{
	LOG_DBG("Toggle SPI intr\n");
	clear_spi_intr ();
	clock_delay_usec (5);
	raise_spi_intr ();
}

//static void spi_drain_rx ()
//{
//	uint32_t bob;
//	uint32_t count = 0;
//
//	SSIIntDisable(SSI0_BASE, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
//
//	while (ROM_SSIDataGetNonBlocking(SSI0_BASE, &bob))
//		count++;
//
//	LOG_DBG("SPI drained %d bytes\n", (int ) count);
//}
//
//
//void spi_transfer_callback(void *data, int status, void *txbuf, void *rxbuf)
//{
//	LOG_INFO("SPI callback!");
//}
//
//int get_mode(const spi_device_t *dev)
//{
//  /* Select the correct SPI mode */
//  if(dev->spi_pha == 0 && dev->spi_pol == 0) {
//    return SSI_FRF_MOTO_MODE_0;
//  } else if(dev->spi_pha != 0 && dev->spi_pol == 0) {
//    return SSI_FRF_MOTO_MODE_1;
//  } else if(dev->spi_pha == 0 && dev->spi_pol != 0) {
//    return SSI_FRF_MOTO_MODE_2;
//  } else {
//    return SSI_FRF_MOTO_MODE_3;
//  }
//}
//
//// Initialize the SPI bus.
//void spi_init ()
//{
//
//	spi_event = process_alloc_event ();
//
//	//TODO this should have been set in the platform?
//	IOCPinTypeGpioInput (1);   // chip select from PI
//	GPIO_setOutputEnableDio (26, GPIO_OUTPUT_ENABLE);
//	GPIO_clearDio (26);
//
//	IOCPinTypeGpioOutput (26);   // chip select from PI
//
//	spi_device_t spidev =
//				{
//						.spi_controller = SPI_CONTROLLER,
//						.pin_spi_sck = SPI_PIN_SCK,
//						.pin_spi_miso = SPI_PIN_MISO,
//						.pin_spi_mosi = SPI_PIN_MOSI,
//						.pin_spi_cs = SPI_PIN_CS,
//						.spi_bit_rate = 1000000,
//						.spi_pha = 1,
//						.spi_pol = 0,
//						.spi_slave = 1,
//				};
//
//
//	LOG_DBG("Enabling PRCM power domain periph\n");
//	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);
//	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON);
//
//	LOG_DBG("Enabling PRCM power domain SSI\n");
//	PRCMPeripheralRunEnable(PRCM_PERIPH_SSI0);
//	PRCMPeripheralSleepEnable(PRCM_PERIPH_SSI0);
//	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_SSI0);
//	PRCMLoadSet();
//	while (PRCMLoadGet() == 0) ;
//
//		LOG_DBG("Config SSI\n");
//	IOCPinTypeSsiSlave(SSI0_BASE, spidev.pin_spi_miso, spidev.pin_spi_mosi, spidev.pin_spi_cs, spidev.pin_spi_sck);
//	SSIConfigSetExpClk(SSI0_BASE, SysCtrlClockGet(), get_mode(&spidev), SSI_MODE_SLAVE, spidev.spi_bit_rate, 8);
//	SSIIntDisable(SSI0_BASE, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
//	SSIIntClear(SSI0_BASE, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
//
//
//
//
//	LOG_DBG("Draining SPI\n");
//	spi_drain_rx ();
//
//	LOG_DBG("Enabling DMA\n");
//	dma_init ();
//
//	SSIIntEnable(SSI0_BASE, SSI_RXOR | SSI_RXTO);
//	SSIEnable(SSI0_BASE);
//
//	LOG_DBG("Spi Init Finished.\n");
//}
//
//
//PROCESS_THREAD(spi_rdcmd, ev, data)
//{
//	PROCESS_BEGIN( );
//		static int seq = 0;
//		static int rc = 0;
//		//static int trash = -1;
//
//		while (1) {
//			dma_set_callback (&spi_rdcmd);
//
//			LOG_DBG("spi_rdcmd starting %d\n", ++seq);
//			spi_drain_rx ();
//
//			dma_clear_event_all_flags ();
//
//			rc = dma_xfer (NULL, (void *) &spi_data, 4);
//			if (rc < 0) {
//				LOG_DBG("dma_xfer failed %d\n", seq);
//				continue;
//			}
//			LOG_DBG("dma started, toggle interrupt\n");
//			toggle_spi_intr ();
//
//			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//			LOG_DBG("Received DMA interrupt %x\n", get_intr_reason ());
//
//			if (dma_did_oflow ())
//			{
//				dma_clr_oflow ();
//				LOG_DBG("spi_rdcmd dma xfer oflow detected %x %d\n", get_intr_reason (), seq);
//				spi_drain_rx ();
//				continue;
//			}
//
//			if (dma_did_rxerr ())
//			{
//				dma_clr_rxerr ();
//				LOG_DBG("spi_rdcmd dma rx error detected %d\n", seq);
//				spi_drain_rx ();
//				continue;
//			}
//
//			if (dma_did_txfin ()) {
//				dma_clr_txfin ();
//			}
//
//			if (dma_did_rxfin ()) {
//				LOG_DBG("spi_rdcmd dma rx fin %d\n", seq);
//
//				LOG_DBG("dma data: %x %x %x %x\n", spi_data[0], spi_data[1], spi_data[2], spi_data[3]);
//
//				dma_clr_rxfin ();
//
//				if ((spi_data[0] == 0x10) && (spi_data[3] == 0xff))  {
//					regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];
//
//					LOG_DBG("Starting proc spi_rdreg\n");
//					dma_set_callback (&spi_rdreg);
//					process_start (&spi_rdreg, NULL);
//				}
//				else if ((spi_data[0] == 0x11) && (spi_data[3] == 0xff)) {
//					regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];
//
//					LOG_DBG("Start proc spi wrreg\n");
//					dma_set_callback (&spi_wrreg);
//					process_start (&spi_wrreg, NULL);
//				}
//				else if ((spi_data[0] == 0x20) && (spi_data[3] == 0xff)) {
//					req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];
//					LOG_DBG("Start proc pkt recieve from pi");
//					dma_set_callback(&spi_rcvpkt);
//					process_start(&spi_rcvpkt, NULL);
//				}
//				else if ((spi_data[0] == 0x21) && (spi_data[3] == 0xff)) {
//					req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];
//					if (req_pkt_len != uip_len) {
//						LOG_ERR("Error - packet lengths dont match.");
//						continue;
//					}
//
//					LOG_DBG("Start proc pkt xmit from pi");
//					dma_set_callback(&spi_xmtpkt);
//					process_start(&spi_xmtpkt, NULL);
//				}
//				else {
//					LOG_ERR("Err - unknown command: %x %x %x %x\n", spi_data[0], spi_data[1], spi_data[2], spi_data[3]);
//					spi_drain_rx();
//					continue;
//				}
//				// started the handler, so wait for it to finish
//				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
//			} // end received valid packet
//		} // end while command loop
//
//	PROCESS_END( );
//}
//
//PROCESS_THREAD(spi_rdreg, ev, data)
//{
//	PROCESS_BEGIN( );
//	int trash = 0x55555;
//
//	LOG_DBG("spi_rdreg starting\n");
//
//	uint32_t val = register_read (regnum);
//	LOG_DBG("Sending reg val %x\n", (unsigned) val);
//
//	dma_xfer (&val, (void *)&trash, 4);
//
//	toggle_spi_intr ();
//
//	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//
//	LOG_DBG("spi_rdreg posting \n");
//	process_post (&spi_rdcmd, spi_event, data);
//
//PROCESS_END();
//}
//
//PROCESS_THREAD(spi_wrreg, ev, data)
//{
//	PROCESS_BEGIN( );
//	int trash = 0x5555;
//	LOG_DBG("spi_wrreg starting\n");
//
//	memset((void *) &spi_data, '$', 4);
//	dma_xfer (&trash, (void *) &spi_data, 4);
//	toggle_spi_intr ();
//
//	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//	LOG_DBG("wrreg Received DMA interrupt %x\n", get_intr_reason ());
//
//	uint32_t val = ((uint32_t) spi_data[3]) << 24 |
//			((uint32_t) spi_data[2]) << 16 |
//			((uint32_t) spi_data[1]) << 8 |
//			((uint32_t) spi_data[0]);
//
//	LOG_DBG("Writing register %d = %x\n", regnum, (unsigned int) val);
//	register_write (regnum, val);
//
//	process_post (&spi_rdcmd, spi_event, data);
//
//PROCESS_END();
//}
//
//PROCESS_THREAD(spi_rcvpkt, ev, data)
//{
//	PROCESS_BEGIN( );
//	int trash = 0x55555;
//
//	LOG_DBG("spi_rcvpkt starting\n");
//
//	uip_len = req_pkt_len;
//	dma_xfer ((void *)&trash, &uip_aligned_buf, req_pkt_len);
//
//	toggle_spi_intr ();
//
//	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//
//	LOG_DBG("spi_rcvpkt delivering & posting %d bytes \n", uip_len);
//	tcpip_input();
//
//	process_post (&spi_rdcmd, spi_event, data);
//
//PROCESS_END();
//}
//
//PROCESS_THREAD(spi_xmtpkt, ev, data)
//{
//	PROCESS_BEGIN( );
//	int trash = 0x55555;
//
//	LOG_DBG("spi_xmitpkt starting\n");
//
//	uint32_t val = register_read (regnum);
//	LOG_DBG("Sending reg val %x\n", (unsigned) val);
//
//	dma_xfer (&uip_aligned_buf, (void *)&trash, req_pkt_len);
//
//	toggle_spi_intr ();
//
//	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
//
//	LOG_DBG("spi_xmitpkt delivering & posting \n");
//	uip_len = 0;
//
//	process_post (&spi_rdcmd, spi_event, data);
//
//PROCESS_END();
//}

// called in "interrupt" context, can only process_poll

struct process *spi_process_callback = NULL;

void UserCallbackFxn (SPI_Handle handle, SPI_Transaction *transaction)
{
	if (spi_process_callback != NULL)
		process_poll(spi_process_callback);
}


void spi_init( )
{
	 SPI_init( );

	SPI_Params_init(&spiParams);
	spiParams.bitRate = 1000000;
	spiParams.dataSize = 8;
	spiParams.frameFormat = SPI_POL0_PHA1;
	spiParams.transferMode = SPI_MODE_CALLBACK;
	spiParams.transferCallbackFxn = UserCallbackFxn;
	spiParams.mode = SPI_SLAVE;

	spiHandle = SPI_open(Board_SPI0, &spiParams);
	if (spiHandle == NULL) {
			printf("Error opening spi.\n");
			while(1);
	}

	process_start(&spi_rdcmd, NULL);

}

static void spi_start_xfer(void *mosi, void *miso, int len)
{
	if ((miso == NULL) || (mosi == NULL) || (len == 0)) {
		LOG_ERR("null buffers, zero-len xfer, transfer aborted.");
		if (spi_process_callback != NULL) {
			process_poll(spi_process_callback);
		}
		return;
	}

	spiTrans.count = len;
	spiTrans.txBuf = miso;
	spiTrans.rxBuf = mosi;
	spiTrans.arg = NULL;

	if (!SPI_transfer(spiHandle, &spiTrans)) {
		if (spi_process_callback != NULL) {
					process_poll(spi_process_callback);
		}
	}

	return;
}

PROCESS_THREAD(spi_rdcmd, ev, data)
{
	PROCESS_BEGIN( );
		static int seq = 0;

		while (1) {
			LOG_DBG("Back in rdcmd\n");
			spi_process_callback = &spi_rdcmd;
			memset((void *)&spi_data, 0xff, 4);
			spi_start_xfer(&spi_data, &spi_data, 4);
			toggle_spi_intr();
			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

			LOG_DBG("spi-dma %d finished data: %x %x %x %x\n", seq++, spi_data[0], spi_data[1], spi_data[2], spi_data[3]);

			if ((spi_data[0] == 0x10) && (spi_data[3] == 0xff))  {
				regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];

				LOG_DBG("Starting proc spi_rdreg\n");
				spi_process_callback = &spi_rdreg;
				process_start (&spi_rdreg, NULL);

				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}
			else if ((spi_data[0] == 0x11) && (spi_data[3] == 0xff)) {
				regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];

				LOG_DBG("Start proc spi wrreg\n");
				spi_process_callback = &spi_wrreg;
				process_start (&spi_wrreg, NULL);

				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}
			else if ((spi_data[0] == 0x20) && (spi_data[3] == 0xff)) {
				req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];
				LOG_DBG("Start proc pkt recieve from pi");
				if (req_pkt_len != spi_pkt_data_len) {
					LOG_ERR("spi_pkt_data_len %d != req len %d\n", spi_pkt_data_len, req_pkt_len);
				}

				spi_process_callback = &spi_rcvpkt;
				process_start(&spi_rcvpkt, NULL);
			}
			else if ((spi_data[0] == 0x21) && (spi_data[3] == 0xff)) {
				req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];

				LOG_DBG("Start proc pkt xmit from pi");
					spi_process_callback = &spi_xmtpkt;
					process_start(&spi_xmtpkt, NULL);
				}
			else {
				LOG_ERR("Err - unknown command: %x %x %x %x\n", spi_data[0], spi_data[1], spi_data[2], spi_data[3]);
				continue;
			}
		} // end while loop

	PROCESS_END( );
}


PROCESS_THREAD(spi_rdreg, ev, data)
{
	PROCESS_BEGIN( );

	LOG_DBG("spi_rdreg starting\n");

	uint32_t val = register_read (regnum);
	memcpy(spi_data,  &val,  sizeof(val));

	LOG_DBG("Sending reg val %x\n", (unsigned) val);
	spi_start_xfer(&spi_data, &spi_data, 4);
	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	LOG_DBG("spi_rdreg posting \n");
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

PROCESS_THREAD(spi_wrreg, ev, data)
{
	PROCESS_BEGIN( );

	LOG_DBG("spi_wrreg starting\n");

	memset((void *) &spi_data, '$', 4);
	spi_start_xfer(&spi_data, &spi_data, 4);
	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	uint32_t val = ((uint32_t) spi_data[3]) << 24 |
			((uint32_t) spi_data[2]) << 16 |
			((uint32_t) spi_data[1]) << 8 |
			((uint32_t) spi_data[0]);

	LOG_DBG("Writing register %d = %x\n", regnum, (unsigned int) val);
	register_write (regnum, val);

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}


int spi_get_pktlen( )
{
	return spi_pkt_data_len;
}

#define ETHER2_HEADER_SIZE (14)
static void copy_eth_header( )
{
	const char dest_mac[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
	for (int i = 0; i < 6; i++) {
			spi_pkt_data[i] = dest_mac[i];
	}
	for (int i = 0; i < 6; i++) {
		  spi_pkt_data[i+6] = uip_lladdr.addr[i];
	}
	spi_pkt_data[12] = 0x08;
	spi_pkt_data[13] = 0x00;
}
void copy_uip_to_spi( )
{
	int len = uip_len + ETHER2_HEADER_SIZE;
	spi_pkt_data_len = len;

	if (len == 0) {
		LOG_ERR("zero length, discarding.\n");
		return;
	}

	if (len > sizeof(spi_pkt_data))
	{
		LOG_ERR("spi pkt data cannot handle %d bytes\n", len);
		len = sizeof(spi_pkt_data);
	}

	copy_eth_header( );
  memcpy(spi_pkt_data+ETHER2_HEADER_SIZE, &uip_buf, uip_len);

  for (int i = 0; i < spi_pkt_data_len; i+=16) {
  	for (int j = 0; j < 16; j++) {
  		printf("%-2.2x ", spi_pkt_data[i*16 + j]);
  	}
  	printf("\n");
  }
  printf("\n");
}


PROCESS_THREAD(spi_rcvpkt, ev, data)
{
	PROCESS_BEGIN( );

	LOG_DBG("spi_rcvpkt %d - %x %x %x %x starting\n", req_pkt_len, spi_pkt_data[0], spi_pkt_data[1], spi_pkt_data[2], spi_pkt_data[3] );

	spi_start_xfer(&spi_pkt_data, &spi_pkt_data, req_pkt_len);
	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}


PROCESS_THREAD(spi_xmtpkt, ev, data)
{
	PROCESS_BEGIN( );

	LOG_DBG("spi_xmtpkt %d\n", req_pkt_len);

	spi_start_xfer(&spi_pkt_data, &spi_pkt_data, req_pkt_len);
	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	LOG_DBG("done reading pi data, need to queue %d bytes into hardware tx\n", req_pkt_len);
	memcpy(uip_buf, spi_pkt_data, spi_pkt_data_len);
  uip_len = spi_pkt_data_len;

	printf("tcpip input %d : %x %x %x %x\n", spi_pkt_data_len, uip_buf[0],uip_buf[1],uip_buf[2],uip_buf[3]);
	tcpip_input();

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}
