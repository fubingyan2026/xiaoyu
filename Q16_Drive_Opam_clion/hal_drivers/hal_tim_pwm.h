//
// Created by fubingyan on 25-9-20.
//

#ifndef __HAL_TIM_PWM_H
#define __HAL_TIM_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hal_gpio.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief TIM PWM 操作错误码枚举
 */
typedef enum {
  HAL_TIM_PWM_OK = 0,              /**< 操作成功 */
  HAL_TIM_PWM_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_TIM_PWM_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_TIM_PWM_ERROR_HARDWARE,      /**< 硬件错误 */
  HAL_TIM_PWM_ERROR_BUSY,          /**< 设备忙 */
  HAL_TIM_PWM_ERROR_UNSUPPORTED,   /**< 不支持的操作 */
  HAL_TIM_PWM_ERROR_TIMEOUT,       /**< 超时 */
} hal_tim_pwm_error_t;

/**
 * @brief TIM PWM通道定义
 */
typedef enum __attribute__((packed)) {
  HAL_TIM_PWM_CHANNEL_1 = 0, /**< PWM通道1 */
  HAL_TIM_PWM_CHANNEL_2,     /**< PWM通道2 */
  HAL_TIM_PWM_CHANNEL_3,     /**< PWM通道3 */
  HAL_TIM_PWM_CHANNEL_4,     /**< PWM通道4 */
  HAL_TIM_PWM_CHANNEL_LEN    /**< 通道数量 */
} hal_tim_pwm_channel_t;

/**
 * @brief TIM PWM极性定义
 */
typedef enum __attribute__((packed)) {
  HAL_TIM_PWM_POLARITY_HIGH = 0, /**< 高电平有效 */
  HAL_TIM_PWM_POLARITY_LOW       /**< 低电平有效 */
} hal_tim_pwm_polarity_t;

/**
 * @brief TIM PWM实例定义
 */
typedef enum __attribute__((packed)) {
  HAL_TIM_PWM_INSTANCE_1 = 1, /**< TIM1 */
  HAL_TIM_PWM_INSTANCE_2,     /**< TIM2 */
  HAL_TIM_PWM_INSTANCE_3,     /**< TIM3 */
  HAL_TIM_PWM_INSTANCE_4,     /**< TIM4 */
  HAL_TIM_PWM_INSTANCE_6,     /**< TIM6 */
  HAL_TIM_PWM_INSTANCE_7,     /**< TIM7 */
  HAL_TIM_PWM_INSTANCE_8,     /**< TIM8 */
  HAL_TIM_PWM_INSTANCE_15,    /**< TIM15 */
  HAL_TIM_PWM_INSTANCE_16,    /**< TIM16 */
  HAL_TIM_PWM_INSTANCE_17,    /**< TIM17 */
} hal_tim_pwm_instance_t;

/**
 * @brief GPIO引脚复用配置
 */
typedef struct {
  gpio_port_t port;  /**< GPIO端口 */
  uint8_t pin;       /**< GPIO引脚 */
  uint8_t alternate; /**< 复用功能 */
} hal_tim_pwm_gpio_config_t;

/**
 * @brief TIM PWM配置结构体
 */
typedef struct {
  hal_tim_pwm_instance_t timer_instance; /**< 定时器实例 */
  hal_tim_pwm_channel_t channel;         /**< PWM通道 */
  uint32_t timer_frequency;              /**< 定时器时钟频率(Hz) */
  uint32_t pwm_frequency;                /**< PWM频率(Hz) */
  uint32_t duty_cycle;                   /**< 占空比(0-10000对应0%-100% */
  hal_tim_pwm_polarity_t polarity;       /**< PWM极性 */
  hal_tim_pwm_gpio_config_t gpio;        /**< GPIO引脚配置 */
} hal_tim_pwm_config_t;

/**
 * @brief TIM PWM 上下文结构体前向声明
 */
typedef struct hal_tim_pwm_context hal_tim_pwm_context_t;

/**
 * @brief TIM PWM 上下文结构体
 *
 * 用于保存 TIM PWM 实例的状态信息，支持多实例操作。
 */
struct hal_tim_pwm_context {
  const struct hal_tim_pwm_ops *ops; /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized; /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_tim_pwm_config_t config; /**< 当前 PWM 配置 */
  volatile uint8_t running; /**< 运行状态标志（0=停止，1=运行） */
};

