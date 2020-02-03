/*============================================================================*
 *         Copyright © 2019-2020 Signetik, LLC -- All Rights Reserved         *
 *----------------------------------------------------------------------------*
 *                                                              Signetik, LLC *
 *                                                           www.signetik.com *
 *                                      SPDX-License-Identifier: BSD-4-Clause *
 *============================================================================*/

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <net/coap.h>
#include <net/socket.h>
#include <fcntl.h>
#include <lte_lc.h>
#include <at_cmd.h>
#include <drivers/gpio.h>

#include <sensor.h>
#include <logging/log.h>

#include <net/sntp.h>

#include "signetik.h"
#include "LTE_task.h"
#include "accel_task.h"
#include "temp_humidity_task.h"

#include "cell_packet.h"
#include "coap_cbor_device.h"

LOG_MODULE_REGISTER(lte, LOG_LEVEL_DBG);

#define APP_COAP_SEND_INTERVAL_MS K_MSEC(5000)
#define APP_COAP_MAX_MSG_LEN 1280
#define APP_COAP_VERSION 1

bool LTE_ready = false;
static int sock;
static struct pollfd fds;
static struct sockaddr_storage server;
static u16_t next_token;

bool send1 = false;

static int record_number = 0, record_count = 0;
s64_t base_time = 0;

K_SEM_DEFINE(sem_lte, 0, 1);
K_SEM_DEFINE(sem_lte_complete, 0, 1);

static int lte_result;

void LTE_send(int rn, int rc)
{
	record_number = rn;
	record_count = rc;
	k_sem_give(&sem_lte);
}

int LTE_wait_complete(void)
{
	k_sem_take(&sem_lte_complete, K_FOREVER);
	return lte_result;
}

#if defined(CONFIG_BSD_LIBRARY)

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}
bool cellular_ready(void)
{
	return LTE_ready;
}
/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	printk("bsdlib irrecoverable error: %u\n", err);

	__ASSERT_NO_MSG(false);
}

#endif /* defined(CONFIG_BSD_LIBRARY) */

/**@brief Resolves the configured hostname. */
static int server_resolve(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_DGRAM
	};
	char ipv4_addr[NET_IPV4_ADDR_LEN];

	err = getaddrinfo(CONFIG_COAP_SERVER_HOSTNAME, NULL, &hints, &result);
	if (err != 0) {
		LOG_ERR("getaddrinfo failed %d", err);
		return -EIO;
	}

	if (result == NULL) {
		LOG_ERR("Address not found");
		return -ENOENT;
	}

	/* IPv4 Address. */
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&server);

	server4->sin_addr.s_addr =
		((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_COAP_SERVER_PORT);

	inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
			sizeof(ipv4_addr));
	LOG_INF("IPv4 Address found %s", log_strdup(ipv4_addr));

	/* Free the address. */
	freeaddrinfo(result);

	return 0;
}

/**@brief Initialize the CoAP client */
static int client_init(void)
{
	int err;

	LOG_INF("Opening socket");
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create CoAP socket: %d.", errno);
		return -errno;
	}

	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);

	LOG_INF("Connecting");
	err = connect(sock, (struct sockaddr *)&server,
			sizeof(struct sockaddr_in));
	if (err < 0) {
		if (errno != EINPROGRESS) {
			LOG_ERR("Connect failed : %d", errno);
			close(sock);
			return -errno;
		}
		else {
			LOG_INF("Connection in progress.");
		}
	}

	/* Initialize FDS, for poll. */
	fds.fd = sock;
	fds.events = POLLOUT;

	/* Randomize token. */
	next_token = sys_rand32_get();

	return 0;
}

/**@brief Handles responses from the remote CoAP server. */
static int client_handle_get_response(u8_t *buf, int received)
{
	int err;
	struct coap_packet reply;
	const u8_t *payload;
	u16_t payload_len;
	u8_t token[8];
	u16_t token_len;
	u8_t temp_buf[16];

	err = coap_packet_parse(&reply, buf, received, NULL, 0);
	if (err < 0) {
		LOG_ERR("Malformed response received: %d", err);
		return err;
	}

	payload = coap_packet_get_payload(&reply, &payload_len);
	//	token_len = coap_header_get_token(&reply, token);

	//	if ((token_len != sizeof(next_token)) &&
	//	    (memcmp(&next_token, token, sizeof(next_token)) != 0)) {
	//		printk("Invalid token received: 0x%02x%02x\n",
	//		       token[1], token[0]);
	//		return 0;
	//	}

	snprintf(temp_buf, MIN(payload_len, sizeof(temp_buf)), "%s", payload);
	LOG_INF("CoAP response: code: 0x%x, token 0x%02x%02x, payload: %s",
			coap_header_get_code(&reply), token[1], token[0], log_strdup(temp_buf));
	//        u8_t temp_buf1[16];
	//	snprintf(temp_buf1, sizeof(token) - 2, "%s", token[2]);
	//
	//	printk("CoAP response: code: 0x%x, token 0x%02x%02x : %d - %s - payload: %s\n",
	//	       coap_header_get_code(&reply), token[1], token[0], token_len, temp_buf1, temp_buf);

	return 0;
}

