//
// Created by fubingyan on 25-9-20.
//
/**
 * @file    hal_gpio.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-20
 * @brief   硬件抽象层 - GPIO 实现
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

/* Private function prototypes -----------------------------------------------*/
static inline bool is_valid_port(uint8_t port);
static inline bool is_valid_pin(uint8_t pin);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 检查端口号是否有效
 * @param port 要检查的端口号
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_port(uint8_t port) {
  return port < HAL_GPIO_PORT_LEN;
}

/**
 * @brief 检查引脚号是否有效
 * @param pin 要检查的引脚号
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_pin(uint8_t pin) {
  return pin <= HAL_GPIO_PIN_MAX_VALUE;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置 GPIO 操作函数
 * @param  ctx GPIO 上下文指针
 * @param  ops GPIO 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_gpio_error_t hal_gpio_set_ops(hal_gpio_context_t* ctx,
                                  const hal_gpio_ops_t* ops) {
  // 检查参数有效性
  if (ctx == NULL || ops == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查必要的函数指针是否为 NULL
  if (ops->init == NULL || ops->deinit == NULL || ops->write == NULL ||
      ops->read == NULL || ops->toggle == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 进入临界区，保护共享资源
  HAL_GPIO_ENTER_CRITICAL();
  ctx->ops = ops;
  HAL_GPIO_EXIT_CRITICAL();

  return HAL_GPIO_OK;
}

/**
 * @brief  初始化 GPIO 引脚
 * @param  ctx GPIO 上下文指针
 * @param  config GPIO 配置结构体指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t* ctx,
                               const hal_gpio_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(config->port) || !is_valid_pin(config->pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 如果是第一次初始化，标记为已初始化
  if (!ctx->initialized) {
    ctx->initialized = 1;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的初始化函数
  HAL_GPIO_ENTER_CRITICAL();
  hal_gpio_error_t result = ctx->ops->init(ctx, config);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  反初始化 GPIO 引脚
 * @param  ctx GPIO 上下文指针
 * @param  port GPIO 端口号
 * @param  pin GPIO 引脚号
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_deinit(hal_gpio_context_t* ctx,
                                 uint8_t port,
                                 uint8_t pin) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(port) || !is_valid_pin(pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的反初始化函数
  HAL_GPIO_ENTER_CRITICAL();
  hal_gpio_error_t result = ctx->ops->deinit(ctx, port, pin);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  写入 GPIO 引脚状态
 * @param  ctx GPIO 上下文指针
 * @param  port GPIO 端口号
 * @param  pin GPIO 引脚号
 * @param  state 要设置的引脚状态
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_write(hal_gpio_context_t* ctx,
                                uint8_t port,
                                uint8_t pin,
                                hal_gpio_pin_state_t state) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(port) || !is_valid_pin(pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查状态值是否有效
  if (state != HAL_GPIO_PIN_RESET && state != HAL_GPIO_PIN_SET) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->write == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的写入函数
  HAL_GPIO_ENTER_CRITICAL();
  hal_gpio_error_t result = ctx->ops->write(ctx, port, pin, state);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  读取 GPIO 引脚状态
 * @param  ctx GPIO 上下文指针
 * @param  port GPIO 端口号
 * @param  pin GPIO 引脚号
 * @param  state 输出参数，用于存储读取到的引脚状态
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_read(hal_gpio_context_t* ctx,
                               uint8_t port,
                               uint8_t pin,
                               hal_gpio_pin_state_t* state) {
  // 检查参数有效性
  if (ctx == NULL || state == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(port) || !is_valid_pin(pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->read == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的读取函数
  HAL_GPIO_ENTER_CRITICAL();
  hal_gpio_error_t result = ctx->ops->read(ctx, port, pin, state);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  翻转 GPIO 引脚状态
 * @param  ctx GPIO 上下文指针
 * @param  port GPIO 端口号
 * @param  pin GPIO 引脚号
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_toggle(hal_gpio_context_t* ctx,
                                 uint8_t port,
                                 uint8_t pin) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(port) || !is_valid_pin(pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->toggle == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的翻转函数
  HAL_GPIO_ENTER_CRITICAL();
  hal_gpio_error_t result = ctx->ops->toggle(ctx, port, pin);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  注册 GPIO 中断回调函数
 * @param  ctx GPIO 上下文指针
 * @param  port GPIO 端口号
 * @param  pin GPIO 引脚号
 * @param  callback 中断回调函数指针
 * @param  user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_register_callback(hal_gpio_context_t* ctx,
                                            uint8_t port,
                                            uint8_t pin,
                                            hal_gpio_callback_t callback,
                                            void* user_data) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查端口和引脚号是否有效
  if (!is_valid_port(port) || !is_valid_pin(pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->register_callback == NULL) {
    return HAL_GPIO_ERROR_UNSUPPORTED;
  }

  // 进入临界区，保存回调函数和用户数据，然后调用平台特定的注册函数
  HAL_GPIO_ENTER_CRITICAL();
  ctx->callback = callback;
  ctx->user_data = user_data;
  hal_gpio_error_t result =
      ctx->ops->register_callback(ctx, port, pin, callback, user_data);
  HAL_GPIO_EXIT_CRITICAL();

  return result;
}
