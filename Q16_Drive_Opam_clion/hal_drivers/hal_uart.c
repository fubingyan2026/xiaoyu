//
// Created by fubingyan on 25-9-27.
//

/**
 * @file    hal_uart.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-27
 * @brief   硬件抽象层 - UART 实现
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
#include "hal_uart.h"

/* Private function prototypes -----------------------------------------------*/
static inline bool is_valid_instance(hal_uart_instance_t instance);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 检查UART实例是否有效
 * @param instance 要检查的UART实例
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_instance(hal_uart_instance_t instance) {
  return instance < HAL_UART_INSTANCE_LEN;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置 UART 操作函数
 * @param  ctx UART 上下文指针
 * @param  ops UART 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_uart_error_t hal_uart_set_ops(hal_uart_context_t* ctx,
                                  const hal_uart_ops_t* ops) {
  // 检查参数有效性
  if (ctx == NULL || ops == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查必要的函数指针是否为 NULL
  if (ops->init == NULL || ops->deinit == NULL || ops->send == NULL ||
      ops->receive == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 进入临界区，保护共享资源
  HAL_UART_ENTER_CRITICAL();
  ctx->ops = ops;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief  初始化 UART
 * @param  ctx UART 上下文指针
 * @param  config UART 配置结构体指针
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_init(hal_uart_context_t* ctx,
                               const hal_uart_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(config->instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的初始化函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->init(ctx, config);
  if (result == HAL_UART_OK) {
    ctx->initialized = 1;
    ctx->config = *config;
  }
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  反初始化 UART
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_deinit(hal_uart_context_t* ctx,
                                 hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的反初始化函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->deinit(ctx, instance);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  发送数据（阻塞）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 要发送的数据指针
 * @param  size 要发送的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send(hal_uart_context_t* ctx,
                               hal_uart_instance_t instance,
                               const uint8_t* data, uint16_t size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->send == NULL) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的发送函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->send(ctx, instance, data, size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  接收数据（阻塞）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 接收数据缓冲区指针
 * @param  size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive(hal_uart_context_t* ctx,
                                  hal_uart_instance_t instance, uint8_t* data,
                                  uint16_t size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive == NULL) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->receive(ctx, instance, data, size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  发送数据（异步）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 要发送的数据指针
 * @param  size 要发送的数据大小
 * @param  sent_size 输出参数，返回已发送的字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send_async(hal_uart_context_t* ctx,
                                     hal_uart_instance_t instance,
                                     const uint8_t* data, uint16_t size,
                                     uint16_t* sent_size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0 || sent_size == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->send_async == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的异步发送函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result =
      ctx->ops->send_async(ctx, instance, data, size, sent_size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  接收数据（异步）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 接收数据缓冲区指针
 * @param  size 要接收的数据大小
 * @param  received_size 输出参数，返回已接收的字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_async(hal_uart_context_t* ctx,
                                        hal_uart_instance_t instance,
                                        uint8_t* data, uint16_t size,
                                        uint16_t* received_size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0 || received_size == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive_async == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的异步接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result =
      ctx->ops->receive_async(ctx, instance, data, size, received_size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  发送数据（DMA）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 要发送的数据指针
 * @param  size 要发送的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send_dma(hal_uart_context_t* ctx,
                                   hal_uart_instance_t instance,
                                   const uint8_t* data, uint32_t size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->send_dma == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的DMA发送函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->send_dma(ctx, instance, data, size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  接收数据（DMA）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 接收数据缓冲区指针
 * @param  size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_dma(hal_uart_context_t* ctx,
                                      hal_uart_instance_t instance,
                                      uint8_t* data, uint32_t size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive_dma == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的DMA接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->receive_dma(ctx, instance, data, size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  接收数据（DMA，空闲中断检测）
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  data 接收数据缓冲区指针
 * @param  size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_dma_to_idle(hal_uart_context_t* ctx,
                                              hal_uart_instance_t instance,
                                              uint8_t* data, uint32_t size) {
  // 检查参数有效性
  if (ctx == NULL || data == NULL || size == 0) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive_dma_to_idle == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的DMA空闲接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result =
      ctx->ops->receive_dma_to_idle(ctx, instance, data, size);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  中止发送
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_send(hal_uart_context_t* ctx,
                                     hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->abort_send == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的中止发送函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->abort_send(ctx, instance);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  中止接收
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_receive(hal_uart_context_t* ctx,
                                        hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->abort_receive == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的中止接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->abort_receive(ctx, instance);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  中止DMA发送
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_send_dma(hal_uart_context_t* ctx,
                                         hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->abort_send_dma == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的中止DMA发送函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->abort_send_dma(ctx, instance);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  中止DMA接收
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_receive_dma(hal_uart_context_t* ctx,
                                            hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->abort_receive_dma == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的中止DMA接收函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->abort_receive_dma(ctx, instance);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取发送字节数
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  count 输出参数，返回发送字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_tx_count(hal_uart_context_t* ctx,
                                       hal_uart_instance_t instance,
                                       uint32_t* count) {
  // 检查参数有效性
  if (ctx == NULL || count == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_tx_count == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取发送字节数函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->get_tx_count(ctx, instance, count);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取接收字节数
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  count 输出参数，返回接收字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_rx_count(hal_uart_context_t* ctx,
                                       hal_uart_instance_t instance,
                                       uint32_t* count) {
  // 检查参数有效性
  if (ctx == NULL || count == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_rx_count == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取接收字节数函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->get_rx_count(ctx, instance, count);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取DMA状态
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  status 输出参数，返回DMA状态
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_dma_status(hal_uart_context_t* ctx,
                                         hal_uart_instance_t instance,
                                         hal_uart_dma_status_t* status) {
  // 检查参数有效性
  if (ctx == NULL || status == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_dma_status == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取DMA状态函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->get_dma_status(ctx, instance, status);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  设置波特率
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  baudrate 波特率
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_baudrate(hal_uart_context_t* ctx,
                                       hal_uart_instance_t instance,
                                       hal_uart_baudrate_t baudrate) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->set_baudrate == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的设置波特率函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->set_baudrate(ctx, instance, baudrate);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  设置接收超时
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @param  timeout 超时时间(ms)
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_rx_timeout(hal_uart_context_t* ctx,
                                         hal_uart_instance_t instance,
                                         uint32_t timeout) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->set_rx_timeout == NULL) {
    return HAL_UART_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的设置接收超时函数
  HAL_UART_ENTER_CRITICAL();
  hal_uart_error_t result = ctx->ops->set_rx_timeout(ctx, instance, timeout);
  HAL_UART_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  UART中断处理
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 */
void hal_uart_irq_handler(hal_uart_context_t* ctx,
                          hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->irq_handler == NULL) {
    return;
  }

  // 调用平台特定的UART中断处理函数
  ctx->ops->irq_handler(ctx, instance);
}

/**
 * @brief  DMA中断处理
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 */
void hal_uart_dma_irq_handler(hal_uart_context_t* ctx,
                              hal_uart_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return;
  }

  // 检查UART实例是否有效
  if (!is_valid_instance(instance)) {
    return;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->dma_irq_handler == NULL) {
    return;
  }

  // 调用平台特定的DMA中断处理函数
  ctx->ops->dma_irq_handler(ctx, instance);
}

/**
 * @brief  检查UART是否初始化
 * @param ctx UART 上下文指针
 * @param initialized 输出参数，返回UART是否初始化
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_is_initialized(hal_uart_context_t* ctx, bool* initialized) {
  if (ctx == NULL || initialized == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  *initialized = (ctx->initialized != 0);

  return HAL_UART_OK;
}

/**
 * @brief  获取UART配置
 * @param ctx UART 上下文指针
 * @param config 输出参数，返回UART配置
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_config(hal_uart_context_t* ctx, hal_uart_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  *config = ctx->config;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief  更新UART配置
 * @param ctx UART 上下文指针
 * @param config UART配置指针
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_update_config(hal_uart_context_t* ctx, const hal_uart_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  return hal_uart_init(ctx, config);
}

/**
 * @brief  设置UART字长
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param wordlength UART字长
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_wordlength(hal_uart_context_t* ctx,
                                          hal_uart_instance_t instance,
                                          hal_uart_wordlength_t wordlength) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  ctx->config.wordlength = wordlength;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief  设置UART停止位
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param stopbits UART停止位
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_stopbits(hal_uart_context_t* ctx,
                                        hal_uart_instance_t instance,
                                        hal_uart_stopbits_t stopbits) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  ctx->config.stopbits = stopbits;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief  设置UART校验位
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param parity UART校验位
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_parity(hal_uart_context_t* ctx,
                                      hal_uart_instance_t instance,
                                      hal_uart_parity_t parity) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  ctx->config.parity = parity;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief  设置UART硬件控制
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param hwcontrol UART硬件控制
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_hwcontrol(hal_uart_context_t* ctx,
                                         hal_uart_instance_t instance,
                                         hal_uart_hwcontrol_t hwcontrol) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  ctx->config.hwcontrol = hwcontrol;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}
/**
 * @brief  设置UART模式
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param mode UART模式
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_mode(hal_uart_context_t* ctx,
                                    hal_uart_instance_t instance,
                                    hal_uart_mode_t mode) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  HAL_UART_ENTER_CRITICAL();
  ctx->config.mode = mode;
  HAL_UART_EXIT_CRITICAL();

  return HAL_UART_OK;
}

/**
 * @brief 刷新发送缓冲区
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_flush_tx(hal_uart_context_t* ctx, hal_uart_instance_t instance) {
  (void)instance;
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  return HAL_UART_OK;
}

/**
 * @brief  清空接收缓冲区
 * @param  ctx UART 上下文指针
 * @param  instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_flush_rx(hal_uart_context_t* ctx, hal_uart_instance_t instance) {
  (void)instance;
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_UART_ERROR_UNINITIALIZED;
  }

  return HAL_UART_OK;
}
