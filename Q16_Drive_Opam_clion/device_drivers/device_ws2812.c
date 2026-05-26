//
// Created by fubingyan on 24-12-26.
//

/**
 * @file    device_ws2812.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2024-12-26
 * @brief   WS2812 LED 驱动（SPI + DMA）
 * @attention
 *
 * Copyright (c) 2024 by fubingyan, All Rights Reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "device_ws2812.h"

#include "bsp_delay.h"
#include "hal_spi.h"
#include "public.h"

/* Private define ------------------------------------------------------------*/

#define RGB_FLOW_COLOR_CHANGE_TIME 500
#define STOP_CNT 200
#define BLINK_CNT 100

/* Private variables ---------------------------------------------------------*/

static device_ws2812_mode_t s_mode = 0;
static uint8_t s_error_code = 0;

static const uint32_t s_rgb_flow_color[] = {
    0x0F0000FF, 0x0F00FF00, 0x0FFF0000,
    0x0F0000FF, 0x0F00FF00, 0x0FFF0000,
    0x0F0000FF
};

static uint16_t s_i, s_j;
static uint16_t s_last_i;
static float s_delta_alpha, s_delta_red, s_delta_green, s_delta_blue;
static float s_alpha, s_red, s_green, s_blue;
static uint32_t s_argb;

static uint8_t s_error, s_last_error;
static uint8_t s_error_num;

static device_ws2812_buffer_t s_publish = { 0 };
static device_ws2812_buffer_t s_subscribe = { 0 };

static message_center_publisher_t* s_pub = NULL;
static message_center_subscriber_t* s_sub = NULL;

static hal_spi_context_t s_spi_ctx;

/* Private function prototypes -----------------------------------------------*/

static void argb_led_show(const uint32_t argb);
static void led_blink_error(const uint8_t num);
static void led_set(const uint8_t state);

/* Exported functions --------------------------------------------------------*/

void device_ws2812_init(void)
{
    /* 初始化 SPI */
    stm32_spi_init_context(&s_spi_ctx);
    hal_spi_config_t spi_cfg = { .instance = HAL_SPI_INSTANCE_1 };
    hal_spi_init(&s_spi_ctx, &spi_cfg);

    /* 配置消息中心 */
    message_center_config_t config = {
        .name = "ws2812",
        .data_len = sizeof(device_ws2812_buffer_t),
        .queue_size = 4,
        .max_topic_name_len = MESSAGE_CENTER_MAX_TOPIC_NAME_LEN,
    };

    message_center_error_t err = message_center_publisher_register(&s_pub, config);
    DEBUG_ASSERT(err == MESSAGE_CENTER_OK);

    err = message_center_subscriber_register(s_pub, &s_sub);
    DEBUG_ASSERT(err == MESSAGE_CENTER_OK);
}

void device_ws2812_ctrl(const uint16_t led_index, const uint8_t r,
    const uint8_t g, const uint8_t b)
{
    for (int i = 0; i < 8; i++) {
        s_publish.tx_buffer[7 - i + 24 * led_index] = g >> i & 0x01 ? DEVICE_WS2812_HIGH_LEVEL : DEVICE_WS2812_LOW_LEVEL;
        s_publish.tx_buffer[15 - i + 24 * led_index] = r >> i & 0x01 ? DEVICE_WS2812_HIGH_LEVEL : DEVICE_WS2812_LOW_LEVEL;
        s_publish.tx_buffer[23 - i + 24 * led_index] = b >> i & 0x01 ? DEVICE_WS2812_HIGH_LEVEL : DEVICE_WS2812_LOW_LEVEL;
    }

    message_center_publisher_push_message(s_pub, (void*)&s_publish);
}

void device_ws2812_flush(void)
{
    if (message_center_subscriber_get_message(
            s_sub, (void*)&s_subscribe)) {
        if (hal_spi_get_state(&s_spi_ctx) == HAL_SPI_ST_READY) {
            if (hal_spi_get_dma_state(&s_spi_ctx) == HAL_SPI_DMA_ST_READY) {
                hal_spi_transmit_dma(&s_spi_ctx,
                    (uint8_t*)&s_subscribe, sizeof(s_subscribe));
            }
        }
    }
}

