//
// Created by fubingyan on 25-3-30.
//
/**
 * @file    hal_adc.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-03-30
 * @brief   硬件抽象层 - ADC 实现
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
#include "hal_adc.h"

/* Private function prototypes -----------------------------------------------*/
static inline bool is_valid_instance(hal_adc_instance_t instance);
static inline bool is_valid_channel(hal_adc_channel_t channel);
static inline bool is_valid_resolution(hal_adc_resolution_t resolution);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 检查ADC实例是否有效
 * @param instance 要检查的ADC实例
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_instance(hal_adc_instance_t instance) {
  return instance < HAL_ADC_INSTANCE_LEN;
}

/**
 * @brief 检查ADC通道是否有效
 * @param channel 要检查的ADC通道
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_channel(hal_adc_channel_t channel) {
  return channel < HAL_ADC_CHANNEL_LEN;
}

/**
 * @brief 检查ADC分辨率是否有效
 * @param resolution 要检查的ADC分辨率
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_resolution(hal_adc_resolution_t resolution) {
  return resolution <= HAL_ADC_RESOLUTION_6B;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置 ADC 操作函数
 * @param  ctx ADC 上下文指针
 * @param  ops ADC 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_adc_error_t hal_adc_set_ops(hal_adc_context_t* ctx,
                                const hal_adc_ops_t* ops) {
  // 检查参数有效性
  if (ctx == NULL || ops == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查必要的函数指针是否为 NULL
  if (ops->init == NULL || ops->deinit == NULL ||
      ops->start_conversion == NULL || ops->stop_conversion == NULL ||
      ops->get_value == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 进入临界区，保护共享资源
  HAL_ADC_ENTER_CRITICAL();
  ctx->ops = ops;
  HAL_ADC_EXIT_CRITICAL();

  return HAL_ADC_OK;
}

/**
 * @brief  初始化 ADC
 * @param  ctx ADC 上下文指针
 * @param  config ADC 配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_init(hal_adc_context_t* ctx,
                             const hal_adc_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(config->instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查分辨率是否有效
  if (!is_valid_resolution(config->resolution)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 如果是第一次初始化，标记为已初始化
  if (!ctx->initialized) {
    ctx->initialized = 1;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的初始化函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->init(ctx, config);
  if (result == HAL_ADC_OK) {
    ctx->config = *config;
  }
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  反初始化 ADC
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_deinit(hal_adc_context_t* ctx,
                               hal_adc_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的反初始化函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->deinit(ctx, instance);
  if (result == HAL_ADC_OK) {
    ctx->initialized = 0;
    ctx->busy = 0;
  }
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  配置ADC通道
 * @param  ctx ADC 上下文指针
 * @param  channel_config 通道配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_config_channel(
    hal_adc_context_t* ctx, const hal_adc_channel_config_t* channel_config) {
  // 检查参数有效性
  if (ctx == NULL || channel_config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC通道是否有效
  if (!is_valid_channel(channel_config->channel)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->config_channel == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的通道配置函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->config_channel(ctx, channel_config);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  启动ADC转换（阻塞）
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  value 输出参数，存储转换结果
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_conversion(hal_adc_context_t* ctx,
                                         hal_adc_instance_t instance,
                                         uint32_t* value) {
  // 检查参数有效性
  if (ctx == NULL || value == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查是否已初始化
  if (!ctx->initialized) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->start_conversion == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 检查是否忙
  if (ctx->busy) {
    return HAL_ADC_ERROR_BUSY;
  }

  // 进入临界区，调用平台特定的转换函数
  HAL_ADC_ENTER_CRITICAL();
  ctx->busy = 1;
  hal_adc_error_t result = ctx->ops->start_conversion(ctx, instance, value);
  ctx->busy = 0;
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  启动ADC转换（异步）
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_conversion_async(hal_adc_context_t* ctx,
                                               hal_adc_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查是否已初始化
  if (!ctx->initialized) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->start_conversion_async == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 检查是否忙
  if (ctx->busy) {
    return HAL_ADC_ERROR_BUSY;
  }

  // 进入临界区，调用平台特定的异步转换函数
  HAL_ADC_ENTER_CRITICAL();
  ctx->busy = 1;
  hal_adc_error_t result = ctx->ops->start_conversion_async(ctx, instance);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  停止ADC转换
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_stop_conversion(hal_adc_context_t* ctx,
                                        hal_adc_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->stop_conversion == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的停止转换函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->stop_conversion(ctx, instance);
  ctx->busy = 0;
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  启动ADC DMA转换
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  dma_config DMA配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_dma(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance,
                                  const hal_adc_dma_config_t* dma_config) {
  // 检查参数有效性
  if (ctx == NULL || dma_config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查缓冲区指针是否有效
  if (dma_config->buffer == NULL || dma_config->buffer_length == 0) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查是否已初始化
  if (!ctx->initialized) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->start_dma == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 检查是否忙
  if (ctx->busy) {
    return HAL_ADC_ERROR_BUSY;
  }

  // 进入临界区，调用平台特定的DMA启动函数
  HAL_ADC_ENTER_CRITICAL();
  ctx->busy = 1;
  hal_adc_error_t result = ctx->ops->start_dma(ctx, instance, dma_config);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  停止ADC DMA转换
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_stop_dma(hal_adc_context_t* ctx,
                                 hal_adc_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->stop_dma == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的DMA停止函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->stop_dma(ctx, instance);
  ctx->busy = 0;
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取转换结果
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  value 输出参数，存储转换结果
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_value(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance,
                                  uint32_t* value) {
  // 检查参数有效性
  if (ctx == NULL || value == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_value == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的获取值函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->get_value(ctx, instance, value);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  校准ADC
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  calibration_config 校准配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_calibrate(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_calibration_config_t* calibration_config) {
  // 检查参数有效性
  if (ctx == NULL || calibration_config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->calibrate == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的校准函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result =
      ctx->ops->calibrate(ctx, instance, calibration_config);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  设置分辨率
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  resolution 分辨率
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_set_resolution(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       hal_adc_resolution_t resolution) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查分辨率是否有效
  if (!is_valid_resolution(resolution)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->set_resolution == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的设置分辨率函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->set_resolution(ctx, instance, resolution);
  if (result == HAL_ADC_OK) {
    ctx->config.resolution = resolution;
  }
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取分辨率
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  resolution 输出参数，返回分辨率
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_resolution(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       hal_adc_resolution_t* resolution) {
  // 检查参数有效性
  if (ctx == NULL || resolution == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_resolution == NULL) {
    // 如果平台层未实现，直接返回上下文中保存的配置
    *resolution = ctx->config.resolution;
    return HAL_ADC_OK;
  }

  // 进入临界区，调用平台特定的获取分辨率函数
  HAL_ADC_ENTER_CRITICAL();
  hal_adc_error_t result = ctx->ops->get_resolution(ctx, instance, resolution);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  注册转换完成回调函数
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  callback 回调函数指针
 * @param  user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_register_callback(hal_adc_context_t* ctx,
                                          hal_adc_instance_t instance,
                                          hal_adc_conv_callback_t callback,
                                          void* user_data) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->register_callback == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 进入临界区，保存回调函数和用户数据，然后调用平台特定的注册函数
  HAL_ADC_ENTER_CRITICAL();
  ctx->conv_callback = callback;
  ctx->user_data = user_data;
  hal_adc_error_t result =
      ctx->ops->register_callback(ctx, instance, callback, user_data);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  注册DMA完成回调函数
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 * @param  callback 回调函数指针
 * @param  user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_register_dma_callback(hal_adc_context_t* ctx,
                                              hal_adc_instance_t instance,
                                              hal_adc_dma_callback_t callback,
                                              void* user_data) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->register_dma_callback == NULL) {
    return HAL_ADC_ERROR_UNSUPPORTED;
  }

  // 进入临界区，保存回调函数和用户数据，然后调用平台特定的注册函数
  HAL_ADC_ENTER_CRITICAL();
  ctx->dma_callback = callback;
  ctx->user_data = user_data;
  hal_adc_error_t result =
      ctx->ops->register_dma_callback(ctx, instance, callback, user_data);
  HAL_ADC_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  ADC中断处理
 * @param  ctx ADC 上下文指针
 * @param  instance ADC 实例
 */
