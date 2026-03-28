//
// Created by fubingyan on 25-9-20.
//

/**
 * @file    stm32_hal_gpio.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-20
 * @brief   STM32平台硬件抽象层 - GPIO实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "hal_gpio.h"
#include "main.h"

/* Private variables ---------------------------------------------------------*/
static GPIO_TypeDef *const port_map[HAL_GPIO_PORT_LEN] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG,
};

#define PORT_MAP_SIZE (sizeof(port_map) / sizeof(port_map[0]))

typedef struct {
  hal_gpio_context_t *ctx;
  uint8_t port;
} exti_callback_info_t;

static exti_callback_info_t exti_callbacks[16] = {0};

/* Private function prototypes -----------------------------------------------*/
static hal_gpio_error_t stm32_gpio_init(hal_gpio_context_t *ctx,
                                        const hal_gpio_config_t *config);
static hal_gpio_error_t stm32_gpio_deinit(hal_gpio_context_t *ctx, uint8_t port,
                                          uint8_t pin);
static hal_gpio_error_t stm32_gpio_write(hal_gpio_context_t *ctx, uint8_t port,
                                         uint8_t pin,
                                         hal_gpio_pin_state_t state);
static hal_gpio_error_t stm32_gpio_read(hal_gpio_context_t *ctx, uint8_t port,
                                        uint8_t pin,
                                        hal_gpio_pin_state_t *state);
static hal_gpio_error_t stm32_gpio_toggle(hal_gpio_context_t *ctx, uint8_t port,
                                          uint8_t pin);
static hal_gpio_error_t stm32_gpio_register_callback(hal_gpio_context_t *ctx,
                                                     uint8_t port, uint8_t pin,
                                                     hal_gpio_callback_t callback,
                                                     void *user_data);

static void stm32_gpio_enable_clock(uint8_t port);
static uint32_t stm32_gpio_get_mode(hal_gpio_mode_t mode);
static uint32_t stm32_gpio_get_pull(hal_gpio_pull_t pull);
static uint32_t stm32_gpio_get_speed(hal_gpio_speed_t speed);
static uint8_t stm32_gpio_get_af(hal_gpio_af_t af);

