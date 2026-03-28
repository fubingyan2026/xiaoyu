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
#include "main.h"  // 根据实际使用的STM32系列调整

/* Private variables ---------------------------------------------------------*/
static GPIO_TypeDef *port_map[HAL_GPIO_PORT_LEN] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG,
};

/* Private function prototypes -----------------------------------------------*/
static void stm32_gpio_init(const hal_gpio_config_t *config);
static void stm32_gpio_write(uint8_t port, uint8_t pin,
                             hal_gpio_pin_state_t state);
static hal_gpio_pin_state_t stm32_gpio_read(uint8_t port, uint8_t pin);
static void stm32_gpio_toggle(uint8_t port, uint8_t pin);

/* GPIO操作函数结构体 */
static const hal_gpio_ops_t stm32_gpio_ops = {.init = stm32_gpio_init,
                                              .write = stm32_gpio_write,
                                              .read = stm32_gpio_read,
                                              .toggle = stm32_gpio_toggle};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化STM32平台HAL
 */
void platform_hal_init(void) {
  /* 启用GPIO时钟 */

  /* 设置GPIO操作函数 */
  hal_gpio_set_ops(&stm32_gpio_ops);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  初始化GPIO
 * @param  config: GPIO配置结构体指针
 */
static void stm32_gpio_init(const hal_gpio_config_t *config) {
  if (config == NULL ||
      config->port >= sizeof(port_map) / sizeof(port_map[0]) ||
      config->pin > 15) {
    return;
  }

  // 只启用指定端口的时钟
  switch (config->port) {
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
      return;
  }

  GPIO_InitTypeDef gpio_init_struct = {0};

  // 使用正确的引脚定义
  gpio_init_struct.Pin = 1 << config->pin;

  if (config->direction == HAL_GPIO_DIRECTION_INPUT) {
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
  } else if (config->direction == HAL_GPIO_DIRECTION_OUTPUT) {
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
  }

  if (config->pull == HAL_GPIO_PULL_NONE) {
    gpio_init_struct.Pull = GPIO_NOPULL;
  } else if (config->pull == HAL_GPIO_PULL_UP) {
    gpio_init_struct.Pull = GPIO_PULLUP;
  } else if (config->pull == HAL_GPIO_PULL_DOWN) {
    gpio_init_struct.Pull = GPIO_PULLDOWN;
  }

  if (config->speed == HAL_GPIO_SPEED_FREQ_LOW) {
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
  } else if (config->speed == HAL_GPIO_SPEED_FREQ_MEDIUM) {
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  } else if (config->speed == HAL_GPIO_SPEED_FREQ_HIGH) {
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
  } else if (config->speed == HAL_GPIO_SPEED_FREQ_VERY_HIGH) {
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  }

  HAL_GPIO_DeInit(port_map[config->port], gpio_init_struct.Pin);
  HAL_GPIO_Init(port_map[config->port], &gpio_init_struct);

  /* 设置默认状态 */
  if (config->direction == HAL_GPIO_DIRECTION_OUTPUT) {
    stm32_gpio_write(config->port, config->pin, config->default_state);
  }
}

/**
 * @brief  写入GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  state: 引脚状态
 */
static void stm32_gpio_write(uint8_t port, uint8_t pin,
                             hal_gpio_pin_state_t state) {
  if (port >= sizeof(port_map) / sizeof(port_map[0]) || pin > 15) {
    return;
  }

  HAL_GPIO_WritePin(
      port_map[port], 1 << pin,
      (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief  读取GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @return 引脚状态
 */
static hal_gpio_pin_state_t stm32_gpio_read(uint8_t port, uint8_t pin) {
  if (port >= sizeof(port_map) / sizeof(port_map[0]) || pin > 15) {
    return HAL_GPIO_PIN_RESET;
  }

  GPIO_PinState pin_state = HAL_GPIO_ReadPin(port_map[port], 1 << pin);
  return (pin_state == GPIO_PIN_SET) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

/**
 * @brief  切换GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 */
static void stm32_gpio_toggle(uint8_t port, uint8_t pin) {
  if (port >= sizeof(port_map) / sizeof(port_map[0]) || pin > 15) {
    return;
  }

  HAL_GPIO_TogglePin(port_map[port], 1 << pin);
}