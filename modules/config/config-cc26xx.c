/*
 * config.c
 *
 *  Created on: Jun 29, 2016
 *      Author: user
 */

#include <contiki.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <contiki-net.h>
#include <sys/compower.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/flash.h)
#include DeviceFamily_constructPath(driverlib/vims.h)

#include "../modules/config/config.h"

// contiki-ism for logging the data -
#include "sys/log.h"
#define LOG_MODULE "CONFIG"
#define LOG_LEVEL LOG_LEVEL_DBG

#ifdef DEBUG
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DBG
#endif


extern void * _uflash_base;
extern void * _uflash_size;

extern config_t config;

void config_read()
{
//	// Copy the ram stored configs into the config struct
//	memcpy(&config, (config_t *) &_uflash_base, sizeof(config));
//
//	// show config bytes
//	uint8_t *val = (uint8_t *) &_uflash_base;
//	int i;
//	for (i = 0; i < sizeof(config); i++) {
//		printf("%-2.2x ", val[i]);
//	}
//	printf("\n");
//
//	for (i = 0; i < 8; i++) {
//		printf("%d : %-4.4x\n", i, config.server[i]);
//	}

}

void config_write()
{
//	uint32_t mybase = (uint32_t) &_uflash_base;
//	//uint32_t mysize = (uint32_t) &_uflash_size;
//
//	if(FlashPowerModeGet() != FLASH_PWR_ACTIVE_MODE){
//		FlashPowerModeSet (FLASH_PWR_ACTIVE_MODE, 1024 * 1024, 1024 * 1024);
//	}
//
//	// Disable and flush the cache so that the old memory isn't there
//	VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
//	while(VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);
//	while(FlashCheckFsmForError() != FAPI_STATUS_SUCCESS);
//	while(FlashCheckFsmForReady() != FAPI_STATUS_FSM_READY);
//
//	/* Disable all interrupts when accessing flash, prevents memory from getting hosed */
//	CPUcpsid();
//
//	/* Erase the flash -- required in order to write */
//	uint32_t resp = FlashSectorErase(mybase);
//	if(resp == FAPI_STATUS_SUCCESS){
//		LOG_DBG("[CONFIG WRITE] Flash erasure successful...\n");
//	} else {
//		LOG_DBG("[CONFIG WRITE] Flash erasure unsuccessful...\r\n");
//	}
//
//	resp = FlashProgram((uint8_t *) &config, mybase, sizeof(config_t));
//	if(resp == FAPI_STATUS_SUCCESS){
//		LOG_DBG("[CONFIG WRITE] Flash program successful...\r\n");
//	} else {
//		LOG_DBG("[CONFIG WRITE] Flash program unsuccessful...\r\n");
//	}
//
//	/* Reenable interupts */
//	CPUcpsie();
//
//	/* Re-enable the cache */
//	VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
}

