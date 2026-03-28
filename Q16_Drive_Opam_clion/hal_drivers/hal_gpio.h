//
// Created by fubingyan on 25-9-20.
//

#ifndef __HAL_GPIO_H
#define __HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief GPIO错误码定义
 */
typedef enum {
  HAL_GPIO_OK = 0,              /**< 成功 */
  HAL_GPIO_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_GPIO_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_GPIO_ERROR_HARDWARE,      /**< 硬件错误 */
} hal_gpio_error_t;

/**
 * @brief GPIO引脚电平定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PIN_RESET = 0, /**< 低电平 */
  HAL_GPIO_PIN_SET = 1,   /**< 高电平 */
} hal_gpio_pin_state_t;

/**
 * @brief GPIO方向定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_DIRECTION_INPUT = 0, /**< 输入模式 */
  HAL_GPIO_DIRECTION_OUTPUT     /**< 输出模式 */
} hal_gpio_direction_t;

/**
 * @brief GPIO端口定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PORT_A = 0,
  HAL_GPIO_PORT_B,
  HAL_GPIO_PORT_C,
  HAL_GPIO_PORT_D,
  HAL_GPIO_PORT_E,
  HAL_GPIO_PORT_F,
  HAL_GPIO_PORT_G,
  HAL_GPIO_PORT_LEN,
} gpio_port_t;

/**
 * @brief GPIO上拉/下拉定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PULL_NONE = 0, /**< 无上拉下拉 */
  HAL_GPIO_PULL_UP = 1,   /**< 上拉 */
  HAL_GPIO_PULL_DOWN = 2, /**< 下拉 */
} hal_gpio_pull_t;

/**
 * @brief GPIO速度定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_SPEED_FREQ_LOW = 0,       /**< 低频率 */
  HAL_GPIO_SPEED_FREQ_MEDIUM = 1,    /**< 中频率 */
  HAL_GPIO_SPEED_FREQ_HIGH = 2,      /**< 高频率 */
  HAL_GPIO_SPEED_FREQ_VERY_HIGH = 3, /**< 非常高频率 */
} hal_gpio_speed_t;

/**
 * @brief GPIO引脚定义
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PIN_0 = 0,
  HAL_GPIO_PIN_1,
  HAL_GPIO_PIN_2,
  HAL_GPIO_PIN_3,
  HAL_GPIO_PIN_4,
  HAL_GPIO_PIN_5,
  HAL_GPIO_PIN_6,
  HAL_GPIO_PIN_7,
  HAL_GPIO_PIN_8,
  HAL_GPIO_PIN_9,
  HAL_GPIO_PIN_10,
  HAL_GPIO_PIN_11,
  HAL_GPIO_PIN_12,
  HAL_GPIO_PIN_13,
  HAL_GPIO_PIN_14,
  HAL_GPIO_PIN_15,
} hal_gpio_pin_t;

/**
 * @brief GPIO引脚配置结构体
 */
typedef struct {
  gpio_port_t port;                   /**< GPIO端口 */
  hal_gpio_pin_t pin;                 /**< GPIO引脚 */
  hal_gpio_direction_t direction;     /**< 方向 */
  hal_gpio_pin_state_t default_state; /**< 默认状态 */
  hal_gpio_pin_state_t active_level;  /**< 活动电平 */
  hal_gpio_pull_t pull;               /**< 上拉下拉 */
  hal_gpio_speed_t speed;             /**< 速度 */
} hal_gpio_config_t;

/**
 * @brief GPIO操作函数指针结构体
 */
typedef struct {
  void (*init)(const hal_gpio_config_t *config);

  void (*write)(uint8_t port, uint8_t pin, hal_gpio_pin_state_t state);

  hal_gpio_pin_state_t (*read)(uint8_t port, uint8_t pin);

  void (*toggle)(uint8_t port, uint8_t pin);
} hal_gpio_ops_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  设置GPIO操作函数
 * @param  ops: GPIO操作函数结构体指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_set_ops(const hal_gpio_ops_t *ops);

/**
 * @brief  初始化GPIO
 * @param  config: GPIO配置指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_init(const hal_gpio_config_t *config);

/**
 * @brief  写入GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  state: 引脚状态
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_write(uint8_t port, uint8_t pin,
                                hal_gpio_pin_state_t state);

/**
 * @brief  读取GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @param  state: 指向状态存储的指针
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_read(uint8_t port, uint8_t pin,
                               hal_gpio_pin_state_t *state);

/**
 * @brief  切换GPIO引脚状态
 * @param  port: GPIO端口
 * @param  pin: GPIO引脚
 * @return HAL_GPIO_OK 成功，其他值为错误码
 */
hal_gpio_error_t hal_gpio_toggle(uint8_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H */