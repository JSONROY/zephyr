# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_HASH_STM32_POLLING hash_stm32_polling.c)
zephyr_library_sources_ifdef(CONFIG_HASH_STM32_DMA hash_stm32_dma.c)
zephyr_library_sources_ifdef(CONFIG_HASH_STM32_INT hash_stm32_int.c)
