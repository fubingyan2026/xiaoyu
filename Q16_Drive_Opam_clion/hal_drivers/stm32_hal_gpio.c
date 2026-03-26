//
// Created by fubingyan on 25-9-20.
//

/**
 * @file    stm32_hal.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-20
 * @brief   STM32平台硬件抽象层实现
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
#include "main.h" // 根据实际使用的STM32系列调整

/* Private variables ---------------------------------------------------------*/
static GPIO_TypeDef *port_map[HAL_GPIO_PORT_LEN] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG,
};

/* Private function prototypes -----------------------------------------------*/
static void stm32_gpio_init(const hal_gpio_config_t *config);
static void stm32_gpio_write(uint8_t port, uint8_t pin, hal_gpio_pin_state_e state);
static hal_gpio_pin_state_e stm32_gpio_read(uint8_t port, uint8_t pin);
static void stm32_gpio_toggle(uint8_t port, uint8_t pin);

/* GPIO操作函数结构体 */
static const hal_gpio_ops_t stm32_gpio_ops = {.init = stm32_gpio_init, .write = stm32_gpio_write, .read = stm32_gpio_read, .toggle = stm32_gpio_toggle};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化STM32平台HAL
 */
void platform_hal_init(void)
{
    /* 启用GPIO时钟 */

    /* 设置GPIO操作函数 */
    hal_gpio_set_ops(&stm32_gpio_ops);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  初始化GPIO
 */
static void stm32_gpio_init(const hal_gpio_config_t *config)
{
    if (config == NULL || config->port >= sizeof(port_map) / sizeof(port_map[0]))
    {
        return;
    }
    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /**TIM17 GPIO Configuration
    PB9     ------> TIM17_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    if (config->direction == HAL_GPIO_DIRECTION_INPUT)
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    if (config->direction == HAL_GPIO_DIRECTION_OUTPUT)
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_DeInit(port_map[config->port], 1 << config->pin);
    HAL_GPIO_Init(port_map[config->port], &GPIO_InitStruct);
    /* 设置默认状态 */
    if (config->direction == HAL_GPIO_DIRECTION_OUTPUT)
    {
        stm32_gpio_write(config->port, config->pin, config->default_state);
    }
}

/**
 * @brief  写入GPIO引脚状态
 */
static void stm32_gpio_write(uint8_t port, uint8_t pin, hal_gpio_pin_state_e state)
{
    if (port >= sizeof(port_map) / sizeof(port_map[0]))
    {
        return;
    }

    HAL_GPIO_WritePin(port_map[port], 1 << pin, (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief  读取GPIO引脚状态
 */
static hal_gpio_pin_state_e stm32_gpio_read(uint8_t port, uint8_t pin)
{
    if (port >= sizeof(port_map) / sizeof(port_map[0]))
    {
        return HAL_GPIO_PIN_RESET;
    }

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port_map[port], 1 << pin);
    return (pin_state == GPIO_PIN_SET) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

/**
 * @brief  切换GPIO引脚状态
 */
static void stm32_gpio_toggle(uint8_t port, uint8_t pin)
{
    if (port >= sizeof(port_map) / sizeof(port_map[0]))
    {
        return;
    }

    HAL_GPIO_TogglePin(port_map[port], 1 << pin);
}