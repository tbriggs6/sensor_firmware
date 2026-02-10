/*
 * Messenger service - Zephyr UDP implementation.
 * Same semantics as message-service.h for compatibility.
 */
#ifndef MODULES_MESSENGER_MESSENGER_Z_H_
#define MODULES_MESSENGER_MESSENGER_Z_H_

#include <stdint.h>

#define COMMAND_SERVER_PORT_Z 5323
#define MESSAGE_SERVER_PORT_Z 5555

/** IPv6 address for send: 8 x uint16_t (network order) */
typedef struct {
	uint16_t u16[8];
} messenger_addr_t;

typedef int (*messenger_handler_t)(const uint8_t *inputdata, int inputlength,
				   uint8_t *outputdata, int *maxoutputlen);

/**
 * Initialize messenger: start listener thread on COMMAND_SERVER_PORT_Z.
 * \return 0 on success
 */
int messenger_z_init(void);

/**
 * Send data to remote address. Blocks until send completes.
 * If reply is expected, call messenger_z_wait_reply() after.
 */
int messenger_z_send(const messenger_addr_t *remote_addr, uint16_t port,
		     uint16_t sequence, const void *data, int length);

/**
 * Wait for a reply (e.g. ACK) after send, with timeout_ms.
 * Result is stored for messenger_z_last_result_okack().
 */
void messenger_z_wait_reply(int timeout_ms);

/**
 * Non-zero if last send received a valid ACK.
 */
int messenger_z_last_result_okack(void);

/**
 * Get last received RSSI (stub: 0).
 */
int messenger_z_recvd_rssi(void);

/**
 * Register handler for incoming messages (by header match).
 */
void messenger_z_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len,
			     messenger_handler_t handler);

#endif /* MODULES_MESSENGER_MESSENGER_Z_H_ */
