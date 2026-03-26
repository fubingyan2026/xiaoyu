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
static const hal_gpio_ops_t *gpio_ops = NULL;

extern void platform_hal_init(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  设置GPIO操作函数
  */
void hal_gpio_set_ops(const hal_gpio_ops_t *ops)
{
    gpio_ops = ops;
}

/**
  * @brief  初始化GPIO
  */
void hal_gpio_init(const hal_gpio_config_t *config)
{
    static uint8_t init_flag = 1;
    if (init_flag)
        platform_hal_init();
    init_flag = 0;
    if (gpio_ops != NULL && gpio_ops->init != NULL) {
        gpio_ops->init(config);
    }
}

/**
  * @brief  写入GPIO引脚状态
  */
void hal_gpio_write(uint8_t port, uint8_t pin, hal_gpio_pin_state_e state)
{
    if (gpio_ops != NULL && gpio_ops->write != NULL) {
        gpio_ops->write(port, pin, state);
    }
}

/**
  * @brief  读取GPIO引脚状态
  */
hal_gpio_pin_state_e hal_gpio_read(uint8_t port, uint8_t pin)
{
    if (gpio_ops != NULL && gpio_ops->read != NULL) {
        return gpio_ops->read(port, pin);
    }
    return HAL_GPIO_PIN_RESET;
}

/**
  * @brief  切换GPIO引脚状态
  */
void hal_gpio_toggle(uint8_t port, uint8_t pin)
{
    if (gpio_ops != NULL && gpio_ops->toggle != NULL) {
        gpio_ops->toggle(port, pin);
    }
}