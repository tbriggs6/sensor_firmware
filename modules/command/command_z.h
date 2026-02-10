/*
 * Command handler - Zephyr implementation.
 * Parses command_set_t and produces command_ret_t using config_z.
 */
#ifndef MODULES_COMMAND_COMMAND_Z_H_
#define MODULES_COMMAND_COMMAND_Z_H_

#include <stdint.h>

/**
 * Command handler for messenger_z: parse input, fill output, return bytes written.
 * Same signature as Contiki handler for messenger_add_handler.
 * \return bytes written to outputdata, or 0 on error
 */
int command_z_handler(const uint8_t *inputdata, int inputlength,
		      uint8_t *outputdata, int *maxoutputlen);

/**
 * Register command handler with messenger_z. Call after messenger_z_init().
 */
void command_z_register(void);

#endif /* MODULES_COMMAND_COMMAND_Z_H_ */
