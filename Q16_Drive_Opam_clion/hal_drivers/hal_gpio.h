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
 * @brief GPIO 操作错误码枚举
 */
typedef enum {
  HAL_GPIO_OK = 0,              /**< 操作成功 */
  HAL_GPIO_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_GPIO_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_GPIO_ERROR_HARDWARE,      /**< 硬件错误 */
  HAL_GPIO_ERROR_BUSY,          /**< 设备忙 */
  HAL_GPIO_ERROR_UNSUPPORTED,   /**< 不支持的操作 */
} hal_gpio_error_t;

/**
 * @brief GPIO 引脚状态枚举
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PIN_RESET = 0, /**< 引脚低电平 */
  HAL_GPIO_PIN_SET = 1,   /**< 引脚高电平 */
} hal_gpio_pin_state_t;

/**
 * @brief GPIO 工作模式枚举
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_MODE_INPUT = 0,          /**< 输入模式 */
  HAL_GPIO_MODE_OUTPUT_PP,          /**< 推挽输出模式 */
  HAL_GPIO_MODE_OUTPUT_OD,          /**< 开漏输出模式 */
  HAL_GPIO_MODE_AF_PP,              /**< 推挽复用功能模式 */
  HAL_GPIO_MODE_AF_OD,              /**< 开漏复用功能模式 */
  HAL_GPIO_MODE_ANALOG,             /**< 模拟模式 */
  HAL_GPIO_MODE_IT_RISING,          /**< 上升沿中断触发 */
  HAL_GPIO_MODE_IT_FALLING,         /**< 下降沿中断触发 */
  HAL_GPIO_MODE_IT_RISING_FALLING,  /**< 双边沿中断触发 */
  HAL_GPIO_MODE_EVT_RISING,         /**< 上升沿事件触发 */
  HAL_GPIO_MODE_EVT_FALLING,        /**< 下降沿事件触发 */
  HAL_GPIO_MODE_EVT_RISING_FALLING, /**< 双边沿事件触发 */
} hal_gpio_mode_t;

/**
 * @brief GPIO 端口枚举
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PORT_A = 0, /**< GPIOA 端口 */
  HAL_GPIO_PORT_B,     /**< GPIOB 端口 */
  HAL_GPIO_PORT_C,     /**< GPIOC 端口 */
  HAL_GPIO_PORT_D,     /**< GPIOD 端口 */
  HAL_GPIO_PORT_E,     /**< GPIOE 端口 */
  HAL_GPIO_PORT_F,     /**< GPIOF 端口 */
  HAL_GPIO_PORT_G,     /**< GPIOG 端口 */
  HAL_GPIO_PORT_LEN,   /**< 端口数量 */
} gpio_port_t;

/**
 * @brief GPIO 上下拉配置枚举
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_PULL_NONE = 0, /**< 无上下拉 */
  HAL_GPIO_PULL_UP,       /**< 上拉 */
  HAL_GPIO_PULL_DOWN,     /**< 下拉 */
} hal_gpio_pull_t;

/**
 * @brief GPIO 速度配置枚举
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_SPEED_FREQ_LOW = 0,   /**< 低速 */
  HAL_GPIO_SPEED_FREQ_MEDIUM,    /**< 中速 */
  HAL_GPIO_SPEED_FREQ_HIGH,      /**< 高速 */
  HAL_GPIO_SPEED_FREQ_VERY_HIGH, /**< 超高速 */
} hal_gpio_speed_t;

/**
 * @brief GPIO 复用功能枚举 (AF0-AF15)
 */
typedef enum __attribute__((packed)) {
  HAL_GPIO_AF_0 = 0,
  HAL_GPIO_AF_1,
  HAL_GPIO_AF_2,
  HAL_GPIO_AF_3,
  HAL_GPIO_AF_4,
  HAL_GPIO_AF_5,
  HAL_GPIO_AF_6,
  HAL_GPIO_AF_7,
  HAL_GPIO_AF_8,
  HAL_GPIO_AF_9,
  HAL_GPIO_AF_10,
  HAL_GPIO_AF_11,
  HAL_GPIO_AF_12,
  HAL_GPIO_AF_13,
  HAL_GPIO_AF_14,
  HAL_GPIO_AF_15,
} hal_gpio_af_t;

/**
 * @brief GPIO 引脚号枚举
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
  HAL_GPIO_PIN_MAX,
} hal_gpio_pin_t;

/**
 * @brief 引脚掩码宏，将引脚号转换为位掩码
 * @param pin 引脚号 (0-15)
 * @return 对应的位掩码 (1 << pin)
 */
#define HAL_GPIO_PIN_MASK(pin) (1U << (pin))

/**
 * @brief GPIO 配置结构体
 */
typedef struct {
  gpio_port_t port;                   /**< GPIO 端口 */
  hal_gpio_pin_t pin;                 /**< GPIO 引脚号 */
  hal_gpio_mode_t mode;               /**< GPIO 工作模式 */
  hal_gpio_pin_state_t default_state; /**< 默认引脚状态（仅输出模式有效） */
  hal_gpio_pull_t pull;               /**< 上下拉配置 */
  hal_gpio_speed_t speed;             /**< 输出速度 */
  hal_gpio_af_t alternate;            /**< 复用功能选择 */
} hal_gpio_config_t;

/**
 * @brief GPIO 上下文结构体前向声明
 */
typedef struct hal_gpio_context hal_gpio_context_t;

/**
 * @brief GPIO 中断回调函数类型
 * @param ctx GPIO 上下文指针
 * @param port 触发中断的端口号
 * @param pin 触发中断的引脚号
 * @param user_data 用户自定义数据指针
 */