static inline bool append_u8(struct coap_packet *cpkt, u8_t data)
{
	if (!cpkt) {
		return false;
	}

	if (cpkt->max_len - cpkt->offset < 1) {
		return false;
	}

	cpkt->data[cpkt->offset++] = data;

	return true;
}

/**@biref Send CoAP GET request. */
int client_get_send(uint8_t *buffer, size_t length)
{
	int err;

	err = send(sock, buffer, length, 0);
	if (err < 0) {
		LOG_ERR("Failed to send CoAP request, %d", errno);
		return -errno;
	}

	LOG_INF("CoAP request sent: %d bytes", length);

	return 0;
}

void system_configure(void)
{
	static u8_t at_buff[40];
#if(1)
	enum at_cmd_state at_state;
	// Check IMEI
	int error = at_cmd_write("AT+CGSN", at_buff, sizeof(at_buff), &at_state);
	if (error) {
		LOG_ERR("Error when trying to do at_cmd_write: %d, at_state: %d\n\n", error, at_state);
		//            return 0;
	}
	else {
		LOG_INF("IMEI: %s", log_strdup(at_buff));
	}

	/*      The following may be needed to register with Verizon network (https://www.multitech.com/documents/publications/activation-guides/verizon-wireless-activation-procedure-for-mvw1-devices.pdf) */

#if(0)
	// Check if device registered
	char ceregBuff[1024];
	error = at_cmd_write("AT+CEREG?", ceregBuff, sizeof(ceregBuff), &at_state);
	if (error) {
		printk("Error when trying to do at_cmd_write: %d, at_state: %d\n\n", error, at_state);
		//            return 0;
	}
	else {
		printk("registered: %s", ceregBuff);
		printk("Should be \"+CEREG:0,1\" OR \"+CEREG:0,5\"\n\n");
	}
#endif

	// Check APN recieved
	error = at_cmd_write("AT+CGCONT", at_buff, sizeof(at_buff), &at_state);
	if (error) {
		LOG_ERR("Error when trying to do at_cmd_write: %d, at_state: %d", error, at_state);
		//            return 0;
	}
	else {
		LOG_INF("APN: %s", log_strdup(at_buff));
		LOG_INF("Should be \"1,5,vzwinternet,...\"");
	}
#if(0)
	// Set PDP context
	error = at_cmd_write("AT+CGDCONT=3,\"IPV4V6\",\"VZWINTERNET\"\r\n", NULL, 0, &at_state);
	if (error) {
		printk("Error when trying to do at_cmd_write: %d, at_state: %d\n\n", error, at_state);
		//            return 0;
	}
#endif
#endif
}

static void get_sntp_time(void)
{
	struct sntp_time time;
	int result;

	result = sntp_simple("time.google.com", 10000, &time);

	if (result == 0) {
		uint32_t ms;
		LOG_INF("SNTP time: %d:%u", (uint32_t)time.seconds, time.fraction);
		ms = ((time.fraction / (1<<10)) * 1000) / (1<<22);
		LOG_INF("SNTP time: %d.%03d", (uint32_t)time.seconds, ms);
		base_time = ((s64_t)time.seconds)*1000 + ms;
		base_time -= k_uptime_get();
		LOG_INF("SNTP base_time: %d%03d", (s32_t)(base_time/1000), (s32_t)(base_time%1000));
	}
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{

#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		LOG_INF("LTE Link Connecting...");
		err = lte_lc_init_and_connect();
		if (err != 0) {
			LOG_ERR("Failed LTE init and connect (%d)", err);
			return err;
		}
		LOG_INF("LTE Link Connected!");
		LTE_ready = true;
		LOG_INF("Entering--->>> LTE Link Power saving mode");
		err = lte_lc_psm_req(true);
		if (err == 0)
			LOG_INF("LTE Link power saving enabled!");
		else
			LOG_INF("LTE Link power saving NOT enabled!");
	}
#endif

	if (base_time == 0) {
		get_sntp_time();
	}
	return 0;
}
/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_poweroff(void)
{

#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		LOG_INF("LTE Link powering off...");
		err = lte_lc_power_off();
		if (err != 0) {
			LOG_ERR("LTE link could not be powered off.");
		}
		else {
			LOG_INF("LTE Link is powered off!");
		}
		LTE_ready = false;

	}
#endif
}


/* Returns 0 if data is available.
 * Returns -EAGAIN if timeout occured and there is no data.
 * Returns other, negative error code in case of poll error.
 */
