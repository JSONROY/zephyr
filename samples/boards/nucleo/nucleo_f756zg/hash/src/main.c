/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/hash/hash_stm32.h>

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   250

/* The devicetree node identifier for the "led0" alias. */
#define DMA_NODE CONFIG_DMA_2_NAME
#define LED0_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led2)
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0    DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN    DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS    DT_GPIO_FLAGS(LED0_NODE, gpios)

#define LED2    DT_GPIO_LABEL(LED2_NODE, gpios)
#define PIN2    DT_GPIO_PIN(LED2_NODE, gpios)
#define FLAGS2    DT_GPIO_FLAGS(LED2_NODE, gpios)

#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif




void main(void) {
    const struct device *dev_led_green;
    const struct device *dev_led_red;
    bool led_is_on = true;
    int ret;

    printk("Application start\n");

    dev_led_green = device_get_binding(LED0);
    if (!dev_led_green) {
        printk("Cannot get LED0 controller\n");
        return;
    }

    dev_led_red = device_get_binding(LED2);
    if (!dev_led_red) {
        printk("Cannot get LED2 controller\n");
        return;
    }

    ret = gpio_pin_configure(dev_led_green, PIN, GPIO_OUTPUT_INACTIVE | FLAGS);
    if (ret < 0) {
        return;
    }

    ret = gpio_pin_configure(dev_led_red, PIN2, GPIO_OUTPUT_INACTIVE | FLAGS2);
    if (ret < 0) {
        return;
    }

    hash_something();

    printk("Done\n");
    while (1) {
        gpio_pin_set(dev_led_green, PIN, (int) led_is_on);
        led_is_on = !led_is_on;
        k_msleep(SLEEP_TIME_MS);
    }
}

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
