/*
 * Messenger service - Zephyr UDP implementation.
 */
#include "messenger_z.h"
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(messenger_z, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_HANDLERS 8
#define LISTEN_PORT COMMAND_SERVER_PORT_Z
#define RECV_BUF_SIZE 256
#define SEND_BUF_SIZE 256

typedef struct {
	uint32_t header;
	uint32_t min_len;
	uint32_t max_len;
	messenger_handler_t handler;
} handler_entry_t;

static handler_entry_t handlers[MAX_HANDLERS];
static int num_handlers;
static K_MUTEX_DEFINE(handlers_mutex);

static int last_okack;
static int last_rssi;
static K_MUTEX_DEFINE(result_mutex);

static void listener_thread(void *p1, void *p2, void *p3)
{
	int sock;
	struct sockaddr_in6 addr;
	uint8_t *recv_buf;
	uint8_t *send_buf;
	socklen_t addr_len = sizeof(addr);
	int i;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	recv_buf = k_malloc(RECV_BUF_SIZE);
	send_buf = k_malloc(SEND_BUF_SIZE);
	if (!recv_buf || !send_buf) {
		LOG_ERR("messenger alloc fail");
		return;
	}

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("messenger socket %d", sock);
		goto out;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(LISTEN_PORT);
	addr.sin6_addr = in6addr_any;

	if (zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("messenger bind");
		zsock_close(sock);
		goto out;
	}

	LOG_INF("messenger listening on port %d", LISTEN_PORT);

	while (1) {
		ssize_t received = zsock_recvfrom(sock, recv_buf, RECV_BUF_SIZE, 0,
						  (struct sockaddr *)&addr, &addr_len);
		if (received < 4) {
			continue;
		}

		uint32_t *h = (uint32_t *)recv_buf;
		k_mutex_lock(&handlers_mutex, K_FOREVER);
		for (i = 0; i < num_handlers; i++) {
			if (handlers[i].header != *h) {
				continue;
			}
			if ((uint32_t)received < handlers[i].min_len ||
			    (uint32_t)received > handlers[i].max_len) {
				continue;
			}
			int out_len = SEND_BUF_SIZE;
			int n = handlers[i].handler(recv_buf, (int)received, send_buf, &out_len);
			k_mutex_unlock(&handlers_mutex);
			if (n > 0 && out_len > 0) {
				zsock_sendto(sock, send_buf, (size_t)out_len, 0,
					    (struct sockaddr *)&addr, addr_len);
			}
			break;
		}
		if (i >= num_handlers) {
			k_mutex_unlock(&handlers_mutex);
		}
	}

	zsock_close(sock);
out:
	if (recv_buf) {
		k_free(recv_buf);
	}
	if (send_buf) {
		k_free(send_buf);
	}
}

#define LISTENER_STACK 1024
K_THREAD_STACK_DEFINE(listener_stack, LISTENER_STACK);
static struct k_thread listener_thread_data;

int messenger_z_init(void)
{
	num_handlers = 0;
	k_mutex_lock(&result_mutex, K_FOREVER);
	last_okack = 0;
	last_rssi = 0;
	k_mutex_unlock(&result_mutex);

	k_thread_create(&listener_thread_data, listener_stack,
			K_THREAD_STACK_SIZEOF(listener_stack),
			listener_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	return 0;
}

int messenger_z_send(const messenger_addr_t *remote_addr, uint16_t port,
		    uint16_t sequence, const void *data, int length)
{
	int sock;
	struct sockaddr_in6 addr;
	ssize_t sent;
	int ret = 0;

	ARG_UNUSED(sequence);

	if (remote_addr == NULL || data == NULL || length <= 0) {
		return -EINVAL;
	}

	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		return sock;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	memcpy(&addr.sin6_addr, remote_addr->u16, sizeof(addr.sin6_addr));

	sent = zsock_sendto(sock, data, (size_t)length, 0,
			    (struct sockaddr *)&addr, sizeof(addr));
	if (sent != length) {
		ret = -EIO;
	}
	zsock_close(sock);
	return ret;
}

void messenger_z_wait_reply(int timeout_ms)
{
	/* Placeholder: in a full implementation we would block on recv with timeout
	 * and set last_okack from ACK parsing. For now we set 1 after send. */
	ARG_UNUSED(timeout_ms);
	k_mutex_lock(&result_mutex, K_FOREVER);
	last_okack = 1;
	k_mutex_unlock(&result_mutex);
}

int messenger_z_last_result_okack(void)
{
	int v;
	k_mutex_lock(&result_mutex, K_FOREVER);
	v = last_okack;
	k_mutex_unlock(&result_mutex);
	return v;
}

int messenger_z_recvd_rssi(void)
{
	return last_rssi;
}

void messenger_z_add_handler(uint32_t header, uint32_t min_len, uint32_t max_len,
			    messenger_handler_t handler)
{
	k_mutex_lock(&handlers_mutex, K_FOREVER);
	if (num_handlers < MAX_HANDLERS && handler != NULL) {
		handlers[num_handlers].header = header;
		handlers[num_handlers].min_len = min_len;
		handlers[num_handlers].max_len = max_len;
		handlers[num_handlers].handler = handler;
		num_handlers++;
	}
	k_mutex_unlock(&handlers_mutex);
}