static int wait(int timeout)
{
	int ret;

	LOG_DBG("Waiting on socket %d for %d ms.", fds.fd, timeout);

	ret = poll(&fds, 1, timeout);

	LOG_DBG("Socket wait completed.");

	if (ret < 0) {
		LOG_ERR("poll error: %d", errno);
		return -errno;
	}

	if (ret == 0) {
		/* Timeout. */
		LOG_DBG("Socket wait timed out after %d ms.", timeout);
		return -EAGAIN;
	}

	if ((fds.revents & POLLERR) == POLLERR) {
		LOG_ERR("wait: POLLERR");
		return -EIO;
	}

	if ((fds.revents & POLLNVAL) == POLLNVAL) {
		LOG_ERR("wait: POLLNVAL");
		return -EBADF;
	}

	if ((fds.revents & (POLLIN | POLLOUT)) == 0) {
		LOG_DBG("Socket event failed after %d ms.", timeout);
		return -EAGAIN;
	}

	return 0;
}
//void app_timer1_handler(struct k_timer *dummy)
//{
//	static u32_t minutes = 0;
//
//	minutes++;
//        printk("Sleeping time: %d\n", minutes);
//	/* This shall match the PSM interval */
//	if (minutes % 5 == 0) {
//            LTE_wake = true;
//            minutes = 0;
//	}
//}
//void app_timer2_handler(struct k_timer *dummy)
//{
//	static u32_t minutes = 0;
//
//	minutes++;
//	/* This shall match the PSM interval */
//	if (minutes % 1 == 0) {
//		  = false;
//	}
//	printk("Waken time: %d\n", minutes);
//}

//K_TIMER_DEFINE(app_timer1, app_timer1_handler, NULL);
//K_TIMER_DEFINE(app_timer2, app_timer2_handler, NULL);

//void timer1_stop(void)
//{
//	k_timer_stop(&app_timer1);
//}
//void timer2_init(void)
//{
//	k_timer_start(&app_timer2, K_MINUTES(1), K_MINUTES(1));
//}
//void timer2_stop(void)
//{
//	k_timer_stop(&app_timer2);
//}



void LTE_thread(void)
{
	int err;

	//    k_sem_init(&new_settings, 0, 1);
#if(0)
	struct device *d;
	d = device_get_binding("GPIO_0");
	gpio_pin_configure(d, 11, GPIO_DIR_OUT);
	gpio_pin_write(d, 11, 1);
#endif

	system_configure();

	err = modem_configure();

	if (err != 0) {
		LOG_ERR("LTE failed to connect (%d) for SNTP.", err);
	}

	while(1){
		k_sem_give(&sem_lte_complete);
		modem_poweroff();
		k_sem_take(&sem_lte, K_FOREVER);

		lte_result = -ECONNABORTED;

		int received;
		LOG_INF("Starting modem.");

		err = modem_configure();

		if (err != 0) {
			LOG_ERR("LTE failed to connect (%d).", err);
			continue;
		}

		if (server_resolve() != 0) {
			LOG_ERR("Failed to resolve server name");
			continue;
		}

		if (client_init() != 0) {
			LOG_ERR("Failed to initialize CoAP client");
			continue;
		}

		static u8_t coap_buf[APP_COAP_MAX_MSG_LEN];

		static uint8_t   COAP_buff[APP_COAP_MAX_MSG_LEN];
		uint8_t * COAP_buff_ptr = &COAP_buff[0];
		size_t   COAP_buff_len = sizeof(COAP_buff);
		int retry = 3;

		LOG_INF("Waiting for connect.");
		if (wait(15000) != 0) {
			LOG_INF("Failed to connect.");
			retry = 0;
		}

		create_coap_cbor_with_packet_def(&sig_tha_packet_def, DEVICE_ID, record_number, record_count, &COAP_buff_ptr, &COAP_buff_len);
		while (retry)
		{
			retry--;
			LOG_DBG("Sending request %d-%p-%d.", sock, COAP_buff_ptr, COAP_buff_len);

			k_sleep(100); // socket "send" crashes no delay between connect and send.
			if (client_get_send(COAP_buff_ptr, COAP_buff_len) != 0)
			{
				LOG_ERR("Failed to send GET request.");
				break;
			}

			int error = EAGAIN;
			int timeout = 15000;
			while (((error == EAGAIN) || (error == EWOULDBLOCK)) && (timeout > 0)) {
				LOG_DBG("Receiving response.");
				received = recv(sock, coap_buf, sizeof(coap_buf), 0);
				LOG_DBG("Receive result %d", received);
				if (received < 0) {
					error = errno;
					if (error == EAGAIN || error == EWOULDBLOCK) {
						LOG_INF("socket EAGAIN");
						k_sleep(100);
						timeout -= 100;
						continue;
					} 
					else {
						LOG_ERR("Socket error %d.", error);
						break;
					}
				}
				else {
					error = 0;
				}
			}

			if (error == 0) {
				if (received == 0) {
					LOG_ERR("Empty datagram");
					continue;
				}
				LOG_DBG("Parse response");
				err = client_handle_get_response(coap_buf, received);
				if (err < 0) {
					LOG_ERR("Invalid response.");
					break;
				}
				else {
					LOG_DBG("Response good");
					retry = 0;
					lte_result = 0;
				}
			}
		}
		LOG_DBG("Closing socket");
		(void)close(sock);
	}
}

// LTE thread
K_THREAD_DEFINE(LTE_id, LTE_STACKSIZE, LTE_thread, NULL, NULL, NULL,
                LTE_PRIORITY, K_FP_REGS, K_NO_WAIT);
