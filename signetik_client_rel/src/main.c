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
#include <lte_lc.h>

#include <sensor.h>
#include <drivers/gpio.h>
#include <drivers/regulator.h>

void main(void)
{
    struct device *led_en, *led_red, *led_grn, *led_blu;
    struct device *reg;

#if defined(DT_ALIAS_LEDEN_GPIOS_CONTROLLER) && defined(DT_ALIAS_LEDR_GPIOS_CONTROLLER)
    led_en = device_get_binding(DT_ALIAS_LEDEN_GPIOS_CONTROLLER);
    led_red = device_get_binding(DT_ALIAS_LEDR_GPIOS_CONTROLLER);
    led_grn = device_get_binding(DT_ALIAS_LEDG_GPIOS_CONTROLLER);
    led_blu = device_get_binding(DT_ALIAS_LEDB_GPIOS_CONTROLLER);

    gpio_pin_configure(led_en, DT_ALIAS_LEDEN_GPIOS_PIN, GPIO_DIR_OUT);
    gpio_pin_configure(led_red, DT_ALIAS_LEDR_GPIOS_PIN, GPIO_DIR_OUT);
    gpio_pin_configure(led_grn, DT_ALIAS_LEDG_GPIOS_PIN, GPIO_DIR_OUT);
    gpio_pin_configure(led_blu, DT_ALIAS_LEDB_GPIOS_PIN, GPIO_DIR_OUT);

    gpio_pin_write(led_en, DT_ALIAS_LEDEN_GPIOS_PIN, 0);
    gpio_pin_write(led_red, DT_ALIAS_LEDR_GPIOS_PIN, 0);
    gpio_pin_write(led_grn, DT_ALIAS_LEDG_GPIOS_PIN, 1);
    gpio_pin_write(led_blu, DT_ALIAS_LEDB_GPIOS_PIN, 0);
#endif

#if defined(CONFIG_REGULATOR)
    reg = device_get_binding("lsensor_reg");
    if (reg) {
#if !defined(CONFIG_REGULATOR_ALWAYSON)
        regulator_disable(reg);
#endif
    }
#endif
}

