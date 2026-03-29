//
// Created by fubingyan on 25-9-21.
//
// hal_fdcan.c
#include "hal_fdcan.h"

/* Private function prototypes -----------------------------------------------*/
static inline bool is_valid_instance(hal_fdcan_instance_t instance);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 检查FDCAN实例是否有效
 * @param instance 要检查的FDCAN实例
 * @return true 表示有效，false 表示无效
 */
static inline bool is_valid_instance(hal_fdcan_instance_t instance) {
  return instance < HAL_FDCAN_INSTANCE_LEN;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  设置 FDCAN 操作函数
 * @param  ctx FDCAN 上下文指针
 * @param  ops FDCAN 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_fdcan_error_t hal_fdcan_set_ops(hal_fdcan_context_t* ctx,
                                    const hal_fdcan_ops_t* ops) {
  // 检查参数有效性
  if (ctx == NULL || ops == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查必要的函数指针是否为 NULL
  if (ops->init == NULL || ops->deinit == NULL || ops->send == NULL ||
      ops->receive == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 进入临界区，保护共享资源
  HAL_FDCAN_ENTER_CRITICAL();
  ctx->ops = ops;
  HAL_FDCAN_EXIT_CRITICAL();

  return HAL_FDCAN_OK;
}

/**
 * @brief  初始化 FDCAN
 * @param  ctx FDCAN 上下文指针
 * @param  config FDCAN 配置结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_init(hal_fdcan_context_t* ctx,
                                 const hal_fdcan_config_t* config) {
  // 检查参数有效性
  if (ctx == NULL || config == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(config->instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的初始化函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->init(ctx, config);
  if (result == HAL_FDCAN_OK) {
    ctx->initialized = 1;
    ctx->config = *config;
  }
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  反初始化 FDCAN
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_deinit(hal_fdcan_context_t* ctx,
                                   hal_fdcan_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的反初始化函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->deinit(ctx, instance);
  if (result == HAL_FDCAN_OK) {
    ctx->initialized = 0;
  }
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  发送FDCAN消息
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  message FDCAN消息结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_send(hal_fdcan_context_t* ctx,
                                 hal_fdcan_instance_t instance,
                                 const hal_fdcan_message_t* message) {
  // 检查参数有效性
  if (ctx == NULL || message == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->send == NULL) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的发送函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->send(ctx, instance, message);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  接收FDCAN消息
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  message FDCAN消息结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_receive(hal_fdcan_context_t* ctx,
                                    hal_fdcan_instance_t instance,
                                    hal_fdcan_message_t* message) {
  // 检查参数有效性
  if (ctx == NULL || message == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive == NULL) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  // 进入临界区，调用平台特定的接收函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->receive(ctx, instance, message);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取接收FIFO级别
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  fifo_address FIFO地址
 * @param  level 输出参数，返回FIFO级别
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_receive_level(hal_fdcan_context_t* ctx,
                                               hal_fdcan_instance_t instance,
                                               uint8_t fifo_address,
                                               uint32_t* level) {
  // 检查参数有效性
  if (ctx == NULL || level == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->receive_level == NULL) {
    return HAL_FDCAN_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取接收FIFO级别函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->receive_level(ctx, instance, fifo_address, level);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取发送FIFO级别
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  level 输出参数，返回FIFO级别
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_send_fifo_level(hal_fdcan_context_t* ctx,
                                                 hal_fdcan_instance_t instance,
                                                 uint32_t* level) {
  // 检查参数有效性
  if (ctx == NULL || level == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_send_level == NULL) {
    return HAL_FDCAN_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取发送FIFO级别函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->get_send_level(ctx, instance, level);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  设置过滤器
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  filter FDCAN过滤器结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_set_filter(hal_fdcan_context_t* ctx,
                                       hal_fdcan_instance_t instance,
                                       const hal_fdcan_filter_t* filter) {
  // 检查参数有效性
  if (ctx == NULL || filter == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->set_filter == NULL) {
    return HAL_FDCAN_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的设置过滤器函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->set_filter(ctx, instance, filter);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  设置工作模式
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  mode 工作模式
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_set_mode(hal_fdcan_context_t* ctx,
                                     hal_fdcan_instance_t instance,
                                     hal_fdcan_mode_t mode) {
  // 检查参数有效性
  if (ctx == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->set_mode == NULL) {
    return HAL_FDCAN_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的设置工作模式函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->set_mode(ctx, instance, mode);
  if (result == HAL_FDCAN_OK) {
    ctx->config.mode = mode;
  }
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  获取错误计数
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 * @param  error_count 输出参数，返回错误计数
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_error_count(hal_fdcan_context_t* ctx,
                                            hal_fdcan_instance_t instance,
                                            uint32_t* error_count) {
  // 检查参数有效性
  if (ctx == NULL || error_count == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->get_error_count == NULL) {
    return HAL_FDCAN_ERROR_UNSUPPORTED;
  }

  // 进入临界区，调用平台特定的获取错误计数函数
  HAL_FDCAN_ENTER_CRITICAL();
  hal_fdcan_error_t result = ctx->ops->get_error_count(ctx, instance, error_count);
  HAL_FDCAN_EXIT_CRITICAL();

  return result;
}

/**
 * @brief  FDCAN中断处理
 * @param  ctx FDCAN 上下文指针
 * @param  instance FDCAN 实例
 */
void hal_fdcan_irq_handler(hal_fdcan_context_t* ctx, hal_fdcan_instance_t instance) {
  // 检查参数有效性
  if (ctx == NULL) {
    return;
  }

  // 检查FDCAN实例是否有效
  if (!is_valid_instance(instance)) {
    return;
  }

  // 检查操作函数是否已设置
  if (ctx->ops == NULL || ctx->ops->irq_handler == NULL) {
    return;
  }

  // 调用平台特定的FDCAN中断处理函数
  ctx->ops->irq_handler(ctx, instance);
}

/**
 * @brief 检查FDCAN是否已初始化
 * @param ctx FDCAN 上下文指针
 * @param initialized 输出参数，返回是否已初始化
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_is_initialized(hal_fdcan_context_t* ctx, bool* initialized) {
  if (ctx == NULL || initialized == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  *initialized = (ctx->initialized != 0);

  return HAL_FDCAN_OK;
}

/**
 * @brief 获取当前FDCAN配置
 * @param ctx FDCAN 上下文指针
 * @param config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_config(hal_fdcan_context_t* ctx, hal_fdcan_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  HAL_FDCAN_ENTER_CRITICAL();
  *config = ctx->config;
  HAL_FDCAN_EXIT_CRITICAL();

  return HAL_FDCAN_OK;
}

/**
 * @brief 更新FDCAN配置
 * @param ctx FDCAN 上下文指针
 * @param config FDCAN配置结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_update_config(hal_fdcan_context_t* ctx, const hal_fdcan_config_t* config) {
  if (ctx == NULL || config == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  if (ctx->initialized == 0) {
    return HAL_FDCAN_ERROR_UNINITIALIZED;
  }

  return hal_fdcan_init(ctx, config);
}
