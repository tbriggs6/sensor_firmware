#include "contiki.h"
#include "dev/spi.h"
#include "sys/log.h"
#include "sys/mutex.h"
#include "dev/leds.h"

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
#include DeviceFamily_constructPath(driverlib/udma.h)
#include <ti/drivers/Board.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "Board.h"

#include <stdint.h>

#include "spi-iface.h"
#include "spi-regs.h"
#include "print_buff.h"

#define LOG_MODULE "SPINet"
#define LOG_LEVEL LOG_LEVEL_ERR

#define SPI_CONTROLLER    SPI_CONTROLLER_SPI0

#define SPI_PIN_SCK       30
#define SPI_PIN_MOSI      29
#define SPI_PIN_MISO      28
#define SPI_PIN_CS        1

#define SPI_TIMEOUT (CLOCK_SECOND*2)

static SPI_Handle spiHandle;
static SPI_Params spiParams;
static SPI_Transaction spiTrans;

static char spi_cmd[4] =	{ 0 };
static char spi_topi[512];
static char spi_frompi[512];

static int spi_topi_len = 0;
static int spi_topi_actual = 0;

static int spi_frompi_len = 0;

static volatile uint16_t regnum = 0;



/**
 * Start a transfer on the SPI bus for 4 bytes, and wait
 * for it to signal completion.
 */
PROCESS(spi_rdcmd, "SPI read command");
PROCESS(spi_rdreg, "SPI read reg");
PROCESS(spi_wrreg, "SPI write reg");
PROCESS(spi_proc_frompi, "SPI from PI");
PROCESS(spi_proc_topi, "SPI to PI");
PROCESS(spi_proc_tcpsender, "send to UIP");
PROCESS(spi_proc_dmafin, "dma fin");
PROCESS(spi_cancel_process, "spi timeout");

process_event_t spi_event;
process_event_t spi_send_event;
process_event_t spi_dma_done_event;
process_event_t spi_dma_cancel_event;
process_event_t spi_dma_start_event;
mutex_t mutex;


clock_time_t last_clock_time = 0;

static int last_spi_intr = 0;

static void raise_spi_intr ()
{
	last_spi_intr = 1;
	GPIO_setDio(CC1310_LAUNCHXL_PIN_SPIRDY);
	GPIO_setDio(CC1310_LAUNCHXL_PIN_RLED);
}

static void clear_spi_intr ()
{
	last_spi_intr = 0;
	GPIO_clearDio(CC1310_LAUNCHXL_PIN_SPIRDY);
	GPIO_clearDio(CC1310_LAUNCHXL_PIN_RLED);
}

static void toggle_spi_intr ()
{

	raise_spi_intr();
	clock_delay_usec (1000);
	clear_spi_intr();

	//DORK LOG_DBG("toggle SPI intr\n");

}


// spi-dma is finished.
struct process *spi_process_callback = NULL;

void UserCallbackFxn (SPI_Handle handle, SPI_Transaction *transaction)
{
	process_poll(&spi_proc_dmafin);
}


void spi_init( )
{

	 SPI_init( );

	 clear_spi_intr( );

	 last_clock_time = clock_time();

	 LOG_DBG("Spi init, setting DMA / Callback\n");
	SPI_Params_init(&spiParams);
	spiParams.bitRate = 1000000;
	spiParams.dataSize = 8;
	spiParams.frameFormat = SPI_POL0_PHA1;
	spiParams.transferMode = SPI_MODE_CALLBACK;
	spiParams.transferCallbackFxn = UserCallbackFxn;
	spiParams.mode = SPI_SLAVE;

	spiHandle = SPI_open(Board_SPI0, &spiParams);
	if (spiHandle == NULL) {
			LOG_ERR("Error opening the SPI bus.\n");
			while(1);
	}

	spi_event = process_alloc_event();
	spi_send_event = process_alloc_event();
	spi_dma_done_event = process_alloc_event();
	spi_dma_cancel_event = process_alloc_event();
	spi_dma_start_event = process_alloc_event();

	mutex = MUTEX_STATUS_UNLOCKED;

	process_start(&spi_cancel_process, NULL);
	process_start(&spi_proc_dmafin, NULL);
	process_start(&spi_proc_tcpsender, NULL);
	process_start(&spi_rdcmd, NULL);

}

