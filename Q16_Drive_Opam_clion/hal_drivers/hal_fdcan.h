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
#include <stdint.h>
#include <stddef.h>

/**
 * @brief FDCAN实例定义
 */
typedef enum {
  HAL_FDCAN_INSTANCE_1 = 0, /**< FDCAN实例1 */
  HAL_FDCAN_INSTANCE_2,     /**< FDCAN实例2 */
  HAL_FDCAN_INSTANCE_3,     /**< FDCAN实例2 */
  HAL_FDCAN_INSTANCE_LEN,   /**< FDCAN实例数量 */
} hal_fdcan_instance_e;

/**
 * @brief FDCAN工作模式定义
 */
typedef enum {
  HAL_FDCAN_MODE_NORMAL = 0,     /**< 正常工作模式 */
  HAL_FDCAN_MODE_LOOPBACK,       /**< 环回模式 */
  HAL_FDCAN_MODE_SILENT,         /**< 静默模式 */
  HAL_FDCAN_MODE_SILENT_LOOPBACK /**< 静默环回模式 */
} hal_fdcan_mode_e;

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
  hal_fdcan_instance_e instance; /**< FDCAN实例 */
  hal_fdcan_mode_e mode;         /**< 工作模式 */
  hal_fdcan_baudrate_t baudrate; /**< 波特率配置 */
  hal_fdcan_filter_t *filters;   /**< 过滤器数组 */
  uint8_t filter_count;          /**< 过滤器数量 */
  uint32_t rx_fifo_size;         /**< RX FIFO大小 */
  uint32_t tx_fifo_size;         /**< TX FIFO大小 */
} hal_fdcan_config_t;

/**
 * @brief FDCAN操作函数指针结构体
 */
typedef struct {
  bool (*init)(const hal_fdcan_config_t *config);
  bool (*send)(hal_fdcan_instance_e instance,
               const hal_fdcan_message_t *message);
  bool (*receive)(hal_fdcan_instance_e instance, hal_fdcan_message_t *message);
  uint32_t (*receive_level)(hal_fdcan_instance_e, uint8_t fifo_address);
  uint32_t (*get_send_level)(hal_fdcan_instance_e);
  bool (*set_filter)(hal_fdcan_instance_e instance,
                     const hal_fdcan_filter_t *filter);
  bool (*set_mode)(hal_fdcan_instance_e instance, hal_fdcan_mode_e mode);
  uint32_t (*get_error_count)(hal_fdcan_instance_e instance);
  void (*irq_handler)(hal_fdcan_instance_e instance);
} hal_fdcan_ops_t;

/* Exported functions prototypes */
void hal_fdcan_set_ops(const hal_fdcan_ops_t *ops);
bool hal_fdcan_init(const hal_fdcan_config_t *config);

bool hal_fdcan_send(hal_fdcan_instance_e instance,
                    const hal_fdcan_message_t *message);
bool hal_fdcan_receive(hal_fdcan_instance_e instance,
                       hal_fdcan_message_t *message);

uint32_t hal_fdcan_get_send_fifo_level(hal_fdcan_instance_e instance);
uint32_t hal_fdcan_get_receive_level(hal_fdcan_instance_e instance,
                                     uint8_t fifo_address);

bool hal_fdcan_set_filter(hal_fdcan_instance_e instance,
                          const hal_fdcan_filter_t *filter);
bool hal_fdcan_set_mode(hal_fdcan_instance_e instance, hal_fdcan_mode_e mode);
uint32_t hal_fdcan_get_error_count(hal_fdcan_instance_e instance);
void hal_fdcan_irq_handler(hal_fdcan_instance_e instance);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_FDCAN_H */
