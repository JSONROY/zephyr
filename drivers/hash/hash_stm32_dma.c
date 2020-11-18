//
// Created by jason on 11/9/20.
//

/*
 * Copyright (c) 2020 Jason Roy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <string.h>
#include "hash_stm32_priv.h"

#define LOG_LEVEL CONFIG_HASH_LOG_LEVEL
#include <logging/log.h>
#include <drivers/dma.h>

LOG_MODULE_REGISTER(hash_stm32);

#define HASH_DEV DT_NODELABEL(hash1)

// TODO: Replace by DTS definitions
#define HASH_IRQN		80
#define HASH_IRQ_PRIO	87

// TODO: Replace by DTS definitions
#define HASH_DMA2_STREAM_IRQ 70
#define HASH_DMA2_STREAM_PRIO 77

static HASH_HandleTypeDef hhash;
static DMA_HandleTypeDef hashDmaHandleTypeDefIn;
static DMA_HandleTypeDef hashDmaHandleTypeDefOut;

DEVICE_DECLARE(hash1_dev);

__ALIGN_BEGIN const uint8_t
aInput[]
__ALIGN_END = "The STM32 F7 series is the result of a perfect symbiosis of the real-time control capabilities of an MCU and the signal processing performance of a DSP, and thus complements the STM32 portfolio with a new class of devices, digital signal controllers (DSC).";

__ALIGN_BEGIN static uint8_t
pOutBuffer[32]
__ALIGN_END;

__ALIGN_BEGIN static uint8_t
aExpectSHA256Digest[32]
__ALIGN_END = {0x24, 0xac, 0xe1, 0x6a, 0x7d, 0x41, 0x59, 0xdc,
               0x60, 0x4d, 0x1d, 0x96, 0x6a, 0x3b, 0x27, 0xf4,
               0x86, 0x44, 0x60, 0xf8, 0xe5, 0x68, 0x26, 0x25,
               0x1a, 0x53, 0x9d, 0x08, 0x36, 0x74, 0x9c, 0x06
};

void print_array_hex(uint8_t *buffer, uint32_t length);


static void hash_isr(const struct device *dev);
static void hash_dma_isr(const struct device *dev);

void hash_something() {
    const struct device *hash_dev = device_get_binding(DT_LABEL(HASH_DEV));
    const struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
//    struct hash_stm32_data *data = HASH_STM32_DATA(hash_dev);
    const struct hash_stm32_config *cfg = HASH_STM32_CFG(hash_dev);
    int z_status = 0;

    // TODO: Replace by DTS definitions
    struct stm32_pclken hash_pclken = {
            .bus = STM32_CLOCK_BUS_AHB2,
            .enr = 0x00000020
    };

    // TODO: Replace by DTS definitions
    struct stm32_pclken hash_dma_pclken = {
            .bus = STM32_CLOCK_BUS_AHB1,
            .enr = 0x00400000
    };

    HAL_StatusTypeDef status = HAL_OK;
    uint32_t Timeout = 5000;
    DMA_InitTypeDef hashDmaInitTypeDefIn;

    __ASSERT_NO_MSG(clk);

    z_status = clock_control_on(clk, (clock_control_subsys_t *)&hash_pclken);
    printk("Setup hash peripheral clock %d\n", z_status);

    z_status = clock_control_on(clk, (clock_control_subsys_t *)&hash_dma_pclken);
    printk("Setup hash dma2 clock %d\n", z_status);

    memset(pOutBuffer, 0, 32);


    printk("Configure\n");
    hhash.Init.DataType = HASH_CR_DATATYPE_1;

    // TODO: Replace by DTS definitions
    hashDmaInitTypeDefIn.Channel = DMA_CHANNEL_2;
    hashDmaInitTypeDefIn.Direction = DMA_MEMORY_TO_PERIPH;
    hashDmaInitTypeDefIn.PeriphInc = DMA_PINC_ENABLE;
    hashDmaInitTypeDefIn.MemInc = DMA_PINC_DISABLE;
    hashDmaInitTypeDefIn.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hashDmaInitTypeDefIn.MemDataAlignment = DMA_PDATAALIGN_BYTE;
    hashDmaInitTypeDefIn.Mode = DMA_NORMAL;
    hashDmaInitTypeDefIn.Priority = DMA_PRIORITY_VERY_HIGH;
    hashDmaInitTypeDefIn.FIFOMode = DMA_FIFOMODE_ENABLE;
    hashDmaInitTypeDefIn.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hashDmaInitTypeDefIn.MemBurst = DMA_MBURST_INC4;
    hashDmaInitTypeDefIn.PeriphBurst = DMA_MBURST_INC4;
    hashDmaHandleTypeDefIn.Init = hashDmaInitTypeDefIn;
    hashDmaHandleTypeDefIn.Instance = DMA2_Stream7;
    hashDmaHandleTypeDefIn.StreamBaseAddress = DMA2_Stream7_BASE;
    hhash.hdmain = &hashDmaHandleTypeDefIn;

//    hashDmaHandleTypeDefIn.Instance = DMA2_BASE;

    //hashDmaHandleTypeDefIn.Init
//            hashDmaHandleTypeDefout

    printk("DeInit\n");
    status = HAL_HASH_DeInit(&hhash);
    if (status != HAL_OK) {
        printk("DeInit Failed %d\n", status);
    }

    printk("Init hash handle\n");
    status = HAL_HASH_Init(&hhash);
    if (status != HAL_OK) {
        printk("Init Failed %d\n", status);
    }

    printk("DeInit\n");
    status = HAL_DMA_DeInit(&hashDmaHandleTypeDefIn);
    if (status != HAL_OK) {
        printk("DeInit Failed %d\n", status);
    }

    printk("Init dma hash handle\n");
    status = HAL_DMA_Init(&hashDmaHandleTypeDefIn);
    if (status != HAL_OK) {
        printk("Init Failed %d\n", status);
    }

    printk("Setup interrupts\n");
    IRQ_CONNECT(HASH_IRQN,
                HASH_IRQ_PRIO,
                hash_isr,
                DEVICE_GET(hash1_dev),
                0);

    __HAL_HASH_CLEAR_FLAG(HASH_FLAG_DCIS);
    __HAL_HASH_CLEAR_FLAG(HASH_FLAG_DINIS);
    irq_enable(HASH_IRQN);

    IRQ_CONNECT(HASH_DMA2_STREAM_IRQ,
                HASH_DMA2_STREAM_PRIO,
                hash_dma_isr,
                DEVICE_GET(hash1_dev),
                0);

    irq_enable(HASH_DMA2_STREAM_IRQ);

    printk("Expecting: ");
    print_array_hex(aExpectSHA256Digest, 32);

    /* Start HASH computation */
    printk("Start interrupt mode using DMA\n");
    status = HAL_HASHEx_SHA256_Start_DMA(&hhash, (uint8_t *) aInput, strlen((char const *) aInput));
    if (status != HAL_OK) {
        printk("Start Failed %d\n", status);
    }
}

