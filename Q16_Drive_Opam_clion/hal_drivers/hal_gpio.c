//
// Created by fubingyan on 25-9-20.
//
/**
 * @file    hal_gpio.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-20
 * @brief   硬件抽象层 - GPIO实现
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

/* Private variables ---------------------------------------------------------*/
static const hal_gpio_ops_t* gpio_ops = NULL;
static uint8_t gpio_initialized = 0;

extern void platform_hal_init(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置GPIO操作函数
 * @param  ops: GPIO操作函数结构体指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_set_ops(const hal_gpio_ops_t* ops) {
  if (ops == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  if (ops->init == NULL || ops->write == NULL || ops->read == NULL ||
      ops->toggle == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  gpio_ops = ops;
  return HAL_GPIO_OK;
}

/**
 * @brief  初始化GPIO
 * @param  config: GPIO配置结构体指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_init(const hal_gpio_config_t* config) {
  if (config == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  if (!gpio_initialized) {
    platform_hal_init();
    gpio_initialized = 1;
  }

  if (gpio_ops == NULL || gpio_ops->init == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  gpio_ops->init(config);
  return HAL_GPIO_OK;
}

/**
 * @brief  写入GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  state: 引脚状态
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_write(uint8_t port, uint8_t pin,
                                hal_gpio_pin_state_t state) {
  if (gpio_ops == NULL || gpio_ops->write == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  if (port >= HAL_GPIO_PORT_LEN || pin > 15) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  gpio_ops->write(port, pin, state);
  return HAL_GPIO_OK;
}

/**
 * @brief  读取GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  state: 指向状态存储的指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_read(uint8_t port, uint8_t pin,
                               hal_gpio_pin_state_t* state) {
  if (gpio_ops == NULL || gpio_ops->read == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  if (port >= HAL_GPIO_PORT_LEN || pin > 15 || state == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  *state = gpio_ops->read(port, pin);
  return HAL_GPIO_OK;
}

/**
 * @brief  切换GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_toggle(uint8_t port, uint8_t pin) {
  if (gpio_ops == NULL || gpio_ops->toggle == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
  }

  if (port >= HAL_GPIO_PORT_LEN || pin > 15) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
  }

  gpio_ops->toggle(port, pin);
  return HAL_GPIO_OK;
}