void device_ws2812_loop(void)
{
    static uint32_t times, times_last, diff_times;
    times_last = times;
    times = millis();
    diff_times = times - times_last;
    switch (s_mode) {
    case DEVICE_WS2812_MODE_FLOW:
        if (s_j < RGB_FLOW_COLOR_CHANGE_TIME) {
            s_j += diff_times;
            s_alpha += s_delta_alpha * diff_times;
            s_red += s_delta_red * diff_times;
            s_green += s_delta_green * diff_times;
            s_blue += s_delta_blue * diff_times;
            s_argb = (uint32_t)s_alpha << 24 | (uint32_t)s_red << 16 | (uint32_t)s_green << 8 | (uint32_t)s_blue << 0;
            argb_led_show(s_argb);
        } else {
            s_j = 0;
            s_i++;
        }
        if (s_i >= sizeof(s_rgb_flow_color) / sizeof(s_rgb_flow_color[0]) - 1) {
            s_i = 0;
        }
        if (s_i != s_last_i) {
            s_last_i = s_i;
            s_alpha = (float)((s_rgb_flow_color[s_i] & 0xFF000000) >> 24);
            s_red = (float)((s_rgb_flow_color[s_i] & 0x00FF0000) >> 16);
            s_green = (float)((s_rgb_flow_color[s_i] & 0x0000FF00) >> 8);
            s_blue = (float)((s_rgb_flow_color[s_i] & 0x000000FF) >> 0);
            s_delta_alpha = (float)((s_rgb_flow_color[s_i + 1] & 0xFF000000) >> 24) - (float)((s_rgb_flow_color[s_i] & 0xFF000000) >> 24);
            s_delta_red = (float)((s_rgb_flow_color[s_i + 1] & 0x00FF0000) >> 16) - (float)((s_rgb_flow_color[s_i] & 0x00FF0000) >> 16);
            s_delta_green = (float)((s_rgb_flow_color[s_i + 1] & 0x0000FF00) >> 8) - (float)((s_rgb_flow_color[s_i] & 0x0000FF00) >> 8);
            s_delta_blue = (float)((s_rgb_flow_color[s_i + 1] & 0x000000FF) >> 0) - (float)((s_rgb_flow_color[s_i] & 0x000000FF) >> 0);

            s_delta_alpha /= RGB_FLOW_COLOR_CHANGE_TIME;
            s_delta_red /= RGB_FLOW_COLOR_CHANGE_TIME;
            s_delta_green /= RGB_FLOW_COLOR_CHANGE_TIME;
            s_delta_blue /= RGB_FLOW_COLOR_CHANGE_TIME;
        }
        break;
    case DEVICE_WS2812_MODE_ERROR_CODE:
        led_blink_error(s_error_code);
        break;
    default:
        break;
    }
}

void device_ws2812_mode_set(device_ws2812_mode_t mode, uint8_t num)
{
    s_mode = mode;
    s_error_code = num;
}

/* Private functions ---------------------------------------------------------*/

static void led_set(const uint8_t state)
{
    if (state)
        argb_led_show(0x0FFF0000);
    else
        argb_led_show(0x00000000);
}

static void argb_led_show(const uint32_t rgb)
{
    float alpha_b = 0.0f;
    uint8_t _red, _green, _blue;
    alpha_b = (float)((rgb & 0xFF000000) >> 24) / 0xFF;
    _red = (uint8_t)((float)((rgb & 0x00FF0000) >> 16) * alpha_b);
    _green = (uint8_t)((float)((rgb & 0x0000FF00) >> 8) * alpha_b);
    _blue = (uint8_t)((float)((rgb & 0x000000FF) >> 0) * alpha_b);
    device_ws2812_ctrl(0, _red, _green, _blue);
}

static void led_blink_error(const uint8_t num)
{
    static uint16_t show_num = 0;
    static uint16_t stop_num = 100;
    if (show_num == 0 && stop_num == 0) {
        show_num = num;
        stop_num = STOP_CNT;
    } else if (show_num == 0) {
        stop_num--;
        led_set(0);
    } else {
        static uint16_t tick = 0;
        tick++;
        if (tick < BLINK_CNT / 2) {
            led_set(0);
        } else if (tick < BLINK_CNT) {
            led_set(1);
        } else {
            tick = 0;
            show_num--;
        }
    }
}
