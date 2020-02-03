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
#include <drivers/regulator.h>
#include <logging/log.h>

#include "signetik.h"
#include "accel_task.h"
#include "cell_packet.h"

LOG_MODULE_REGISTER(acc, LOG_LEVEL_INF);

////////////LIS3DH
static bool accel_data_ready;
K_SEM_DEFINE(sem_accel, 0, 1);

static struct sensor_value x_value;
static struct sensor_value y_value;
static struct sensor_value z_value;

void accel_start(void)
{
    accel_data_ready = false;
    k_sem_give(&sem_accel);
}


bool accel_ready(void)
{
    return accel_data_ready;
}

void accel_get_data(uint32_t *x, uint32_t *y, uint32_t *z)
{
	*x = x_value.val2;
	*y = y_value.val2;
	*z = z_value.val2;
}

void read_lis3dh(struct device *dev)
{
    int ret;
    //    struct sensor_value XYZ_value;

    while (1)
    {
        //	struct sensor_value attr;
        //
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

        k_sleep(1000);
        k_sem_take(&sem_accel, K_FOREVER);

        x_value.val1 = y_value.val1 = z_value.val1 = 0;
        x_value.val2 = y_value.val2 = z_value.val2 = 0;
        if (dev) {
#if defined(CONFIG_REGULATOR)
            struct device *reg;

            reg = device_get_binding("lsensor_reg");
            if (reg) {
                regulator_enable(reg);
            }
#endif

            ret = sensor_sample_fetch(dev);
            if (ret)
            {
                LOG_ERR("sensor_sample_fetch failed ret %d", ret);
            }

            ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &x_value);
            if (ret)
            {
                LOG_ERR("sensor_channel_get failed ret %d", ret);
            }

            ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &y_value);
            if (ret)
            {
                LOG_ERR("sensor_channel_get failed ret %d", ret);
            }

            ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &z_value);
            if (ret)
            {
                LOG_ERR("sensor_channel_get failed ret %d", ret);
            }

#if defined(CONFIG_REGULATOR)
            if (reg) {
                regulator_disable(reg);
            }
#endif
        }

        //            ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &XYZ_value);
        //            if (ret)
        //            {
        //                printk("sensor_channel_get failed ret %d\n", ret);
        //                continue;
        //            }

        accel_data_ready = true;

        if (accel_data_ready == true)
        {
			// TODO: This ^ match is not right
            LOG_INF("Accel data ready, X=%d, Y=%d, Z=%d",
				x_value.val1 + (x_value.val2*(10^(-6))),
				y_value.val1 + (y_value.val2*(10^(-6))),
				z_value.val1 + (z_value.val2*(10^(-6))));
        }
    }
}

void accel_thread(void)
{
    struct device *dev;

    dev = device_get_binding("LIS3DH");
    LOG_INF("ACC thread, device is %p, name is %s", dev, dev ? dev->config->name : "NOTFOUND");

    read_lis3dh(dev);
}

#ifdef CONFIG_LIS3DH
K_THREAD_DEFINE(accel_id, ACCEL_STACKSIZE, accel_thread, NULL, NULL, NULL,
		ACCEL_PRIORITY, K_FP_REGS, K_NO_WAIT);
#endif //CONFIG_LIS3DH
