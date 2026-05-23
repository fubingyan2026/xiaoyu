//
// Created by fubingyan on 24-12-26.
//

/**
 * @file    device_mt6816.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2024-12-26
 * @brief   MT6816 编码器驱动
 * @attention
 *
 * Copyright (c) 2024 by fubingyan, All Rights Reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "device_mt6816.h"

#include "hal_gpio.h"
#include "hal_spi.h"
#include "public.h"

/* Private types -------------------------------------------------------------*/

typedef struct {
    uint16_t sample_data;
    uint16_t angle;
    bool no_mag_flag;
    bool pc_flag;
} device_mt6816_spi_signal_t;

/* Private variables ---------------------------------------------------------*/

static device_mt6816_spi_signal_t s_spi_signal = { 0 };

hal_spi_context_t mt68xx_spi_ctx;
hal_gpio_context_t mt68xx_cs_ctx;

daemon_context_t* daemon_encoder;

/* Private function prototypes -----------------------------------------------*/

static uint16_t spi_get_angle_data(void);

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

void device_mt6816_init(void)
{
    s_spi_signal.sample_data = 0;
    s_spi_signal.angle = 0;

    stm32_spi_init_context(&mt68xx_spi_ctx);
    hal_spi_config_t spi_cfg = { .instance = HAL_SPI_INSTANCE_3 };
    hal_spi_init(&mt68xx_spi_ctx, &spi_cfg);

    stm32_gpio_init_context(&mt68xx_cs_ctx);
    hal_gpio_config_t cs_cfg = { .port = HAL_GPIO_PORT_A,
        .pin = HAL_GPIO_PIN_15,
        .mode = HAL_GPIO_MODE_OUTPUT_PP,
        .default_state = HAL_GPIO_PIN_SET,
        .pull = HAL_GPIO_PULL_NONE,
        .speed = HAL_GPIO_SPEED_FREQ_HIGH };
    hal_gpio_init(&mt68xx_cs_ctx, &cs_cfg);

    const daemon_config_t daemon_config_encoder = {
        .offline_cb = NULL,
        .owner_ptr = NULL,
        .name = "encoder",
        .reload_timeout_ms = 10,
        .init_wait_time_ms = 1500,
    };
    daemon_encoder = daemon_register(&daemon_config_encoder);
    DEBUG_ASSERT(daemon_encoder);
}

uint16_t device_mt6816_get_angle_data(void)
{
    return spi_get_angle_data();
}

/* Private functions ---------------------------------------------------------*/

static uint16_t spi_get_angle_data(void)
{
    uint16_t data_t[2];
    uint16_t data_r[2];
    data_t[0] = (0x80 | 0x03) << 8;
    data_t[1] = (0x80 | 0x04) << 8;
    for (uint8_t i = 0; i < 1; i++) {
        spi_cs_low();
        hal_spi_transmit_receive(&mt68xx_spi_ctx, (uint8_t*)&data_t[0],
            (uint8_t*)&data_r[0], 1, 10);
        spi_cs_high();

        spi_cs_low();
        hal_spi_transmit_receive(&mt68xx_spi_ctx, (uint8_t*)&data_t[1],
            (uint8_t*)&data_r[1], 1, 10);
        spi_cs_high();
        s_spi_signal.sample_data = ((data_r[0] & 0x00FF) << 8) | (data_r[1] & 0x00FF);

        uint8_t h_count = 0;
        for (uint8_t j = 0; j < 16; j++) {
            if (s_spi_signal.sample_data & (0x0001 << j))
                h_count++;
        }
        if (h_count & 0x01) {
            s_spi_signal.pc_flag = false;
        } else {
            s_spi_signal.pc_flag = true;
            break;
        }
    }
    if (s_spi_signal.pc_flag) {
        s_spi_signal.angle = s_spi_signal.sample_data >> 2;
        s_spi_signal.no_mag_flag = (bool)(s_spi_signal.sample_data & (0x0001 << 1));
    }
    return s_spi_signal.angle;
}