int dma_in_progress( )
{
  int state = (uDMAGetStatus(UDMA0_BASE) >> 4) & 0x0f;
  if ((state == 0x00) || (state == 0x08)) return 0;
  else return 1;
}

/**
 * \brief wait for DMA hardware to signal its started
 */
int wait_for_dma_start( )
{
	int count = 0;
	while (count++ < 1500) {
		if (dma_in_progress() == 1)
			return 1;
		}

	LOG_ERR("err - timeout waiting for DMA to register as ready.\n");

	return 0;
}


/**
 * \brief start a SPI transaction, sending MISO, receiving MOSI, for len bytes
 * poll the spi_callback function when finished
 */
static void spi_start_xfer(void *mosi, void *miso, int len)
{
	int rc;

	if ((miso == NULL) || (mosi == NULL) || (len == 0)) {
		LOG_ERR("null buffers, zero-len xfer, transfer aborted.");
		if (spi_process_callback != NULL) {
			process_post(spi_process_callback, spi_dma_cancel_event, NULL);
		}
		return;
	}

	spiTrans.count = len;
	spiTrans.txBuf = miso;
	spiTrans.rxBuf = mosi;
	spiTrans.arg = NULL;
	//DORK LOG_DBG("+ %u spi_start_xfer %d\n", (unsigned) (clock_time( ) - last_clock_time), len);


	rc = SPI_transfer(spiHandle, &spiTrans);

	if (rc == 0) {	// failed
		LOG_ERR("SPI transfer err\n");
		return;
	}

	clock_delay_usec(1000);

	process_post(&spi_cancel_process, spi_dma_start_event, NULL);

	// alert PI that we're ready to exchange data
	toggle_spi_intr();

//		wait_for_dma_start( );
	return;
}

void spi_drain_rx( )
{
  uint32_t bob;
  uint32_t count = 0;

  while (SSIDataGetNonBlocking(SSI0_BASE, &bob)) count++;

}

/**
 * \brief Read a SPI command and dispatch to the appropriate handler
 */
PROCESS_THREAD(spi_rdcmd, ev, data)
{
	PROCESS_BEGIN( );

	while (1) {

			// set spi transfer "notification" process
			spi_process_callback = &spi_rdcmd;

			memset((void *)&spi_cmd, 0xff, 4);

			// flush the rx path
			//spi_drain_rx( );

			// start DMA transfer
			spi_start_xfer(&spi_cmd, &spi_cmd, 4);


			// wait for the SPI/DMA transfer to signal us
			PROCESS_WAIT_EVENT_UNTIL((ev == spi_dma_done_event) || (ev == spi_dma_cancel_event));
			if (ev == spi_dma_cancel_event) continue;

//DORK			LOG_DBG("+ %u dma_done\n", (unsigned) (clock_time( ) - last_clock_time));

			// the four byte command is ready...
//DORK			LOG_DBG("+ %u rdcmd payload: %x %x %x %x\n", (unsigned) (clock_time( ) - last_clock_time), spi_cmd[0],spi_cmd[1],spi_cmd[2],spi_cmd[3]);

			// Command 0x10 is the "read a register" command
			// Payload is:  0x10 REG_HI  REG_LO 0xff
			if ((spi_cmd[0] == 0x10) && (spi_cmd[3] == 0xff))  {
				regnum = ((uint16_t) spi_cmd[1] << 8) | spi_cmd[2];

				// set the spi transfer notification
				spi_process_callback = &spi_rdreg;

				// start the read register process
				process_start (&spi_rdreg, NULL);

				// when its done, it will signal us
				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}

			// Command 0x11 is the "write register" command
			// Payload is: 0x11 REG_HI REG_LOW 0xff
			else if ((spi_cmd[0] == 0x11) && (spi_cmd[3] == 0xff)) {
				regnum = ((uint16_t) spi_cmd[1] << 8) | spi_cmd[2];

				// set spi transfer notification
				spi_process_callback = &spi_wrreg;

				// start the write register process
				process_start (&spi_wrreg, NULL);

				// it will signal us when its done.
				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}


			// Command 0x20 is the "read packet from PI, send over radio" command
			// Payload is: 0x20 LEN_HI LEN_LOW 0xFF
			else if ((spi_cmd[0] == 0x20) && (spi_cmd[3] == 0xff)) {
				spi_frompi_len = ((uint16_t) spi_cmd[1] << 8) | spi_cmd[2];

				// set the transfer notification
				spi_process_callback = &spi_proc_frompi;

				// start the handler
				process_start(&spi_proc_frompi, NULL);

				// process will signal us when its done
				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}


			// This is triggered by us raising the RX interrupt
			// Command 0x21 is the "write packet to PI, send over LAN
			// Payload is: 0x21 LEN_HI LEN_LOW 0xFF
			else if ((spi_cmd[0] == 0x21) && (spi_cmd[3] == 0xff)) {
				spi_topi_len = ((uint16_t) spi_cmd[1] << 8) | spi_cmd[2];

				spi_process_callback = &spi_proc_topi;
				process_start(&spi_proc_topi, NULL);
				PROCESS_WAIT_EVENT_UNTIL(ev == spi_event);
			}
			else {
				LOG_ERR("Err - unknown command: %x %x %x %x\n", spi_cmd[0], spi_cmd[1], spi_cmd[2], spi_cmd[3]);
				continue;
			}
		} // end while loop

	PROCESS_END( );
}

