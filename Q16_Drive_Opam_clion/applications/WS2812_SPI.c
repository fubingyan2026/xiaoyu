/**
 * @brief: WS2812 驱动
 * @FilePath: WS2812_SPI.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-04-13 22:31:51
 * @LastEditTime: 2025-09-26 20:58:36
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "WS2812_SPI.h"

#include "bsp_delay.h"
#include "debug.h"
#include "memory_pool/memory_pool.h"
#include "message_center.h"
#include "spi.h"

#define RGB_FLOW_COLOR_CHANGE_TIME 500

ws2812_mode_e ws2812_mode = 0;
static uint8_t error_code = 0;
// blue-> green(dark)-> red -> blue(dark) -> green(dark) -> red(dark) -> blue
// 蓝 -> 绿(灭) -> 红 -> 蓝(灭) -> 绿 -> 红(灭) -> 蓝
static const uint32_t RGB_flow_color[] = {0x0F0000FF, 0x0F00FF00, 0x0FFF0000,
                                          0x0F0000FF, 0x0F00FF00, 0x0FFF0000,
                                          0x0F0000FF};

static uint16_t i, j;
static uint16_t Last_I;
static float delta_alpha, delta_red, delta_green, delta_blue;
static float alpha, red, green, blue;
static uint32_t aRGB;

#define STOP_CNT 200   // 暂停的数值
#define BLINK_CNT 100  // 每次闪烁的数值
static uint8_t error, last_error;
static uint8_t error_num;

static void aRGB_led_show(const uint32_t _aRGB);
static void led_blink_error(const uint8_t num);

ws2812b_t ws2812b_publish = {0}, ws2812b_subscribe = {0};

message_center_publisher_t* pub_ws2812 = NULL;
message_center_subscriber_t* sub_ws2812 = NULL;

/**
 * @brief Sets the state of the LED.
 *
 * @param state The new state of the LED, where 0 represents off and 1
 * represents on.
 */
static void led_set(const uint8_t state) {
  if (state)
    aRGB_led_show(0x0FFF0000);  // 红色
  else
    aRGB_led_show(0x00000000);
}

void WS2812_SPI_Init(void) {
  /* 配置消息中心 */
  message_center_config_t config = {
      .name = "ws2812",
      .data_len = sizeof(ws2812b_t),
      .queue_size = 4,
      .max_topics = MESSAGE_CENTER_MAX_TOPICS,
      .max_topic_name_len = MESSAGE_CENTER_MAX_TOPIC_NAME_LEN,
  };

  // 注册发布者
  message_center_error_t err =
      message_center_publisher_register(&pub_ws2812, config);
  DEBUG_ASSERT(err == MESSAGE_CENTER_OK);

  // 注册订阅者
  err = message_center_subscriber_register(pub_ws2812, &sub_ws2812);
  DEBUG_ASSERT(err == MESSAGE_CENTER_OK);
}

/**
 * @brief          控制WS2812 LED显示指定颜色
 * @details        该函数通过SPI接口控制WS2812
 * LED显示指定的红、绿、蓝三色。它将输入的RGB值转换为WS2812
 * LED所需的格式，并通过SPI发送数据。
 *                 函数首先根据输入的RGB值生成一个24位的数据缓冲区，然后通过中断方式发送这些数据到SPI接口，最后等待SPI传输完成。
 * @param led_index led index
 * @param r        红色分量（0-255）
 * @param g        绿色分量（0-255）
 * @param b        蓝色分量（0-255）
 * @retval         none
 */
void WS2812_Ctrl(const uint16_t led_index, const uint8_t r, const uint8_t g,
                 const uint8_t b) {
  for (int i = 0; i < 8; i++) {
    ws2812b_publish.tx_buffer[7 - i + 24 * led_index] =
        g >> i & 0x01 ? WS2812_HighLevel : WS2812_LowLevel;
    ws2812b_publish.tx_buffer[15 - i + 24 * led_index] =
        r >> i & 0x01 ? WS2812_HighLevel : WS2812_LowLevel;
    ws2812b_publish.tx_buffer[23 - i + 24 * led_index] =
        b >> i & 0x01 ? WS2812_HighLevel : WS2812_LowLevel;
  }

  message_center_publisher_push_message(pub_ws2812,
                                        (void*)&ws2812b_publish);  // 发布消息
}

/**
 * @brief          WS2812 LED刷新函数
 * @details        该函数用于刷新WS2812
 * LED的显示状态。它检查是否有新的颜色数据需要发送到LED，如果有，则通过SPI接口以DMA方式传输数据。
 *                 函数首先从订阅者获取最新的颜色数据，如果获取成功且SPI和DMA均处于空闲状态，则启动DMA传输，将颜色数据发送到WS2812
 * LED。 该函数应定期调用以确保LED显示最新的颜色状态。
 * @retval         none
 */
