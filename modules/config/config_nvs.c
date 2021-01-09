#include <contiki.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <spiffs.h>
#include <SPIFFSNVS.h>

#include "Board.h"
#include "config.h"

#include <shell.h>
#include <shell-commands.h>

#include <sys/log.h>
#define LOG_MODULE "CONFIG"
#define LOG_LEVEL LOG_LEVEL_DBG

/* SPIFFS configuration parameters */
#define SPIFFS_LOGICAL_BLOCK_SIZE    (4096)
#define SPIFFS_LOGICAL_PAGE_SIZE     (256)
#define SPIFFS_FILE_DESCRIPTOR_SIZE  (44)

static uint8_t spiffsWorkBuffer[SPIFFS_LOGICAL_PAGE_SIZE * 2];
static uint8_t spiffsFileDescriptorCache[SPIFFS_FILE_DESCRIPTOR_SIZE * 4];
static uint8_t spiffsReadWriteCache[SPIFFS_LOGICAL_PAGE_SIZE * 2];

/* File system object */
static spiffs fs;

/* SPIFFS runtime configuration object */
static spiffs_config fsConfig;

/* Binding to NVS driver */
static SPIFFSNVS_Data spiffsnvsData;

extern config_t config;

/*---------------------------------------------------------------------------*/
static PT_THREAD(cmd_format(struct pt *pt, shell_output_func output, char *args))
{
	PT_BEGIN(pt);

	SHELL_OUTPUT(output, "Formatting");

	SPIFFS_unmount(&fs);

	int status = SPIFFS_format(&fs);
	if (status != SPIFFS_OK) {
		SHELL_OUTPUT(output, "Error - could not format the SPIFFS, file system not initialized & not mounted..\n");
		PT_EXIT(pt);
	}


	status = SPIFFS_mount( &fs,
													 &fsConfig,
													 spiffsWorkBuffer,
													 spiffsFileDescriptorCache,
													 sizeof(spiffsFileDescriptorCache),
													 spiffsReadWriteCache,
													 sizeof(spiffsReadWriteCache),
													 NULL);


	SHELL_OUTPUT(output,"Format complete.\n");
	PT_END(pt);
}

static PT_THREAD(cmd_list(struct pt *pt, shell_output_func output, char *args))
{
	spiffs_DIR dir;
	struct spiffs_dirent ent;

	PT_BEGIN(pt);

	if (SPIFFS_opendir(&fs, "/", &dir) == NULL) {
		LOG_ERR("could not open directory list.");
		PT_EXIT(pt);
	}

	SHELL_OUTPUT(output,"---- DIR ----\n");
	while(1) {
		if (SPIFFS_readdir(&dir, &ent) == NULL)
			break;

		SHELL_OUTPUT(output,"%5.5d %s\n", (int) ent.size, ent.name);
	}
	SHELL_OUTPUT(output,"-------------\n");

	PT_END(pt);
}

static PT_THREAD(cmd_remove(struct pt *pt, shell_output_func output, char *args))
{
	char *next_args;

	PT_BEGIN(pt);

	SHELL_ARGS_INIT(args, next_args);
	if (args == NULL) {
		SHELL_OUTPUT(output,"Error - need to specify filename to remove.");
		PT_EXIT(pt);
	}

	SHELL_ARGS_NEXT(args, next_args);
	while (args != NULL) {
		if (SPIFFS_remove(&fs,args) != SPIFFS_OK) {
			SHELL_OUTPUT(output,"could not remove %s\n", args);
		}
		else {
			SHELL_OUTPUT(output,"removed %s\n", args);
		}
		SHELL_ARGS_NEXT(args, next_args);
	}

	PT_END(pt);
}



static PT_THREAD(cmd_save(struct pt *pt, shell_output_func output, char *args))
{
	PT_BEGIN(pt);

	config_write(&config);
	SHELL_OUTPUT(output,"Configuration saved.\n");

	PT_END(pt);
}


static PT_THREAD(cmd_set(struct pt *pt, shell_output_func output, char *args))
{
	char *next_args;
	int token = -1;
	int value = -1;

	PT_BEGIN(pt);

	SHELL_ARGS_INIT(args, next_args);
	SHELL_ARGS_NEXT(args, next_args);
		if (args == NULL) {
		SHELL_OUTPUT(output,"Error - need config token id (see config.h) .");
		PT_EXIT(pt);
	}
	token = atoi(args);
	SHELL_ARGS_NEXT(args, next_args);

	if (args == NULL) {
			SHELL_OUTPUT(output,"Error - need configuration value .");
			PT_EXIT(pt);
		}

	value = atoi(args);

	config_set(token, value);

	int val = config_get(token);

	SHELL_OUTPUT(output, "-----------------------\n");
	SHELL_OUTPUT(output, "Val[%d]= %-8.8x : %d\n", token, val, val);

	SHELL_OUTPUT(output, "-----------------------\n");


	PT_END(pt);
}

static PT_THREAD(cmd_get(struct pt *pt, shell_output_func output, char *args))
{
	char *next_args;
	int token = -1;

	PT_BEGIN(pt);

	SHELL_ARGS_INIT(args, next_args);
	SHELL_ARGS_NEXT(args, next_args);
	if (args == NULL) {
		SHELL_OUTPUT(output,"Error - need config token id (see config.h) .");
		PT_EXIT(pt);
	}

	token = atoi(args);

	int val = config_get(token);

	SHELL_OUTPUT(output, "-----------------------\n");
	SHELL_OUTPUT(output, "Val[%d]= %-8.8x : %d\n", token, val, val);
	SHELL_OUTPUT(output, "-----------------------\n");

	SHELL_ARGS_NEXT(args, next_args);

	PT_END(pt);
}