/**
 * \brief handle transmitting the requested register
 */

PROCESS_THREAD(spi_rdreg, ev, data)
{
	PROCESS_BEGIN( );

	uint32_t val = register_read (regnum);
	memcpy(spi_cmd,  &val,  sizeof(val));

	// start the SPI transfer (idle until master starts its side)
	spi_start_xfer(&spi_cmd, &spi_cmd, 4);

	// wait for the signal that the transfer is complete
	PROCESS_WAIT_EVENT_UNTIL((ev == spi_dma_done_event) || (ev == spi_dma_cancel_event));

	// notify the read command process
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

/**
 * \brief handle write a register
 */
PROCESS_THREAD(spi_wrreg, ev, data)
{
	PROCESS_BEGIN( );

	// clear out the command buffer
	memset((void *) &spi_cmd, 0xff, 4);

	// start the transfer
	spi_start_xfer(&spi_cmd, &spi_cmd, 4);

	// wait for the signal that the transfer is complete
	PROCESS_WAIT_EVENT_UNTIL((ev == spi_dma_done_event) || (ev == spi_dma_cancel_event));


	// the new reg value is in the SPI data
	uint32_t val = ((uint32_t) spi_cmd[3]) << 24 |
			((uint32_t) spi_cmd[2]) << 16 |
			((uint32_t) spi_cmd[1]) << 8 |
			((uint32_t) spi_cmd[0]);

	// actuall write the register value
	register_write (regnum, val);

	// notify the read command process to resume
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}


int from_pi_pktlen( )
{
	return spi_frompi_len;
}

int to_pi_pktlen( )
{
	return spi_topi_actual;
}

#define ETHER2_HEADER_SIZE (14)
static void copy_eth_header(char *buff)
{
	const char dest_mac[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
	for (int i = 0; i < 6; i++) {
			buff[i] = dest_mac[i];
	}
	for (int i = 0; i < 6; i++) {
		  buff[i+6] = uip_lladdr.addr[i];
	}
	buff[12] = 0x08;
	buff[13] = 0x00;
}

/**
 * \brief copy from the global UIP buffer (from radio) into the topi buffer
 */
void copy_uip_to_spi( )
{
	int len = uip_len + ETHER2_HEADER_SIZE;

	if (len == 0) {
		LOG_ERR("zero length, discarding.\n");
		return;
	}
	if (len > sizeof(spi_topi)) {
		LOG_ERR("copy_uip_to_spi: radio data larger than to_pi buffer.");
		return;
	}


	copy_eth_header(spi_topi);
  memcpy(spi_topi+ETHER2_HEADER_SIZE, &uip_buf, uip_len);
  spi_topi_actual = uip_len + ETHER2_HEADER_SIZE;

  LOG_DBG("copying %d bytes to spi_topi\n", spi_topi_actual);
}


/** \brief handle packets sent to the PI/linux, **rcvd** from radio
 *  Typical pattern is:
 *  1. packet arrives in the radio, contiki builds uip buffer
 *  2. eventually, "rpl_output()" handler issues RX interrupt to PI
 *  3. pi schedules a radio receive
 *  4. pi sends 0x10 0x00 0x11 0xff - command to read received data length
 *  5. pi sends 0x21 LEN_HI LEN_LO 0xff - command to request that number of bytes
 *  6. pi sends SPI command with that number of bytes
 *  7. pi sends 0x11 0x00 0x11 0xff / 0x00 0x00 0x00 0x00 to write the received length to zero
 */
PROCESS_THREAD(spi_proc_topi, ev, data)
{
	PROCESS_BEGIN( );

	// start the transfer
	spi_start_xfer(&spi_topi, &spi_topi, spi_topi_len);

	// wait for the transfer to finish
	PROCESS_WAIT_EVENT_UNTIL((ev == spi_dma_done_event) || (ev == spi_dma_cancel_event));


	// received data has been sent to the PI for processing

	// return to the "rd command"
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}


/** \brief handle packets from the PI/linux to the CC1310, **xmit** on radio
 * Typical pattern is:
 * 1. pi has a packet from net stack, queues up a packet request
 * 2. sends SPI command "0x20 LEN_HI LEN_LO 0xff" to start the transfer
 * 3. sends that number of bytes of data
 * 4. packet is then delivered to the radio stack for processing
 */
PROCESS_THREAD(spi_proc_frompi, ev, data)
{
	PROCESS_BEGIN( );

	LOG_DBG("+ %u spi_proc_frompi\n", (unsigned) (clock_time( ) - last_clock_time));


	// start the DMA
	spi_start_xfer(&spi_frompi, &spi_frompi, spi_frompi_len);

	// wait until signalled
	PROCESS_WAIT_EVENT_UNTIL((ev == spi_dma_done_event) || (ev == spi_dma_cancel_event));
	if (ev == spi_dma_cancel_event) {
		LOG_ERR("canceled.");
	}
	else {
		LOG_DBG("+ %u spi_proc_frompi polled \n", (unsigned) (clock_time( ) - last_clock_time));


		// inject packet into tcp stack
		process_post(&spi_proc_tcpsender, spi_send_event, spi_frompi);

	}
	// return to reading commands
	process_post (&spi_rdcmd, spi_event, data);

PROCESS_END();
}

PROCESS_THREAD(spi_proc_tcpsender, ev, data)
{
	PROCESS_BEGIN( );

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == spi_send_event);

		if (!mutex_try_lock(&mutex)) {
			LOG_ERR("mutex is still busy\n");
			register_increment(REG_TXCOLL);

		}
		register_write(REG_TXBUSY, 1);
//		LOG_DBG("spi from pi %d\n", spi_frompi_len);
//		print_hex_dump(LOG_LEVEL_DBG, "spi from pi", 16, spi_frompi, spi_frompi_len);

		memcpy(uip_buf, spi_frompi, spi_frompi_len);
		uip_len = spi_frompi_len;

		tcpip_input();

		LOG_DBG("+ %u tcpip_input complete\n", (unsigned) (clock_time( ) - last_clock_time));
		register_write(REG_TXBUSY, 0);
		register_increment(REG_TXOK);
		mutex_unlock(&mutex);
	}

	PROCESS_END();
}