void WS2812Flush(void) {
  if (message_center_subscriber_get_message(
          sub_ws2812, (void*)&ws2812b_subscribe))  // 有新消息就发布
  {
    if (WS2812_SPI_UNIT.State == HAL_SPI_STATE_READY) {
      if (WS2812_SPI_UNIT.hdmatx->State == HAL_DMA_STATE_READY) {
        HAL_SPI_Transmit_DMA(&WS2812_SPI_UNIT, (uint8_t*)&ws2812b_subscribe,
                             sizeof(ws2812b_subscribe));
      }
    }
  }
}

/**
 * @brief          显示带透明度的RGB颜色
 * @details        该函数用于在WS2812
 * LED上显示带透明度的RGB颜色。它根据输入的ARGB值计算实际显示的RGB值，并调用WS2812_Ctrl函数设置LED颜色。
 *                 函数首先提取ARGB值中的透明度和RGB分量，然后根据透明度调整RGB分量的亮度，最后调用WS2812_Ctrl函数将计算后的RGB值发送到LED。
 * @param _aRGB   带透明度的ARGB颜色值（格式：0xAARRGGBB）
 * @retval         none
 */
static void aRGB_led_show(const uint32_t _aRGB) {
  static float alpha_B = 0.0f;
  static uint8_t _red, _green, _blue;
  alpha_B = (float)((_aRGB & 0xFF000000) >> 24) / 0xFF;
  _red = (uint8_t)((float)((_aRGB & 0x00FF0000) >> 16) * alpha_B);
  _green = (uint8_t)((float)((_aRGB & 0x0000FF00) >> 8) * alpha_B);
  _blue = (uint8_t)((float)((_aRGB & 0x000000FF) >> 0) * alpha_B);
  WS2812_Ctrl(0, _red, _green, _blue);
}

/**
 * @brief          WS2812 LED颜色流动效果循环函数
 * @details        该函数实现了WS2812
 * LED的颜色流动效果。它通过逐渐改变LED的颜色值，创建出一种平滑的颜色过渡效果。
 *                 函数使用一个预定义的颜色数组，按顺序循环显示这些颜色，并在每次颜色变化时计算出中间过渡色。
 *                 该函数应定期调用以实现连续的颜色流动效果。
 * @retval         none
 */
void WS2812_SPI_Loop(void) {
  static uint32_t times, times_last, diff_times;
  times_last = times;
  times = millis();
  diff_times = times - times_last;
  switch (ws2812_mode) {
    case WS2812_MODE_FLOW:
      if (j < RGB_FLOW_COLOR_CHANGE_TIME) {
        j += diff_times;
        alpha += delta_alpha * diff_times;
        red += delta_red * diff_times;
        green += delta_green * diff_times;
        blue += delta_blue * diff_times;
        aRGB = (uint32_t)alpha << 24 | (uint32_t)red << 16 |
               (uint32_t)green << 8 | (uint32_t)blue << 0;
        aRGB_led_show(aRGB);
      } else {
        j = 0;
        i++;
      }
      if (i >= sizeof(RGB_flow_color) / sizeof(RGB_flow_color[0]) - 1) {
        i = 0;
      }
      if (i != Last_I) {
        Last_I = i;
        alpha = (float)((RGB_flow_color[i] & 0xFF000000) >> 24);
        red = (float)((RGB_flow_color[i] & 0x00FF0000) >> 16);
        green = (float)((RGB_flow_color[i] & 0x0000FF00) >> 8);
        blue = (float)((RGB_flow_color[i] & 0x000000FF) >> 0);
        delta_alpha = (float)((RGB_flow_color[i + 1] & 0xFF000000) >> 24) -
                      (float)((RGB_flow_color[i] & 0xFF000000) >> 24);
        delta_red = (float)((RGB_flow_color[i + 1] & 0x00FF0000) >> 16) -
                    (float)((RGB_flow_color[i] & 0x00FF0000) >> 16);
        delta_green = (float)((RGB_flow_color[i + 1] & 0x0000FF00) >> 8) -
                      (float)((RGB_flow_color[i] & 0x0000FF00) >> 8);
        delta_blue = (float)((RGB_flow_color[i + 1] & 0x000000FF) >> 0) -
                     (float)((RGB_flow_color[i] & 0x000000FF) >> 0);

        delta_alpha /= RGB_FLOW_COLOR_CHANGE_TIME;
        delta_red /= RGB_FLOW_COLOR_CHANGE_TIME;
        delta_green /= RGB_FLOW_COLOR_CHANGE_TIME;
        delta_blue /= RGB_FLOW_COLOR_CHANGE_TIME;
      }
      break;
    case WS2812_MODE_ERROR_CODE:
      led_blink_error(error_code);
      break;
    default:
      break;
  }
}

/**
 * @brief 设置WS2812工作模式
 * @param _mode 工作模式
 * @param _num 错误代码编号
 */
void WS2812_Mode_Set(ws2812_mode_e _mode, uint8_t _num) {
  ws2812_mode = _mode;
  error_code = _num;
}

/**
 * @brief 使LED以错误模式闪烁
 * @param num 错误编号，用于确定LED闪烁的次数
 * @return void
 */
static void led_blink_error(const uint8_t num) {
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