void hal_adc_irq_handler(hal_adc_context_t* ctx, hal_adc_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(instance)) {
    return;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->irq_handler == NULL) {
    return;
  }

  // 调用平台特定的中断处理函数
  ctx->ops->irq_handler(ctx, instance);
}

/**
 * @brief  检查ADC是否已初始化
 * @param  ctx ADC 上下文指针
 * @param  initialized 输出参数，返回初始化状态
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_is_initialized(hal_adc_context_t* ctx,
                                       bool* initialized) {
  // 检查参数有效性
  if (ctx == NULL || initialized == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  *initialized = (ctx->initialized != 0);

  return HAL_ADC_OK;
}

/**
 * @brief  获取当前ADC配置
 * @param  ctx ADC 上下文指针
 * @param  config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_config(hal_adc_context_t* ctx,
                                   hal_adc_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 进入临界区，复制配置
  HAL_ADC_ENTER_CRITICAL();
  *config = ctx->config;
  HAL_ADC_EXIT_CRITICAL();

  return HAL_ADC_OK;
}

/**
 * @brief  更新ADC配置
 * @param  ctx ADC 上下文指针
 * @param  config 新的配置结构体指针
 * @return 操作结果错误码
 *
 * @note 此函数会更新所有配置参数
 */
hal_adc_error_t hal_adc_update_config(hal_adc_context_t* ctx,
                                      const hal_adc_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查ADC实例是否有效
  if (!is_valid_instance(config->instance)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 检查分辨率是否有效
  if (!is_valid_resolution(config->resolution)) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  // 进入临界区，更新配置
  HAL_ADC_ENTER_CRITICAL();
  ctx->config = *config;
  HAL_ADC_EXIT_CRITICAL();

  return HAL_ADC_OK;
}

/**
 * @brief  检查ADC是否忙
 * @param  ctx ADC 上下文指针
 * @param  busy 输出参数，返回忙状态
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_is_busy(hal_adc_context_t* ctx, bool* busy) {
  // 检查参数有效性
  if (ctx == NULL || busy == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  *busy = (ctx->busy != 0);

  return HAL_ADC_OK;
}

/**
 * @brief  将ADC原始值转换为电压值
 * @param  raw_value ADC原始值
 * @param  resolution ADC分辨率
 * @param  vref_mv 参考电压（mV）
 * @return 电压值（mV）
 */
uint32_t hal_adc_raw_to_voltage(uint32_t raw_value,
                                hal_adc_resolution_t resolution,
                                uint32_t vref_mv) {
  uint32_t max_value = hal_adc_get_max_value(resolution);

  if (max_value == 0) {
    return 0;
  }

  return (uint32_t)(((uint64_t)raw_value * vref_mv) / max_value);
}

/**
 * @brief  获取ADC最大值
 * @param  resolution ADC分辨率
 * @return 最大值
 */
uint32_t hal_adc_get_max_value(hal_adc_resolution_t resolution) {
  switch (resolution) {
    case HAL_ADC_RESOLUTION_12B:
      return HAL_ADC_12BIT_MAX_VALUE;
    case HAL_ADC_RESOLUTION_10B:
      return HAL_ADC_10BIT_MAX_VALUE;
    case HAL_ADC_RESOLUTION_8B:
      return HAL_ADC_8BIT_MAX_VALUE;
    case HAL_ADC_RESOLUTION_6B:
      return HAL_ADC_6BIT_MAX_VALUE;
    default:
      return HAL_ADC_12BIT_MAX_VALUE;
  }
}
