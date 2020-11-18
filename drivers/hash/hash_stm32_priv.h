/*
 * Copyright (c) 2020 Jason Roy <jason.roy1403@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_HASH_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_HASH_STM32_PRIV_H_

#include <drivers/hash/hash_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>

#define HASH_STM32_CFG(dev) \
	((const struct hash_stm32_config *const)(dev)->config)

#define HASH_STM32_DATA(dev) \
	((struct hash_stm32_data *const)(dev)->data)

struct hash_stm32_config {
	struct stm32_pclken pclken;
};

struct hash_stm32_data {
	HASH_HandleTypeDef hhash;
};


#endif /* ZEPHYR_DRIVERS_HASH_STM32_PRIV_H_ */
