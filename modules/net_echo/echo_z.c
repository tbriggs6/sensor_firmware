/*
 * Minimal UDP echo server/client for Zephyr 6LoWPAN validation.
 */
#include "echo_z.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <string.h>

LOG_MODULE_REGISTER(net_echo, CONFIG_LOG_DEFAULT_LEVEL);

#define ECHO_BUF_SIZE 128

void net_echo_server_run(uint16_t port)
{
	int sock;
	struct sockaddr_in6 addr;
	ssize_t received;
	uint8_t buf[ECHO_BUF_SIZE];

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("echo server: socket failed %d", sock);
		return;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;

	if (zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("echo server: bind failed");
		zsock_close(sock);
		return;
	}

	LOG_INF("echo server: listening on port %u", port);

	while (1) {
		struct sockaddr_in6 client_addr;
		socklen_t client_len = sizeof(client_addr);

		received = zsock_recvfrom(sock, buf, sizeof(buf), 0,
					 (struct sockaddr *)&client_addr,
					 &client_len);
		if (received < 0) {
			LOG_WRN("echo server: recvfrom %zd", received);
			continue;
		}
		if (received == 0) {
			continue;
		}

		(void)zsock_sendto(sock, buf, (size_t)received, 0,
				   (struct sockaddr *)&client_addr, client_len);
	}
}

bool net_echo_client_send_once(const char *peer_addr, uint16_t port,
			      const uint8_t *payload, size_t len,
			      int wait_reply_ms)
{
	int sock;
	struct sockaddr_in6 addr;
	struct zsock_pollfd fds[1];
	int ret;
	ssize_t received;
	uint8_t buf[ECHO_BUF_SIZE];
	bool ok = false;

	if (len > ECHO_BUF_SIZE) {
		return false;
	}

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("echo client: socket %d", sock);
		return false;
	}

	ret = zsock_inet_pton(AF_INET6, peer_addr, &addr.sin6_addr);
	if (ret != 1) {
		LOG_ERR("echo client: inet_pton %d", ret);
		zsock_close(sock);
		return false;
	}
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);

	if (payload && len > 0) {
		memcpy(buf, payload, len);
	} else {
		len = 4;
		memcpy(buf, "ping", 4);
	}

	ret = zsock_sendto(sock, buf, len, 0,
			   (struct sockaddr *)&addr, sizeof(addr));
	if (ret != (ssize_t)len) {
		LOG_ERR("echo client: sendto %d", ret);
		zsock_close(sock);
		return false;
	}

	if (wait_reply_ms <= 0) {
		zsock_close(sock);
		return true;
	}

	fds[0].fd = sock;
	fds[0].events = ZSOCK_POLLIN;
	ret = zsock_poll(fds, 1, wait_reply_ms);
	if (ret <= 0) {
		LOG_WRN("echo client: no reply (poll %d)", ret);
		zsock_close(sock);
		return false;
	}

	received = zsock_recv(sock, buf, sizeof(buf), 0);
	zsock_close(sock);
	ok = (received == (ssize_t)len);
	if (ok) {
		LOG_INF("echo client: reply received");
	}
	return ok;
}
