/*
 * Minimal UDP echo server/client for validating 6LoWPAN stack timing.
 * Zephyr-only; used by water_node, airborne_node, and spinet_node.
 */
#ifndef MODULES_NET_ECHO_ECHO_Z_H_
#define MODULES_NET_ECHO_ECHO_Z_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default echo port (same as COMMAND_SERVER_PORT for convenience). */
#define NET_ECHO_PORT 5323

/**
 * Run UDP echo server: bind to port and echo back every received packet.
 * Does not return; run in a dedicated thread.
 * \param port UDP port to bind (e.g. NET_ECHO_PORT)
 */
void net_echo_server_run(uint16_t port);

/**
 * Send one UDP packet to peer and wait for echo reply (optional).
 * Used by leaf nodes to validate connectivity to spinet/border router.
 * \param peer_addr IPv6 address string (e.g. "fd00::1" or "fe80::1")
 * \param port destination port
 * \param payload optional payload bytes (can be NULL if len 0)
 * \param len length of payload
 * \param wait_reply_ms max ms to wait for reply (0 = don't wait)
 * \return true if sent and (if wait_reply_ms) reply received
 */
bool net_echo_client_send_once(const char *peer_addr, uint16_t port,
			       const uint8_t *payload, size_t len,
			       int wait_reply_ms);

#ifdef __cplusplus
}
#endif

#endif /* MODULES_NET_ECHO_ECHO_Z_H_ */
