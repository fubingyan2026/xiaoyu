/**
 * @brief:  HAL TIM PWM驱动 - 通用层
 * @FilePath: hal_tim_pwm.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 21:15:01
 * @LastEditTime: 2025-09-23 11:13:20
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
#include "hal_tim_pwm.h"

/**
 * @brief  设置 TIM PWM 操作函数
 * @param ctx TIM PWM 上下文指针
 * @param ops TIM PWM 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_tim_pwm_error_t hal_tim_pwm_set_ops(hal_tim_pwm_context_t* ctx,
                                        const hal_tim_pwm_ops_t* ops) {
  if (ctx == NULL || ops == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  HAL_TIM_PWM_ENTER_CRITICAL();
  ctx->ops = ops;
  HAL_TIM_PWM_EXIT_CRITICAL();

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param config TIM PWM 配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_init(hal_tim_pwm_context_t* ctx,
                                     const hal_tim_pwm_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  hal_tim_pwm_error_t ret = ctx->ops->init(ctx, config);

  if (ret == HAL_TIM_PWM_OK) {
    HAL_TIM_PWM_ENTER_CRITICAL();
    ctx->initialized = 1;
    ctx->config = *config;
    ctx->running = 0;
    HAL_TIM_PWM_EXIT_CRITICAL();
  }

  return ret;
}

/**
 * @brief  反初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_deinit(hal_tim_pwm_context_t* ctx,
                                       hal_tim_pwm_instance_t instance,
                                       hal_tim_pwm_channel_t channel) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  hal_tim_pwm_error_t ret = ctx->ops->deinit(ctx, instance, channel);

  if (ret == HAL_TIM_PWM_OK) {
    ctx->initialized = 0;
  }

  return ret;
}

/**
 * @brief  配置GPIO复用功能
 * @param ctx TIM PWM 上下文指针
 * @param gpio_config GPIO配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_gpio_alternate(
    hal_tim_pwm_context_t* ctx, const hal_tim_pwm_gpio_config_t* gpio_config) {
  if (ctx == NULL || gpio_config == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->ops == NULL || ctx->ops->gpio_alternate == NULL) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  return ctx->ops->gpio_alternate(ctx, gpio_config);
}

/**
 * @brief  启动PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_start(hal_tim_pwm_context_t* ctx,
                                      hal_tim_pwm_instance_t instance,
                                      hal_tim_pwm_channel_t channel) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->start == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  hal_tim_pwm_error_t ret = ctx->ops->start(ctx, instance, channel);

  if (ret == HAL_TIM_PWM_OK) {
    HAL_TIM_PWM_ENTER_CRITICAL();
    ctx->running = 1;
    HAL_TIM_PWM_EXIT_CRITICAL();
  }

  return ret;
}

/**
 * @brief  停止PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_stop(hal_tim_pwm_context_t* ctx,
                                     hal_tim_pwm_instance_t instance,
                                     hal_tim_pwm_channel_t channel) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->stop == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  hal_tim_pwm_error_t ret = ctx->ops->stop(ctx, instance, channel);

  if (ret == HAL_TIM_PWM_OK) {
    HAL_TIM_PWM_ENTER_CRITICAL();
    ctx->running = 0;
    HAL_TIM_PWM_EXIT_CRITICAL();
  }

  return ret;
}

/**
 * @brief  设置PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_duty_cycle(hal_tim_pwm_context_t* ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel,
                                               uint32_t duty_cycle) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->set_duty_cycle == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return ctx->ops->set_duty_cycle(ctx, instance, channel, duty_cycle);
}

/**
 * @brief  设置PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency PWM频率(Hz)
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_frequency(hal_tim_pwm_context_t* ctx,
                                              hal_tim_pwm_instance_t instance,
                                              hal_tim_pwm_channel_t channel,
                                              uint32_t frequency) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->set_frequency == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return ctx->ops->set_frequency(ctx, instance, channel, frequency);
}

/**
 * @brief  获取PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 输出参数，返回占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_duty_cycle(hal_tim_pwm_context_t* ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel,
                                               uint32_t* duty_cycle) {
  if (ctx == NULL || duty_cycle == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->get_duty_cycle == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return ctx->ops->get_duty_cycle(ctx, instance, channel, duty_cycle);
}

/**
 * @brief  获取PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency 输出参数，返回PWM频率(Hz)
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_frequency(hal_tim_pwm_context_t* ctx,
                                              hal_tim_pwm_instance_t instance,
                                              hal_tim_pwm_channel_t channel,
                                              uint32_t* frequency) {
  if (ctx == NULL || frequency == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->get_frequency == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return ctx->ops->get_frequency(ctx, instance, channel, frequency);
}

/**
 * @brief 设置PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity PWM极性
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_set_polarity(hal_tim_pwm_context_t* ctx,
                                             hal_tim_pwm_instance_t instance,
                                             hal_tim_pwm_channel_t channel,
                                             hal_tim_pwm_polarity_t polarity) {
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->set_polarity == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  hal_tim_pwm_error_t ret =
      ctx->ops->set_polarity(ctx, instance, channel, polarity);

  if (ret == HAL_TIM_PWM_OK) {
    HAL_TIM_PWM_ENTER_CRITICAL();
    ctx->config.polarity = polarity;
    HAL_TIM_PWM_EXIT_CRITICAL();
  }

  return ret;
}

/**
 * @brief 获取PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity 输出参数，返回PWM极性
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_polarity(hal_tim_pwm_context_t* ctx,
                                             hal_tim_pwm_instance_t instance,
                                             hal_tim_pwm_channel_t channel,
                                             hal_tim_pwm_polarity_t* polarity) {
  if (ctx == NULL || polarity == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  if (ctx->ops == NULL || ctx->ops->get_polarity == NULL) {
    return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return ctx->ops->get_polarity(ctx, instance, channel, polarity);
}

/**
 * @brief  检查PWM是否正在运行
 * @param ctx TIM PWM 上下文指针
 * @param running 输出参数，返回PWM是否正在运行
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_is_running(hal_tim_pwm_context_t* ctx,
                                           bool* running) {
  if (ctx == NULL || running == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  *running = (ctx->running != 0);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  检查PWM是否初始化
 * @param ctx TIM PWM 上下文指针
 * @param initialized 输出参数，返回PWM是否初始化
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_is_initialized(hal_tim_pwm_context_t* ctx,
                                               bool* initialized) {
  if (ctx == NULL || initialized == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  *initialized = (ctx->initialized != 0);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  获取PWM配置
 * @param ctx TIM PWM 上下文指针
 * @param config 输出参数，返回PWM配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_get_config(hal_tim_pwm_context_t* ctx,
                                           hal_tim_pwm_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  HAL_TIM_PWM_ENTER_CRITICAL();
  *config = ctx->config;
  HAL_TIM_PWM_EXIT_CRITICAL();

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  更新PWM配置
 * @param ctx TIM PWM 上下文指针
 * @param config PWM配置结构体指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t hal_tim_pwm_update_config(
    hal_tim_pwm_context_t* ctx, const hal_tim_pwm_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_TIM_PWM_ERROR_UNINITIALIZED;
  }

  hal_tim_pwm_error_t ret = hal_tim_pwm_init(ctx, config);

  return ret;
}