typedef void (*hal_gpio_callback_t)(hal_gpio_context_t *ctx, uint8_t port,
                                    uint8_t pin, void *user_data);

/**
 * @brief GPIO 上下文结构体
 *
 * 用于保存 GPIO 实例的状态信息，支持多实例操作。
 */
struct hal_gpio_context {
  const struct hal_gpio_ops *ops; /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized; /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_gpio_callback_t callback; /**< 中断回调函数指针 */
  void *user_data;              /**< 用户自定义数据 */
};

/**
 * @brief GPIO 操作函数结构体
 *
 * 定义了平台特定的 GPIO 操作接口，各平台需要实现这些函数。
 */
typedef struct hal_gpio_ops {
  /**
   * @brief 初始化 GPIO 引脚
   * @param ctx GPIO 上下文指针
   * @param config GPIO 配置结构体指针
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*init)(hal_gpio_context_t *ctx,
                           const hal_gpio_config_t *config);

  /**
   * @brief 反初始化 GPIO 引脚
   * @param ctx GPIO 上下文指针
   * @param port GPIO 端口号
   * @param pin GPIO 引脚号
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*deinit)(hal_gpio_context_t *ctx, uint8_t port,
                             uint8_t pin);

  /**
   * @brief 写入 GPIO 引脚状态
   * @param ctx GPIO 上下文指针
   * @param port GPIO 端口号
   * @param pin GPIO 引脚号
   * @param state 要设置的引脚状态
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*write)(hal_gpio_context_t *ctx, uint8_t port, uint8_t pin,
                            hal_gpio_pin_state_t state);

  /**
   * @brief 读取 GPIO 引脚状态
   * @param ctx GPIO 上下文指针
   * @param port GPIO 端口号
   * @param pin GPIO 引脚号
   * @param state 输出参数，用于存储读取到的引脚状态
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*read)(hal_gpio_context_t *ctx, uint8_t port, uint8_t pin,
                           hal_gpio_pin_state_t *state);

  /**
   * @brief 翻转 GPIO 引脚状态
   * @param ctx GPIO 上下文指针
   * @param port GPIO 端口号
   * @param pin GPIO 引脚号
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*toggle)(hal_gpio_context_t *ctx, uint8_t port,
                             uint8_t pin);

  /**
   * @brief 注册 GPIO 中断回调函数
   * @param ctx GPIO 上下文指针
   * @param port GPIO 端口号
   * @param pin GPIO 引脚号
   * @param callback 中断回调函数指针
   * @param user_data 用户自定义数据指针
   * @return 操作结果错误码
   */
  hal_gpio_error_t (*register_callback)(hal_gpio_context_t *ctx, uint8_t port,
                                        uint8_t pin,
                                        hal_gpio_callback_t callback,
                                        void *user_data);
} hal_gpio_ops_t;

/* Exported constants --------------------------------------------------------*/
#define HAL_GPIO_PIN_MAX_VALUE 15 /**< 最大引脚号 */

/* Exported macro ------------------------------------------------------------*/
/**
 * @brief 进入临界区宏
 *
 * 在多线程/中断环境中保护共享资源，防止竞态条件。
 * 当前为空实现，可根据实际平台需求替换为真正的临界区保护代码。
 */
#define HAL_GPIO_ENTER_CRITICAL() \
  do {                            \
  } while (0)

/**
 * @brief 退出临界区宏
 *
 * 与 HAL_GPIO_ENTER_CRITICAL 配对使用。
 */
#define HAL_GPIO_EXIT_CRITICAL() \
  do {                           \
  } while (0)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化 GPIO 引脚
 * @param ctx GPIO 上下文指针
 * @param config GPIO 配置结构体指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t *ctx,
                               const hal_gpio_config_t *config);

/**
 * @brief 反初始化 GPIO 引脚
 * @param ctx GPIO 上下文指针
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_deinit(hal_gpio_context_t *ctx, uint8_t port,
                                 uint8_t pin);

/**
 * @brief 写入 GPIO 引脚状态
 * @param ctx GPIO 上下文指针
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @param state 要设置的引脚状态
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_write(hal_gpio_context_t *ctx, uint8_t port,
                                uint8_t pin, hal_gpio_pin_state_t state);

/**
 * @brief 读取 GPIO 引脚状态
 * @param ctx GPIO 上下文指针
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @param state 输出参数，用于存储读取到的引脚状态
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_read(hal_gpio_context_t *ctx, uint8_t port,
                               uint8_t pin, hal_gpio_pin_state_t *state);

/**
 * @brief 翻转 GPIO 引脚状态
 * @param ctx GPIO 上下文指针
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_toggle(hal_gpio_context_t *ctx, uint8_t port,
                                 uint8_t pin);

/**
 * @brief 注册 GPIO 中断回调函数
 * @param ctx GPIO 上下文指针
 * @param port GPIO 端口号
 * @param pin GPIO 引脚号
 * @param callback 中断回调函数指针
 * @param user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_register_callback(hal_gpio_context_t *ctx,
                                            uint8_t port, uint8_t pin,
                                            hal_gpio_callback_t callback,
                                            void *user_data);

/**
 * @brief 设置 GPIO 操作函数
 * @param ctx GPIO 上下文指针
 * @param ops GPIO 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_gpio_error_t hal_gpio_set_ops(hal_gpio_context_t *ctx,
                                  const hal_gpio_ops_t *ops);

/**
 * @brief STM32 平台 GPIO 上下文初始化函数
 * @param ctx GPIO 上下文指针
 * @return 操作结果错误码
 *
 * 使用此函数初始化 STM32 平台的 GPIO 上下文，会自动设置好平台特定的操作函数。
 */
hal_gpio_error_t stm32_gpio_init_context(hal_gpio_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H */
