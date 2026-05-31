//
// Created by fubingyan on 24-12-26.
//

/**
 * @file    device_mt6701.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2024-12-26
 * @brief   MT6701 编码器驱动
 * @attention
 *
 * Copyright (c) 2024 by fubingyan, All Rights Reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "device_mt6701.h"
#include "device_mt6816.h"

#include "hal_gpio.h"
#include "hal_spi.h"
#include "public.h"

/* Private variables ---------------------------------------------------------*/

extern hal_spi_context_t mt68xx_spi_ctx;
extern hal_gpio_context_t mt68xx_cs_ctx;

static uint16_t s_data_buffer[2] = { 0 };
static device_mt6701_raw_t s_raw_data;

/* Private function prototypes -----------------------------------------------*/

static uint8_t crc6_itu(const uint16_t* data, uint32_t length);

static uint16_t get_raw_data(uint16_t* raw_data);

static inline void spi_cs_low(void)
{
    hal_gpio_write(&mt68xx_cs_ctx, HAL_GPIO_PORT_A, HAL_GPIO_PIN_15,
        HAL_GPIO_PIN_RESET);
}

static inline void spi_cs_high(void)
{
    hal_gpio_write(&mt68xx_cs_ctx, HAL_GPIO_PORT_A, HAL_GPIO_PIN_15,
        HAL_GPIO_PIN_SET);
}

/* Exported functions --------------------------------------------------------*/

uint16_t device_mt6701_get_angle_data(void) { return get_raw_data(NULL); }

/* Private functions ---------------------------------------------------------*/

static uint16_t get_raw_data(uint16_t* raw_data)
{
    /* MT6701 需要2个16-bit SPI帧：第1帧发命令，第2帧发 dummy 以读取第2个响应字 */
    uint16_t txData[2] = { 0xFFFF, 0x0000 };
    uint8_t Retry_count = 2;

    if (hal_spi_get_state(&mt68xx_spi_ctx) != HAL_SPI_ST_READY) {
        return 0;
    }

Retry:

    Retry_count--;

    spi_cs_low();

    /* SPI3 配置为 16-bit 模式，Size 为半字数（非字节数）= 2 */
    const hal_spi_error_t spiStatus = hal_spi_transmit_receive(
        &mt68xx_spi_ctx, (uint8_t*)txData, (uint8_t*)s_data_buffer,
        sizeof(s_data_buffer) / sizeof(s_data_buffer[0]), 100);
    if (spiStatus != HAL_SPI_OK) {
        spi_cs_high();
        return 0;
    }

    spi_cs_high();

    s_raw_data.mt6701_raw_angle = (s_data_buffer[0] >> 2) & 0x3FFF;
    s_raw_data.Status.magnetic_state = ((s_data_buffer[1] >> 14) | (s_data_buffer[0] & 0x03) >> 2) & 0x03;
    s_raw_data.Status.Push_button_state = ((s_data_buffer[1] >> 14) | (s_data_buffer[0] & 0x03) >> 1) & 0x01;
    s_raw_data.Status.Loss_of_Track = ((s_data_buffer[1] >> 14) | (s_data_buffer[0] & 0x03) >> 0) & 0x01;

    s_raw_data.CRC_6Bit = (s_data_buffer[1] >> 8) & 0x003F;
    uint8_t mt6701_crc = crc6_itu(s_data_buffer, 3);

    if (mt6701_crc != s_raw_data.CRC_6Bit && Retry_count) {
        mt6701_crc = 0;
        s_raw_data.CRC_6Bit = 0;
        goto Retry;
    }

    if (raw_data != NULL) {
        *raw_data = s_raw_data.mt6701_raw_angle;
    }
    return s_raw_data.mt6701_raw_angle;
}

static uint8_t crc6_itu(const uint16_t* data, uint32_t length)
{
    uint8_t i = 0, j = 0;
    uint8_t crc = 0;

    const uint8_t crc_data[3] = {
        data[0] >> 10 & 0x3F, data[0] >> 4 & 0x3F,
        (data[1] >> 14 | (data[0] & 0x000F) << 2) & 0x3F
    };

    while (length--) {
        crc ^= crc_data[j++];
        for (i = 6; i > 0; --i) {
            if (crc & 0x20)
                crc = crc << 1 ^ 0x03;
            else
                crc = crc << 1;
        }
    }
    return crc & 0x3f;
}
