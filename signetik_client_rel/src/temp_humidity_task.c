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
#include <sensor.h>
#include <logging/log.h>
#if defined(CONFIG_REGULATOR)
#include <drivers/regulator.h>
#endif

#include "signetik.h"
#include "temp_humidity_task.h"
#include "cell_packet.h"

LOG_MODULE_REGISTER(th, LOG_LEVEL_INF);

////////////HTU21D
bool temp_humid_data_ready;
K_SEM_DEFINE(sem_th, 0, 1);

struct sensor_value temp_value;
struct sensor_value humidity_value;

void temp_humid_start(void)
{
    temp_humid_data_ready = false;
    k_sem_give(&sem_th);
}

bool temp_humid_ready(void)
{
    return temp_humid_data_ready;
}

void temp_humid_get_data(uint32_t *t, uint32_t *h)
{
	*t = temp_value.val1;
	*h = humidity_value.val1;
}

void read_temp_humidity(struct device *dev)
{
	int ret;
//	struct sensor_value attr;
        temp_humid_data_ready = false;

//	attr.val1 = 150;
//	attr.val2 = 0;
//	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
//			      SENSOR_ATTR_FULL_SCALE, &attr);
//	if (ret) {
//		printk("sensor_attr_set failed ret %d\n", ret);
//		return;
//	}
//
//	attr.val1 = 8;
//	attr.val2 = 0;
//	ret = sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
//			      SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
//	if (ret) {
//		printk("sensor_attr_set failed ret %d\n", ret);
//		return;
//	}

    while (1)
    {
        k_sleep(1000);
        k_sem_take(&sem_th, K_FOREVER);

        temp_value.val1 = humidity_value.val1 = 0;
        if (dev) {
#if defined(CONFIG_REGULATOR)
            struct device *reg;

            reg = device_get_binding("lsensor_reg");
            if (reg) {
#if !defined(CONFIG_REGULATOR_ALWAYSON)
                regulator_enable(reg);
#endif
            }
#endif

            ret = sensor_sample_fetch(dev);
            if (ret) {
                LOG_ERR("sensor_sample_fetch failed ret %d", ret);
            }

            ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
            if (ret) {
                LOG_ERR("sensor_channel_get failed ret %d", ret);
            }

            ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity_value);
            if (ret) {
                LOG_ERR("sensor_channel_get failed ret %d", ret);
            }

#if defined(CONFIG_REGULATOR)
            if (reg) {
#if !defined(CONFIG_REGULATOR_ALWAYSON)
                regulator_disable(reg);
#endif
            }
#endif
        }

        temp_humid_data_ready = true;

        if (temp_humid_data_ready == true)
        {
            LOG_INF("temp/humid data ready, T=%d, H=%d", temp_value.val1, temp_value.val2);
        }
    }
}

void temp_humidity_thread(void)
{
    struct device *dev;

    dev = device_get_binding("HTU21D");
    LOG_INF("TH thread, device is %p, name is %s", dev, dev ? dev->config->name : "NOTFOUND");

    read_temp_humidity(dev);
}

#ifdef CONFIG_HTU21D
K_THREAD_DEFINE(temp_humidity_id, TEMP_HUMIDITY_STACKSIZE, temp_humidity_thread, NULL, NULL, NULL,
		TEMP_HUMIDITY_PRIORITY, K_FP_REGS, K_NO_WAIT);
#endif //CONFIG_HTU21D