/**
 * @brief TIM PWM 操作函数结构体
 *
 * 定义了平台特定的 TIM PWM 操作接口，各平台需要实现这些函数。
 */
typedef struct hal_tim_pwm_ops {
  /**
   * @brief 初始化 TIM PWM
   * @param ctx TIM PWM 上下文指针
   * @param config TIM PWM 配置结构体指针
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*init)(hal_tim_pwm_context_t *ctx,
                              const hal_tim_pwm_config_t *config);

  /**
   * @brief 反初始化 TIM PWM
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*deinit)(hal_tim_pwm_context_t *ctx,
                                hal_tim_pwm_instance_t instance,
                                hal_tim_pwm_channel_t channel);

  /**
   * @brief 配置GPIO复用功能
   * @param ctx TIM PWM 上下文指针
   * @param gpio_config GPIO配置结构体指针
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*gpio_alternate)(
      hal_tim_pwm_context_t *ctx, const hal_tim_pwm_gpio_config_t *gpio_config);

  /**
   * @brief 启动PWM输出
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*start)(hal_tim_pwm_context_t *ctx,
                               hal_tim_pwm_instance_t instance,
                               hal_tim_pwm_channel_t channel);

  /**
   * @brief 停止PWM输出
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*stop)(hal_tim_pwm_context_t *ctx,
                              hal_tim_pwm_instance_t instance,
                              hal_tim_pwm_channel_t channel);

  /**
   * @brief 设置PWM占空比
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param duty_cycle 占空比(0-10000对应0%-100%
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*set_duty_cycle)(hal_tim_pwm_context_t *ctx,
                                        hal_tim_pwm_instance_t instance,
                                        hal_tim_pwm_channel_t channel,
                                        uint32_t duty_cycle);

  /**
   * @brief 设置PWM频率
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param frequency PWM频率(Hz)
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*set_frequency)(hal_tim_pwm_context_t *ctx,
                                       hal_tim_pwm_instance_t instance,
                                       hal_tim_pwm_channel_t channel,
                                       uint32_t frequency);

  /**
   * @brief 获取PWM占空比
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param duty_cycle 输出参数，返回占空比(0-10000对应0%-100%
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*get_duty_cycle)(hal_tim_pwm_context_t *ctx,
                                        hal_tim_pwm_instance_t instance,
                                        hal_tim_pwm_channel_t channel,
                                        uint32_t *duty_cycle);

  /**
   * @brief 获取PWM频率
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param frequency 输出参数，返回PWM频率(Hz)
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*get_frequency)(hal_tim_pwm_context_t *ctx,
                                       hal_tim_pwm_instance_t instance,
                                       hal_tim_pwm_channel_t channel,
                                       uint32_t *frequency);

  /**
   * @brief 设置PWM极性
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param polarity PWM极性
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*set_polarity)(hal_tim_pwm_context_t *ctx,
                                      hal_tim_pwm_instance_t instance,
                                      hal_tim_pwm_channel_t channel,
                                      hal_tim_pwm_polarity_t polarity);

  /**
   * @brief 获取PWM极性
   * @param ctx TIM PWM 上下文指针
   * @param instance 定时器实例
   * @param channel PWM通道
   * @param polarity 输出参数，返回PWM极性
   * @return 操作结果错误码
   */
  hal_tim_pwm_error_t (*get_polarity)(hal_tim_pwm_context_t *ctx,
                                      hal_tim_pwm_instance_t instance,
                                      hal_tim_pwm_channel_t channel,
                                      hal_tim_pwm_polarity_t *polarity);
} hal_tim_pwm_ops_t;

/* Exported macro ------------------------------------------------------------*/
/**
 * @brief 进入临界区宏
 *
 * 在多线程/中断环境中保护共享资源，防止竞态条件。
 * 当前为空实现，可根据实际平台需求替换为真正的临界区保护代码。
 */
#define HAL_TIM_PWM_ENTER_CRITICAL() \
  do {                               \
  } while (0)

/**
 * @brief 退出临界区宏
 *
 * 与 HAL_TIM_PWM_ENTER_CRITICAL 配对使用。
 */
#define HAL_TIM_PWM_EXIT_CRITICAL() \
  do {                              \
  } while (0)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param config TIM PWM 配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_init(hal_tim_pwm_context_t *ctx,
                                     const hal_tim_pwm_config_t *config);