static int spi_cancelled = 0;

PROCESS_THREAD(spi_proc_dmafin, ev, data)
{
	PROCESS_BEGIN();
	while(1) {
		PROCESS_WAIT_EVENT();
		if (ev == PROCESS_EVENT_POLL) {
			// disable cancelation timer
			process_post(&spi_cancel_process, spi_dma_done_event, NULL);

			GPIO_toggleDio(11);

				if (spi_process_callback != NULL) {
//DORK					LOG_DBG("posting to %p\n", spi_process_callback);
					if (spi_cancelled)
						process_post(spi_process_callback, spi_dma_cancel_event, NULL);
					else
						process_post(spi_process_callback, spi_dma_done_event, NULL);
				}
				else {
					LOG_DBG("no process to post\n");
				}
		}
	}
	PROCESS_END();
}

/**
 * \brief when the timer expires, cancel the stuck transaction
 */
PROCESS_THREAD(spi_cancel_process, ev, data)
{
	static struct etimer spi_timer;

	PROCESS_BEGIN();
	while(1) {
		PROCESS_WAIT_EVENT();
		if (ev == spi_dma_start_event) {
			etimer_set(&spi_timer, SPI_TIMEOUT);
			spi_cancelled = 0;
		}
		else if (ev == spi_dma_done_event) {
			etimer_stop(&spi_timer);
			spi_cancelled = 0;
		}
		else if (ev == PROCESS_EVENT_TIMER) {
			spi_cancelled = 1;
  		SPI_transferCancel(spiHandle);
  		spi_drain_rx();
		}

	}
	PROCESS_END();
}