static void hash_isr(const struct device *hash) {
    printk("hash_isr\n");
    HAL_HASH_IRQHandler(&hhash);
    if (__HAL_HASH_GET_FLAG(HASH_FLAG_DCIS))
    {
        printk("In interrupt: computed digest finished\n");
        printk("Computed:  ");
        print_array_hex(pOutBuffer, 32);

        /* Compare computed digest with expected one */
        printk("Compare\n");
        if (memcmp(pOutBuffer, aExpectSHA256Digest, sizeof(aExpectSHA256Digest) / sizeof(aExpectSHA256Digest[0])) != 0) {
            printk("Compared Failed\n");
        }
    }
}

static void hash_dma_isr(const struct device *hash) {
    printk("hash_dma_isr\n");
    HAL_DMA_IRQHandler(hhash.hdmain);
    if (hashDmaHandleTypeDefIn.State == HAL_DMA_STATE_READY)
//    if (__HAL_DMA_GET_TC_FLAG_INDEX(&hashDmaHandleTypeDefIn))
    {
        printk("In dma interrupt: computed digest finished\n");
//        printk("Computed:  ");
        HAL_HASHEx_SHA256_Finish(&hhash, pOutBuffer, 1000);
//        print_array_hex(pOutBuffer, 32);
//
//        /* Compare computed digest with expected one */
//        printk("Compare\n");
//        if (memcmp(pOutBuffer, aExpectSHA256Digest, sizeof(aExpectSHA256Digest) / sizeof(aExpectSHA256Digest[0])) != 0) {
//            printk("Compared Failed\n");
//        }
    }
}

void print_array_hex(uint8_t *buffer, uint32_t length) {
    for (unsigned long i = 0; i < length; i++) {
        printk("%02x ", buffer[i]);
    }
    printk("\n");
}

//static int hash_stm32_init(const struct device *dev)
//{
//    const struct hash_stm32_config *config = dev->config;
//    const struct device *clk =
//            device_get_binding(STM32_CLOCK_CONTROL_NAME);
//
//    if (clock_control_on(clk,
//                         (clock_control_subsys_t *) &config->pclken) != 0) {
//        return -EIO;
//    }
//}
//
//static struct hash_driver_api hash_enc_funcs = {
//
//};
//
//static struct hash_stm32_data hash_stm32_dev_data = {
//
//};
//
//static struct hash_stm32_config hash_stm32_dev_config = {
//    .pclken = {
//            .bus = STM32_CLOCK_BUS_AHB2,
//            .enr = 0x00000020
//    }
//};
//
//DEVICE_AND_API_INIT(hash_stm32, "hashing",
//                    hash_stm32_init, &hash_stm32_dev_data,
//                    &hash_stm32_dev_config, POST_KERNEL,
//                    CONFIG_HASH_INIT_PRIORITY, (void *)&hash_enc_funcs);

//hash1: hash@50060400 {
//compatible = "st,stm32-hash";
//reg = <50060400 0x40084>;
//interrupts = <80 0>;
//clock-names = "stmhash";
//clocks = <&rcc STM32_CLOCK_BUS_AHB2 0x00000020>;
//status = "disabled";
//label = "HASH_INSTANCE";
//};