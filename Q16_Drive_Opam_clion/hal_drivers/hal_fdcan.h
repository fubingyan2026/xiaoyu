//
// Created by fubingyan on 25-9-21.
//
// hal_fdcan.h
#ifndef __HAL_FDCAN_H
#define __HAL_FDCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief FDCAN 操作错误码枚举
 */
typedef enum {
  HAL_FDCAN_OK = 0,              /**< 操作成功 */
  HAL_FDCAN_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_FDCAN_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_FDCAN_ERROR_HARDWARE,      /**< 硬件错误 */
  HAL_FDCAN_ERROR_BUSY,          /**< 设备忙 */
  HAL_FDCAN_ERROR_UNSUPPORTED,   /**< 不支持的操作 */
} hal_fdcan_error_t;

/**
 * @brief FDCAN实例定义
 */
typedef enum __attribute__((packed)) {
  HAL_FDCAN_INSTANCE_1 = 0, /**< FDCAN实例1 */
  HAL_FDCAN_INSTANCE_2,     /**< FDCAN实例2 */
  HAL_FDCAN_INSTANCE_3,     /**< FDCAN实例3 */
  HAL_FDCAN_INSTANCE_LEN,   /**< FDCAN实例数量 */
} hal_fdcan_instance_t;

/**
 * @brief FDCAN工作模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_FDCAN_MODE_NORMAL = 0,     /**< 正常工作模式 */
  HAL_FDCAN_MODE_LOOPBACK,       /**< 环回模式 */
  HAL_FDCAN_MODE_SILENT,         /**< 静默模式 */
  HAL_FDCAN_MODE_SILENT_LOOPBACK /**< 静默环回模式 */
} hal_fdcan_mode_t;

/**
 * @brief FDCAN波特率配置
 */
typedef struct {
  uint32_t prescaler;       /**< 预分频器 */
  uint32_t sync_jump_width; /**< 同步跳转宽度 */
  uint32_t time_seg1;       /**< 时间段1 */
  uint32_t time_seg2;       /**< 时间段2 */
} hal_fdcan_baudrate_t;

/**
 * @brief FDCAN过滤器配置
 */
typedef struct {
  uint32_t id;          /**< 过滤器ID */
  uint32_t mask;        /**< 过滤器掩码 */
  uint8_t filter_index; /**< 过滤器索引 */
  bool is_extended;     /**< 是否为扩展ID */
} hal_fdcan_filter_t;

/**
 * @brief FDCAN消息结构
 */
typedef struct {
  uint32_t id;         /**< 消息ID */
  bool is_extended;    /**< 是否为扩展ID */
  bool is_remote;      /**< 是否为远程帧 */
  uint8_t data_length; /**< 数据长度 */
  uint8_t data[64];    /**< 数据内容（改为固定数组） */
} hal_fdcan_message_t;

/**
 * @brief FDCAN配置结构体
 */
typedef struct {
  hal_fdcan_instance_t instance; /**< FDCAN实例 */
  hal_fdcan_mode_t mode;         /**< 工作模式 */
  hal_fdcan_baudrate_t baudrate; /**< 波特率配置 */
  hal_fdcan_filter_t *filters;   /**< 过滤器数组 */
  uint8_t filter_count;          /**< 过滤器数量 */
  uint32_t rx_fifo_size;         /**< RX FIFO大小 */
  uint32_t tx_fifo_size;         /**< TX FIFO大小 */
} hal_fdcan_config_t;

/**
 * @brief FDCAN 上下文结构体前向声明
 */
typedef struct hal_fdcan_context hal_fdcan_context_t;

/**
 * @brief FDCAN 中断回调函数类型
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param user_data 用户自定义数据指针
 */
typedef void (*hal_fdcan_callback_t)(hal_fdcan_context_t *ctx,
                                     hal_fdcan_instance_t instance,
                                     void *user_data);

/**
 * @brief FDCAN 上下文结构体
 *
 * 用于保存 FDCAN 实例的状态信息，支持多实例操作。
 */
struct hal_fdcan_context {
  const struct hal_fdcan_ops *ops; /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized; /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_fdcan_callback_t callback; /**< 中断回调函数指针 */
  void *user_data;               /**< 用户自定义数据 */
  hal_fdcan_config_t config;     /**< 当前 FDCAN 配置 */
};

/**
 * @brief FDCAN操作函数指针结构体
 */
typedef struct hal_fdcan_ops {
  /**
   * @brief 初始化 FDCAN
   * @param ctx FDCAN 上下文指针
   * @param config FDCAN 配置结构体指针
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*init)(hal_fdcan_context_t *ctx,
                            const hal_fdcan_config_t *config);

  /**
   * @brief 反初始化 FDCAN
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*deinit)(hal_fdcan_context_t *ctx,
                              hal_fdcan_instance_t instance);

  /**
   * @brief 发送FDCAN消息
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param message FDCAN消息结构体指针
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*send)(hal_fdcan_context_t *ctx,
                            hal_fdcan_instance_t instance,
                            const hal_fdcan_message_t *message);

  /**
   * @brief 接收FDCAN消息
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param message FDCAN消息结构体指针
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*receive)(hal_fdcan_context_t *ctx,
                               hal_fdcan_instance_t instance,
                               hal_fdcan_message_t *message);

  /**
   * @brief 获取接收FIFO级别
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param fifo_address FIFO地址
   * @param level 输出参数，返回FIFO级别
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*receive_level)(hal_fdcan_context_t *ctx,
                                     hal_fdcan_instance_t instance,
                                     uint8_t fifo_address, uint32_t *level);

  /**
   * @brief 获取发送FIFO级别
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param level 输出参数，返回FIFO级别
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*get_send_level)(hal_fdcan_context_t *ctx,
                                      hal_fdcan_instance_t instance,
                                      uint32_t *level);

  /**
   * @brief 设置过滤器
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param filter FDCAN过滤器结构体指针
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*set_filter)(hal_fdcan_context_t *ctx,
                                  hal_fdcan_instance_t instance,
                                  const hal_fdcan_filter_t *filter);

  /**
   * @brief 设置工作模式
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param mode 工作模式
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*set_mode)(hal_fdcan_context_t *ctx,
                                hal_fdcan_instance_t instance,
                                hal_fdcan_mode_t mode);

  /**
   * @brief 获取错误计数
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   * @param error_count 输出参数，返回错误计数
   * @return 操作结果错误码
   */
  hal_fdcan_error_t (*get_error_count)(hal_fdcan_context_t *ctx,
                                       hal_fdcan_instance_t instance,
                                       uint32_t *error_count);

  /**
   * @brief FDCAN中断处理
   * @param ctx FDCAN 上下文指针
   * @param instance FDCAN 实例
   */
  void (*irq_handler)(hal_fdcan_context_t *ctx, hal_fdcan_instance_t instance);
} hal_fdcan_ops_t;