/* GPIO操作函数结构体 */
static const hal_gpio_ops_t stm32_gpio_ops = {
    .init = stm32_gpio_init,
    .deinit = stm32_gpio_deinit,
    .write = stm32_gpio_write,
    .read = stm32_gpio_read,
    .toggle = stm32_gpio_toggle,
    .register_callback = stm32_gpio_register_callback,
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化GPIO上下文并设置操作函数
 * @param  ctx: GPIO上下文指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t stm32_gpio_init_context(hal_gpio_context_t *ctx) {
  if (ctx == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  ctx->ops = &stm32_gpio_ops;
  ctx->initialized = 0;
  ctx->callback = NULL;
  ctx->user_data = NULL;

  return HAL_GPIO_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  使能GPIO端口时钟
 * @param  port: GPIO端口索引
 */
static void stm32_gpio_enable_clock(uint8_t port) {
  switch (port) {
    case HAL_GPIO_PORT_A:
      __HAL_RCC_GPIOA_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_B:
      __HAL_RCC_GPIOB_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_C:
      __HAL_RCC_GPIOC_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_D:
      __HAL_RCC_GPIOD_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_E:
      __HAL_RCC_GPIOE_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_F:
      __HAL_RCC_GPIOF_CLK_ENABLE();
      break;
    case HAL_GPIO_PORT_G:
      __HAL_RCC_GPIOG_CLK_ENABLE();
      break;
    default:
      break;
  }
}

/**
 * @brief  将HAL模式转换为STM32 HAL模式
 * @param  mode: HAL模式
 * @return STM32 HAL模式值
 */
static uint32_t stm32_gpio_get_mode(hal_gpio_mode_t mode) {
  switch (mode) {
    case HAL_GPIO_MODE_INPUT:
      return GPIO_MODE_INPUT;
    case HAL_GPIO_MODE_OUTPUT_PP:
      return GPIO_MODE_OUTPUT_PP;
    case HAL_GPIO_MODE_OUTPUT_OD:
      return GPIO_MODE_OUTPUT_OD;
    case HAL_GPIO_MODE_AF_PP:
      return GPIO_MODE_AF_PP;
    case HAL_GPIO_MODE_AF_OD:
      return GPIO_MODE_AF_OD;
    case HAL_GPIO_MODE_ANALOG:
      return GPIO_MODE_ANALOG;
    case HAL_GPIO_MODE_IT_RISING:
      return GPIO_MODE_IT_RISING;
    case HAL_GPIO_MODE_IT_FALLING:
      return GPIO_MODE_IT_FALLING;
    case HAL_GPIO_MODE_IT_RISING_FALLING:
      return GPIO_MODE_IT_RISING_FALLING;
    case HAL_GPIO_MODE_EVT_RISING:
      return GPIO_MODE_EVT_RISING;
    case HAL_GPIO_MODE_EVT_FALLING:
      return GPIO_MODE_EVT_FALLING;
    case HAL_GPIO_MODE_EVT_RISING_FALLING:
      return GPIO_MODE_EVT_RISING_FALLING;
    default:
      return GPIO_MODE_INPUT;
  }
}

/**
 * @brief  将HAL上下拉转换为STM32 HAL上下拉
 * @param  pull: HAL上下拉
 * @return STM32 HAL上下拉值
 */
static uint32_t stm32_gpio_get_pull(hal_gpio_pull_t pull) {
  switch (pull) {
    case HAL_GPIO_PULL_NONE:
      return GPIO_NOPULL;
    case HAL_GPIO_PULL_UP:
      return GPIO_PULLUP;
    case HAL_GPIO_PULL_DOWN:
      return GPIO_PULLDOWN;
    default:
      return GPIO_NOPULL;
  }
}

/**
 * @brief  将HAL速度转换为STM32 HAL速度
 * @param  speed: HAL速度
 * @return STM32 HAL速度值
 */
static uint32_t stm32_gpio_get_speed(hal_gpio_speed_t speed) {
  switch (speed) {
    case HAL_GPIO_SPEED_FREQ_LOW:
      return GPIO_SPEED_FREQ_LOW;
    case HAL_GPIO_SPEED_FREQ_MEDIUM:
      return GPIO_SPEED_FREQ_MEDIUM;
    case HAL_GPIO_SPEED_FREQ_HIGH:
      return GPIO_SPEED_FREQ_HIGH;
    case HAL_GPIO_SPEED_FREQ_VERY_HIGH:
      return GPIO_SPEED_FREQ_VERY_HIGH;
    default:
      return GPIO_SPEED_FREQ_LOW;
  }
}

/**
 * @brief  将HAL AF转换为STM32 HAL AF
 * @param  af: HAL AF值
 * @return STM32 HAL AF值
 */
static uint8_t stm32_gpio_get_af(hal_gpio_af_t af) {
  return (uint8_t)af;
}

/**
 * @brief  初始化GPIO
 * @param  ctx: GPIO上下文指针
 * @param  config: GPIO配置结构体指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_init(hal_gpio_context_t *ctx,
                                        const hal_gpio_config_t *config) {
  if (config == NULL || config->port >= PORT_MAP_SIZE ||
      config->pin >= HAL_GPIO_PIN_MAX) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  stm32_gpio_enable_clock(config->port);

  GPIO_InitTypeDef gpio_init_struct = {0};

  gpio_init_struct.Pin = HAL_GPIO_PIN_MASK(config->pin);
  gpio_init_struct.Mode = stm32_gpio_get_mode(config->mode);
  gpio_init_struct.Pull = stm32_gpio_get_pull(config->pull);
  gpio_init_struct.Speed = stm32_gpio_get_speed(config->speed);

  if (config->mode == HAL_GPIO_MODE_AF_PP ||
      config->mode == HAL_GPIO_MODE_AF_OD) {
    gpio_init_struct.Alternate = stm32_gpio_get_af(config->alternate);
  }

  HAL_GPIO_Init(port_map[config->port], &gpio_init_struct);

  if (config->mode == HAL_GPIO_MODE_OUTPUT_PP ||
      config->mode == HAL_GPIO_MODE_OUTPUT_OD) {
    HAL_GPIO_WritePin(port_map[config->port], gpio_init_struct.Pin,
                      (config->default_state == HAL_GPIO_PIN_SET)
                          ? GPIO_PIN_SET
                          : GPIO_PIN_RESET);
  }

  return HAL_GPIO_OK;
}

/**
 * @brief  反初始化 GPIO
 * @param  ctx: GPIO上下文指针
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_deinit(hal_gpio_context_t *ctx,
                                          uint8_t port, uint8_t pin) {
  if (port >= PORT_MAP_SIZE || pin >= HAL_GPIO_PIN_MAX) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  HAL_GPIO_DeInit(port_map[port], HAL_GPIO_PIN_MASK(pin));
  
  exti_callbacks[pin].ctx = NULL;
  exti_callbacks[pin].port = 0;

  return HAL_GPIO_OK;
}

/**
 * @brief  写入 GPIO 引脚状态
 * @param  ctx: GPIO 上下文指针
 * @param  port: GPIO 端口
 * @param  pin: GPIO 引脚
 * @param  state: 引脚状态
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_write(hal_gpio_context_t *ctx,
                                         uint8_t port, uint8_t pin,
                                         hal_gpio_pin_state_t state) {
  if (port >= PORT_MAP_SIZE || pin >= HAL_GPIO_PIN_MAX) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  HAL_GPIO_WritePin(port_map[port], HAL_GPIO_PIN_MASK(pin),
                    (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET
                                                : GPIO_PIN_RESET);

  return HAL_GPIO_OK;
}

/**
 * @brief  读取 GPIO 引脚状态
 * @param  ctx: GPIO 上下文指针
 * @param  port: GPIO 端口
 * @param  pin: GPIO 引脚
 * @param  state: 指向状态存储的指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_read(hal_gpio_context_t *ctx,
                                        uint8_t port, uint8_t pin,
                                        hal_gpio_pin_state_t *state) {
  if (port >= PORT_MAP_SIZE || pin >= HAL_GPIO_PIN_MAX || state == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  GPIO_PinState pin_state =
      HAL_GPIO_ReadPin(port_map[port], HAL_GPIO_PIN_MASK(pin));
  *state = (pin_state == GPIO_PIN_SET) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;

  return HAL_GPIO_OK;
}

/**
 * @brief  切换 GPIO 引脚状态
 * @param  ctx: GPIO 上下文指针
 * @param  port: GPIO 端口
 * @param  pin: GPIO 引脚
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_toggle(hal_gpio_context_t *ctx,
                                          uint8_t port, uint8_t pin) {
  if (port >= PORT_MAP_SIZE || pin >= HAL_GPIO_PIN_MAX) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  HAL_GPIO_TogglePin(port_map[port], HAL_GPIO_PIN_MASK(pin));

  return HAL_GPIO_OK;
}

/**
 * @brief  注册 GPIO 中断回调函数
 * @param  ctx: GPIO上下文指针
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  callback: 回调函数指针
 * @param  user_data: 用户数据指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
static hal_gpio_error_t stm32_gpio_register_callback(hal_gpio_context_t *ctx,
                                                     uint8_t port, uint8_t pin,
                                                     hal_gpio_callback_t callback,
                                                     void *user_data) {
  if (port >= PORT_MAP_SIZE || pin >= HAL_GPIO_PIN_MAX) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  exti_callbacks[pin].ctx = ctx;
  exti_callbacks[pin].port = port;
  (void)callback;
  (void)user_data;

  return HAL_GPIO_OK;
}

/**
 * @brief  GPIO EXTI 中断处理函数
 * @param  GPIO_Pin: GPIO 引脚号
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  uint8_t pin_index = 0;

  for (uint8_t i = 0; i < 16; i++) {
    if (GPIO_Pin == (1U << i)) {
      pin_index = i;
      break;
    }
  }

  exti_callback_info_t *info = &exti_callbacks[pin_index];
  if (info->ctx != NULL && info->ctx->callback != NULL) {
    info->ctx->callback(info->ctx, info->port, pin_index, info->ctx->user_data);
  }
}