static PT_THREAD(cmd_ids(struct pt *pt, shell_output_func output, char *args))
{

	PT_BEGIN(pt);

	SHELL_OUTPUT(output,"CONFIG_CTIME = 10\n");

	SHELL_OUTPUT(output,"CONFIG_DEVTYPE = 12\n");

	SHELL_OUTPUT(output,"CONFIG_SENSOR_INTERVAL = 32\n");
  SHELL_OUTPUT(output,"CONFIG_MAX_FAILURES = 33\n");
  SHELL_OUTPUT(output,"CONFIG_RETRY_INTERVAL = 34\n");

		// device specific calibration values
	SHELL_OUTPUT(output,"CONFIG_CAL1 = 64\n");
	SHELL_OUTPUT(output,"CONFIG_CAL2 = 65\n");
	SHELL_OUTPUT(output,"CONFIG_CAL3 = 66\n");
	SHELL_OUTPUT(output,"CONFIG_CAL4 = 67\n");
	SHELL_OUTPUT(output,"CONFIG_CAL5 = 68\n");
	SHELL_OUTPUT(output,"CONFIG_CAL6 = 69\n");
	SHELL_OUTPUT(output,"CONFIG_CAL7 = 70\n");
	SHELL_OUTPUT(output,"CONFIG_CAL8 = 71\n");

	PT_END(pt);
}





struct shell_command_t config_commands[] = {
		{ "format", cmd_format, "'> format' : format external flash" },
		{ "list", cmd_list, "'> list' : list files in external flash" },
		{ "rm", cmd_remove, "'> rm file' : remove filename from flash" },
		{ "save", cmd_save, "'> save' : save configuration" },
		{ "set", cmd_set, "'>set id val' : set config value (no save)" },
		{ "get", cmd_get, "'>get id' : get config value (no save)" },
		{ "ids", cmd_ids, "'> ids' : list tokens\n" },
		{ NULL, NULL, NULL }
};

struct shell_command_set_t config_command_set = {
		.next = NULL,
		.commands = config_commands
};


int nvs_init( )
{
	/* Initialize spiffs, spiffs_config & spiffsnvsdata structures */
	int status = SPIFFSNVS_config( &spiffsnvsData,
	                           Board_NVSEXTERNAL,
	                           &fs,
	                           &fsConfig,
	                           SPIFFS_LOGICAL_BLOCK_SIZE,
	                           SPIFFS_LOGICAL_PAGE_SIZE );

	if (status != SPIFFSNVS_STATUS_SUCCESS) {
		LOG_ERR("Could not initialize SPIFFS with NVS config.");

	}


	/* Mount the SPIFFS file system */
	status = SPIFFS_mount( &fs,
	                       &fsConfig,
	                       spiffsWorkBuffer,
	                       spiffsFileDescriptorCache,
	                       sizeof(spiffsFileDescriptorCache),
	                       spiffsReadWriteCache,
	                       sizeof(spiffsReadWriteCache),
	                       NULL);


	if (status != SPIFFSNVS_STATUS_SUCCESS) {
		LOG_INFO("Could not mount the file system... trying to format it.\n");

		SPIFFS_unmount(&fs);

		status = SPIFFS_format(&fs);
		if (status != SPIFFS_OK) {
			LOG_ERR("Error - could not format the SPIFFS, file system not initialized.\n");
			return status;
		}


		status = SPIFFS_mount( &fs,
			                       &fsConfig,
			                       spiffsWorkBuffer,
			                       spiffsFileDescriptorCache,
			                       sizeof(spiffsFileDescriptorCache),
			                       spiffsReadWriteCache,
			                       sizeof(spiffsReadWriteCache),
			                       NULL);

		if (status != SPIFFS_OK) {
					LOG_ERR("Error - could not mount file system even after format..\n");
					return status;
				}

	}


	shell_command_set_register(&config_command_set);

	return status;
}

int config_read(config_t *config)
{

		int fd = SPIFFS_open(&fs, "config.dat", SPIFFS_RDONLY, 0);
		if (fd < 0) {
			LOG_ERR("Error - could not open config.dat for writing.\n");
			return -1;
		}

		int rc = SPIFFS_read(&fs, fd, config, sizeof(config_t));
		if (rc < 0) {
			LOG_ERR("Error could not write config data.\n");
			return rc;
		}

		SPIFFS_close(&fs, fd);

		return rc;
}



int  config_write(config_t *config)
{

	// create / overwrite config file
	int fd = SPIFFS_open(&fs, "config.dat", SPIFFS_CREAT | SPIFFS_RDWR, 0);
	if (fd < 0) {
		LOG_ERR("Error - could not open config.dat for writing.\n");
		return -1;
	}

	int rc = SPIFFS_write(&fs, fd, config, sizeof(config_t));
	if (rc < 0) {
		LOG_ERR("Error could not write config data.\n");
		return -1;
	}

	SPIFFS_close(&fs, fd);

	return rc;


}


void config_list( )
{
	spiffs_DIR dir;
	struct spiffs_dirent ent;


	if (SPIFFS_opendir(&fs, "/", &dir) == NULL) {
		LOG_ERR("could not open directory list.");
		return;
	}

	LOG_INFO("---- DIR ----\n");
	while(1) {
		if (SPIFFS_readdir(&dir, &ent) == NULL)
			break;

		LOG_INFO("%5.5d %s\n", (int) ent.size, ent.name);
	}
	LOG_INFO("-------------\n");
}

