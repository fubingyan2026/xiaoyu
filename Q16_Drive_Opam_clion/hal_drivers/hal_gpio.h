//
// Created by fubingyan on 25-9-20.
//

#ifndef __HAL_GPIO_H
#define __HAL_GPIO_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"
#include <stdbool.h>
#include <stdint.h>

    /* Exported types ------------------------------------------------------------*/

    /**
     * @brief GPIO引脚电平定义
     */
    typedef enum
    {
        HAL_GPIO_PIN_RESET = 0, /**< 低电平 */
        HAL_GPIO_PIN_SET = 1,   /**< 高电平 */
    } hal_gpio_pin_state_e;

    /**
     * @brief GPIO方向定义
     */
    typedef enum
    {
        HAL_GPIO_DIRECTION_INPUT = 0, /**< 输入模式 */
        HAL_GPIO_DIRECTION_OUTPUT     /**< 输出模式 */
    } hal_gpio_direction_e;

    typedef enum
    {
        HAL_GPIO_PORT_A = 0,
        HAL_GPIO_PORT_B,
        HAL_GPIO_PORT_C,
        HAL_GPIO_PORT_D,
        HAL_GPIO_PORT_E,
        HAL_GPIO_PORT_F,
        HAL_GPIO_PORT_G,
        HAL_GPIO_PORT_LEN,
    } gpio_port_e;

    /**
     * @brief GPIO引脚配置结构体
     */
    typedef struct
    {
        gpio_port_e port;                   /**< GPIO端口 */
        uint8_t pin;                        /**< GPIO引脚 */
        hal_gpio_direction_e direction;     /**< 方向 */
        hal_gpio_pin_state_e default_state; /**< 默认状态 */
    } hal_gpio_config_t;

    /**
     * @brief GPIO操作函数指针结构体
     */
    typedef struct
    {
        void (*init)(const hal_gpio_config_t *config);

        void (*write)(uint8_t port, uint8_t pin, hal_gpio_pin_state_e state);

        hal_gpio_pin_state_e (*read)(uint8_t port, uint8_t pin);

        void (*toggle)(uint8_t port, uint8_t pin);
    } hal_gpio_ops_t;

    /* Exported constants --------------------------------------------------------*/
    /* Exported macro ------------------------------------------------------------*/
    /* Exported functions prototypes ---------------------------------------------*/

    /**
     * @brief  设置GPIO操作函数
     * @param  ops: GPIO操作函数结构体指针
     * @retval None
     */
    void hal_gpio_set_ops(const hal_gpio_ops_t *ops);

    /**
     * @brief  初始化GPIO
     * @param  config: GPIO配置指针
     * @retval None
     */
    void hal_gpio_init(const hal_gpio_config_t *config);

    /**
     * @brief  写入GPIO引脚状态
     * @param  port: GPIO端口
     * @param  pin: GPIO引脚
     * @param  state: 引脚状态
     * @retval None
     */
    void hal_gpio_write(uint8_t port, uint8_t pin, hal_gpio_pin_state_e state);

    /**
     * @brief  读取GPIO引脚状态
     * @param  port: GPIO端口
     * @param  pin: GPIO引脚
     * @retval 引脚状态
     */
    hal_gpio_pin_state_e hal_gpio_read(uint8_t port, uint8_t pin);

    /**
     * @brief  切换GPIO引脚状态
     * @param  port: GPIO端口
     * @param  pin: GPIO引脚
     * @retval None
     */
    void hal_gpio_toggle(uint8_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H */