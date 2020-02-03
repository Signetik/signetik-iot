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

#include "signetik.h"
#include "LTE_task.h"
#include "accel_task.h"
#include "temp_humidity_task.h"

#include "cell_packet.h"
#include "coap_cbor_device.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void go_to_sleep(int ms);

void main_thread(void)
{
	struct device *led_en, *led_r, *led_g, *led_b;
	s64_t next_time = k_uptime_get() + 5000;
	int sleep_duration;
	const int sensor_interval = 1000 * 60;
	const int modem_rate = 5;
	int record_number = 1;
	int sample_number = 0;
	struct tha_packet_s tha_data;
	int result;

#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER)
	led_en = device_get_binding(DT_ALIAS_LEDEN_GPIOS_CONTROLLER);
	led_r = device_get_binding(DT_ALIAS_LEDR_GPIOS_CONTROLLER);
	led_g = device_get_binding(DT_ALIAS_LEDG_GPIOS_CONTROLLER);
	led_b = device_get_binding(DT_ALIAS_LEDB_GPIOS_CONTROLLER);
	gpio_pin_configure(led_en, DT_ALIAS_LEDEN_GPIOS_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(led_r, DT_ALIAS_LEDR_GPIOS_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(led_g, DT_ALIAS_LEDG_GPIOS_PIN, GPIO_DIR_OUT);
	gpio_pin_configure(led_b, DT_ALIAS_LEDB_GPIOS_PIN, GPIO_DIR_OUT);
#endif

	LTE_wait_complete();

	while(1) {
#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER)
		gpio_pin_write(led_en, DT_ALIAS_LEDEN_GPIOS_PIN, 1);
		gpio_pin_write(led_r, DT_ALIAS_LEDR_GPIOS_PIN, 1);
		gpio_pin_write(led_g, DT_ALIAS_LEDG_GPIOS_PIN, 1);
		gpio_pin_write(led_b, DT_ALIAS_LEDB_GPIOS_PIN, 1);
#endif
		sleep_duration = next_time - k_uptime_get();
		if (sleep_duration > sensor_interval)
			sleep_duration = sensor_interval;
		if ((sleep_duration * modem_rate) < 60000)
			sleep_duration = 60000 / modem_rate;
		LOG_INF("Sleeping for %d milliseconds (%lu,%lu).", sleep_duration, (long)next_time, (long)k_uptime_get());
		go_to_sleep(sleep_duration);

#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER)
		gpio_pin_write(led_en, DT_ALIAS_LEDEN_GPIOS_PIN, 0);
		gpio_pin_write(led_r, DT_ALIAS_LEDR_GPIOS_PIN, 0);
		gpio_pin_write(led_b, DT_ALIAS_LEDB_GPIOS_PIN, 0);
#endif
		next_time += sensor_interval;

#ifdef CONFIG_LIS3DH
		accel_start();
#endif
#ifdef CONFIG_HTU21D
		temp_humid_start();
#endif

#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER)
		gpio_pin_write(led_r, DT_ALIAS_LEDR_GPIOS_PIN, 0);
		gpio_pin_write(led_b, DT_ALIAS_LEDB_GPIOS_PIN, 1);
#endif

		LOG_INF("Waiting for sensor data.");
		while (false
#ifdef CONFIG_LIS3DH
				|| !accel_ready()
#endif
#ifdef CONFIG_HTU21D
				|| !temp_humid_ready()
#endif
			  )
		{
			// TODO: Some timeout
			k_sleep(100);
		}

		LOG_INF("Storing sensor data (%d).", record_number + sample_number);
		accel_get_data(&tha_data.x, &tha_data.y, &tha_data.y);
		temp_humid_get_data(&tha_data.temperature, &tha_data.humidity);
		if (base_time != 0) {
			s64_t now = k_uptime_get() + base_time;
			tha_data.ts_s = (u32_t)(now / 1000);
			tha_data.ts_ms = (u32_t)(now % 1000);
		}
		else {
			tha_data.ts_s = 0;
			tha_data.ts_ms = 0;
		}
        sig_tha_set_data( &tha_data, sample_number++ );

		if (sample_number >= modem_rate) {
			LOG_INF("%d samples ready, starting connection.", modem_rate);

#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER)
			gpio_pin_write(led_r, DT_ALIAS_LEDR_GPIOS_PIN, 1);
			gpio_pin_write(led_b, DT_ALIAS_LEDB_GPIOS_PIN, 0);
#endif

			LTE_send(record_number, modem_rate);

			// wait and if success
			result = LTE_wait_complete();

			if (result == 0) {
				LOG_INF("Data sent to server. Updating stored data.");
				record_number += modem_rate;
			}
			else {
				LOG_ERR("Failed to send data. Dropping data.");
				// TODO: 
				record_number += modem_rate;
			}
			sample_number = 0;
		}

	}
}

static void go_to_sleep(int ms)
{
	//TODO: Don't sleep, use timer
	k_sleep(ms);
	//	k_timer_start(&app_timer1, K_MINUTES(1), K_MINUTES(1));
}


K_THREAD_DEFINE(main_id, MAIN_STACKSIZE, main_thread, NULL, NULL, NULL,
                MAIN_PRIORITY, K_FP_REGS, K_NO_WAIT);