/**
 * @brief 进入临界区宏
 *
 * 在多线程/中断环境中保护共享资源，防止竞态条件。
 * 当前为空实现，可根据实际平台需求替换为真正的临界区保护代码。
 */
#define HAL_FDCAN_ENTER_CRITICAL() \
  do {                             \
  } while (0)

/**
 * @brief 退出临界区宏
 *
 * 与 HAL_FDCAN_ENTER_CRITICAL 配对使用。
 */
#define HAL_FDCAN_EXIT_CRITICAL() \
  do {                            \
  } while (0)

/**
 * @brief 设置 FDCAN 操作函数
 * @param ctx FDCAN 上下文指针
 * @param ops FDCAN 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_fdcan_error_t hal_fdcan_set_ops(hal_fdcan_context_t *ctx,
                                    const hal_fdcan_ops_t *ops);

/**
 * @brief 初始化 FDCAN
 * @param ctx FDCAN 上下文指针
 * @param config FDCAN 配置结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_init(hal_fdcan_context_t *ctx,
                                 const hal_fdcan_config_t *config);

/**
 * @brief 反初始化 FDCAN
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_deinit(hal_fdcan_context_t *ctx,
                                   hal_fdcan_instance_t instance);

/**
 * @brief 发送FDCAN消息
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param message FDCAN消息结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_send(hal_fdcan_context_t *ctx,
                                 hal_fdcan_instance_t instance,
                                 const hal_fdcan_message_t *message);

/**
 * @brief 接收FDCAN消息
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param message FDCAN消息结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_receive(hal_fdcan_context_t *ctx,
                                    hal_fdcan_instance_t instance,
                                    hal_fdcan_message_t *message);

/**
 * @brief 获取接收FIFO级别
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param fifo_address FIFO地址
 * @param level 输出参数，返回FIFO级别
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_receive_level(hal_fdcan_context_t *ctx,
                                              hal_fdcan_instance_t instance,
                                              uint8_t fifo_address,
                                              uint32_t *level);

/**
 * @brief 获取发送FIFO级别
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param level 输出参数，返回FIFO级别
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_send_fifo_level(hal_fdcan_context_t *ctx,
                                                hal_fdcan_instance_t instance,
                                                uint32_t *level);

/**
 * @brief 设置过滤器
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param filter FDCAN过滤器结构体指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_set_filter(hal_fdcan_context_t *ctx,
                                       hal_fdcan_instance_t instance,
                                       const hal_fdcan_filter_t *filter);

/**
 * @brief 设置工作模式
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param mode 工作模式
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_set_mode(hal_fdcan_context_t *ctx,
                                     hal_fdcan_instance_t instance,
                                     hal_fdcan_mode_t mode);

/**
 * @brief 获取错误计数
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 * @param error_count 输出参数，返回错误计数
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_error_count(hal_fdcan_context_t *ctx,
                                            hal_fdcan_instance_t instance,
                                            uint32_t *error_count);

/**
 * @brief FDCAN中断处理
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN 实例
 */
void hal_fdcan_irq_handler(hal_fdcan_context_t *ctx,
                           hal_fdcan_instance_t instance);

/**
 * @brief 检查FDCAN是否已初始化
 * @param ctx FDCAN 上下文指针
 * @param initialized 输出参数，返回初始化状态（true=已初始化，false=未初始化）
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_is_initialized(hal_fdcan_context_t *ctx,
                                           bool *initialized);

/**
 * @brief 获取当前FDCAN配置
 * @param ctx FDCAN 上下文指针
 * @param config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_fdcan_error_t hal_fdcan_get_config(hal_fdcan_context_t *ctx,
                                       hal_fdcan_config_t *config);

/**
 * @brief 更新FDCAN配置
 * @param ctx FDCAN 上下文指针
 * @param config 新的配置结构体指针
 * @return 操作结果错误码
 *
 * @note 此函数会更新所有配置参数
 */
hal_fdcan_error_t hal_fdcan_update_config(hal_fdcan_context_t *ctx,
                                          const hal_fdcan_config_t *config);

/**
 * @brief STM32 平台 FDCAN 上下文初始化函数
 * @param ctx FDCAN 上下文指针
 * @return 操作结果错误码
 *
 * 使用此函数初始化 STM32 平台的 FDCAN 上下文，会自动设置好平台特定的操作函数。
 */
hal_fdcan_error_t stm32_fdcan_init_context(hal_fdcan_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_FDCAN_H */
