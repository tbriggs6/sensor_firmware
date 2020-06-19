#include "contiki.h"
#include "dev/spi.h"
#include "sys/log.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/gpio.h)

#include <stdint.h>

#include "spi-iface.h"
#include "spi-dma.h"
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

static volatile char spi_data[256] =	{ 0 };

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
	GPIO_setDio(26);
}

static void clear_spi_intr ()
{
	GPIO_clearDio(26);
}

static void toggle_spi_intr ()
{
	LOG_DBG("Toggle SPI intr\n");
	clear_spi_intr ();
	clock_delay_usec (5);
	raise_spi_intr ();
}

static void spi_drain_rx ()
{
	uint32_t bob;
	uint32_t count = 0;
	while (ROM_SSIDataGetNonBlocking(SSI0_BASE, &bob))
		count++;
	LOG_DBG("SPI drained %d bytes\n", (int ) count);
}


void spi_transfer_callback(void *data, int status, void *txbuf, void *rxbuf)
{
	LOG_INFO("SPI callback!");
}

// Initialize the SPI bus.
void spi_init ()
{

	spi_event = process_alloc_event ();

	//TODO this should have been set in the platform?
	IOCPinTypeGpioInput (1);   // chip select from PI
	GPIO_setOutputEnableDio (26, GPIO_OUTPUT_ENABLE);
	GPIO_clearDio (26);

	IOCPinTypeGpioOutput (26);   // chip select from PI

	spi_device_t spidev =
				{
						.spi_controller = SPI_CONTROLLER,
						.pin_spi_sck = SPI_PIN_SCK,
						.pin_spi_miso = SPI_PIN_MISO,
						.pin_spi_mosi = SPI_PIN_MOSI,
						.pin_spi_cs = SPI_PIN_CS,
						.spi_bit_rate = 1000000,
						.spi_pha = 1,
						.spi_pol = 0,
						.spi_slave = 1,
						.spi_slave_data = NULL,
						.spi_callback = spi_transfer_callback,
				};

	if (spi_acquire (&spidev) != SPI_DEV_STATUS_OK)
	{
		LOG_ERR("Could not aquire SPI device.\n");
	}
	else {
		LOG_DBG("spi init OK\n");
	}

	spi_drain_rx ();
	dma_init ();
}