/**
 * @brief 反初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_deinit(hal_tim_pwm_context_t *ctx,
                                       hal_tim_pwm_instance_t instance,
                                       hal_tim_pwm_channel_t channel);

/**
 * @brief 配置GPIO复用功能
 * @param ctx TIM PWM 上下文指针
 * @param gpio_config GPIO配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_gpio_alternate(
    hal_tim_pwm_context_t *ctx, const hal_tim_pwm_gpio_config_t *gpio_config);

/**
 * @brief 启动PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_start(hal_tim_pwm_context_t *ctx,
                                      hal_tim_pwm_instance_t instance,
                                      hal_tim_pwm_channel_t channel);

/**
 * @brief 停止PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_stop(hal_tim_pwm_context_t *ctx,
                                     hal_tim_pwm_instance_t instance,
                                     hal_tim_pwm_channel_t channel);

/**
 * @brief 设置PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_duty_cycle(hal_tim_pwm_context_t *ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel,
                                               uint32_t duty_cycle);

/**
 * @brief 设置PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency PWM频率(Hz)
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_frequency(hal_tim_pwm_context_t *ctx,
                                              hal_tim_pwm_instance_t instance,
                                              hal_tim_pwm_channel_t channel,
                                              uint32_t frequency);

/**
 * @brief 获取PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 输出参数，返回占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_duty_cycle(hal_tim_pwm_context_t *ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel,
                                               uint32_t *duty_cycle);

/**
 * @brief 获取PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency 输出参数，返回PWM频率(Hz)
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_frequency(hal_tim_pwm_context_t *ctx,
                                              hal_tim_pwm_instance_t instance,
                                              hal_tim_pwm_channel_t channel,
                                              uint32_t *frequency);

/**
 * @brief 设置 TIM PWM 操作函数
 * @param ctx TIM PWM 上下文指针
 * @param ops TIM PWM 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_tim_pwm_error_t hal_tim_pwm_set_ops(hal_tim_pwm_context_t *ctx,
                                        const hal_tim_pwm_ops_t *ops);

/**
 * @brief STM32 平台 TIM PWM 上下文初始化函数
 * @param ctx TIM PWM 上下文指针
 * @return 操作结果错误码
 *
 * 使用此函数初始化 STM32 平台的 TIM PWM
 * 上下文，会自动设置好平台特定的操作函数。
 */
hal_tim_pwm_error_t stm32_tim_pwm_init_context(hal_tim_pwm_context_t *ctx);

/**
 * @brief 设置PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity PWM极性
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_polarity(hal_tim_pwm_context_t *ctx,
                                             hal_tim_pwm_instance_t instance,
                                             hal_tim_pwm_channel_t channel,
                                             hal_tim_pwm_polarity_t polarity);

/**
 * @brief 获取PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity 输出参数，返回PWM极性
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_polarity(hal_tim_pwm_context_t *ctx,
                                             hal_tim_pwm_instance_t instance,
                                             hal_tim_pwm_channel_t channel,
                                             hal_tim_pwm_polarity_t *polarity);

/**
 * @brief 检查PWM是否正在运行
 * @param ctx TIM PWM 上下文指针
 * @param running 输出参数，返回运行状态（true=正在运行，false=已停止）
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_is_running(hal_tim_pwm_context_t *ctx,
                                           bool *running);

/**
 * @brief 检查PWM是否已初始化
 * @param ctx TIM PWM 上下文指针
 * @param initialized 输出参数，返回初始化状态（true=已初始化，false=未初始化）
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_is_initialized(hal_tim_pwm_context_t *ctx,
                                               bool *initialized);

/**
 * @brief 获取当前PWM配置
 * @param ctx TIM PWM 上下文指针
 * @param config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_config(hal_tim_pwm_context_t *ctx,
                                           hal_tim_pwm_config_t *config);

/**
 * @brief 更新PWM配置
 * @param ctx TIM PWM 上下文指针
 * @param config 新的配置结构体指针
 * @return 操作结果错误码
 *
 * @note 此函数会更新所有配置参数，包括频率、占空比和极性
 */
hal_tim_pwm_error_t hal_tim_pwm_update_config(
    hal_tim_pwm_context_t *ctx, const hal_tim_pwm_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_TIM_PWM_H */