PROCESS_THREAD(spi_rdcmd, ev, data)
{
	PROCESS_BEGIN( );
		static int seq = 0;
		static int rc = 0;

		while (1) {
			LOG_DBG("spi_rdcmd starting %d\n", ++seq);
			spi_drain_rx ();

			dma_clear_event_all_flags ();
			dma_set_callback (&spi_rdcmd);
			rc = dma_xfer (NULL, (void *) &spi_data, 4);
			if (rc < 0) {
				LOG_DBG("dma_xfer failed %d\n", seq);
				continue;
			}
			LOG_DBG("dma started, toggle interrupt\n");
			toggle_spi_intr ();

			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
			LOG_DBG("Received DMA interrupt %x\n", get_intr_reason ());

			if (dma_did_oflow ())
			{
				dma_clr_oflow ();
				LOG_DBG("spi_rdcmd dma xfer oflow detected %x %d\n", get_intr_reason (), seq);
				spi_drain_rx ();
				continue;
			}

			if (dma_did_rxerr ())
			{
				dma_clr_rxerr ();
				LOG_DBG("spi_rdcmd dma rx error detected %d\n", seq);
				spi_drain_rx ();
				continue;
			}

			if (dma_did_txfin ()) {
				dma_clr_txfin ();
			}

			if (dma_did_rxfin ()) {
				LOG_DBG("spi_rdcmd dma rx fin %d\n", seq);

				LOG_DBG("dma data: %x %x %x %x\n", spi_data[0], spi_data[1], spi_data[2], spi_data[3]);

				dma_clr_rxfin ();

				if ((spi_data[0] == 0x10) && (spi_data[3] == 0xff))  {
					regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];

					LOG_DBG("Starting proc spi_rdreg\n");
					dma_set_callback (&spi_rdreg);
					process_start (&spi_rdreg, NULL);
				}
				else if ((spi_data[0] == 0x11) && (spi_data[3] == 0xff)) {
					regnum = ((uint16_t) spi_data[1] << 8) | spi_data[2];

					LOG_DBG("Start proc spi wrreg\n");
					dma_set_callback (&spi_wrreg);
					process_start (&spi_wrreg, NULL);
				}
				else if ((spi_data[0] == 0x20) && (spi_data[3] == 0xff)) {
					req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];
					LOG_DBG("Start proc pkt recieve from pi");
					dma_set_callback(&spi_rcvpkt);
					process_start(&spi_rcvpkt, NULL);
				}
				else if ((spi_data[0] == 0x21) && (spi_data[3] == 0xff)) {
					req_pkt_len = ((uint16_t) spi_data[1] << 8) | spi_data[2];
					if (req_pkt_len != uip_len) {
						LOG_ERR("Error - packet lengths dont match.");
						continue;
					}

					LOG_DBG("Start proc pkt xmit from pi");
					dma_set_callback(&spi_xmtpkt);
					process_start(&spi_xmtpkt, NULL);
				}
				else {
					LOG_ERR("Err - unknown command: %x %x %x %x\n", spi_data[0], spi_data[1], spi_data[2], spi_data[3]);
					spi_drain_rx();
					continue;
				}
				// started the handler, so wait for it to finish
				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			} // end received valid packet
		} // end while command loop

	PROCESS_END( );
}

PROCESS_THREAD(spi_rdreg, ev, data)
{
	PROCESS_BEGIN( );
	int trash = 0x55555;

	LOG_DBG("spi_rdreg starting\n");

	uint32_t val = register_read (regnum);
	LOG_DBG("Sending reg val %x\n", (unsigned) val);

	dma_xfer (&val, (void *)&trash, 4);

	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	LOG_DBG("spi_rdreg posting \n");
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

PROCESS_THREAD(spi_wrreg, ev, data)
{
	PROCESS_BEGIN( );
	int trash = 0x5555;
	LOG_DBG("spi_wrreg starting\n");

	memset((void *) &spi_data, '$', 4);
	dma_xfer (&trash, (void *) &spi_data, 4);
	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
	LOG_DBG("wrreg Received DMA interrupt %x\n", get_intr_reason ());

	uint32_t val = ((uint32_t) spi_data[3]) << 24 |
			((uint32_t) spi_data[2]) << 16 |
			((uint32_t) spi_data[1]) << 8 |
			((uint32_t) spi_data[0]);

	LOG_DBG("Writing register %d = %x\n", regnum, (unsigned int) val);
	register_write (regnum, val);

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

PROCESS_THREAD(spi_rcvpkt, ev, data)
{
	PROCESS_BEGIN( );
	int trash = 0x55555;

	LOG_DBG("spi_rcvpkt starting\n");

	uip_len = req_pkt_len;
	dma_xfer ((void *)&trash, &uip_aligned_buf, req_pkt_len);

	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	LOG_DBG("spi_rcvpkt delivering & posting %d bytes \n", uip_len);
	tcpip_input();

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

PROCESS_THREAD(spi_xmtpkt, ev, data)
{
	PROCESS_BEGIN( );
	int trash = 0x55555;

	LOG_DBG("spi_xmitpkt starting\n");

	uint32_t val = register_read (regnum);
	LOG_DBG("Sending reg val %x\n", (unsigned) val);

	dma_xfer (&uip_aligned_buf, (void *)&trash, req_pkt_len);

	toggle_spi_intr ();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

	LOG_DBG("spi_xmitpkt delivering & posting \n");
	uip_len = 0;

	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}
