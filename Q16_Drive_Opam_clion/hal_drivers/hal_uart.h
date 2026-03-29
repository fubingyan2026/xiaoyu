//
// Created by fubingyan on 25-9-27.
//

#ifndef __HAL_UART_H
#define __HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UART 操作错误码枚举
 */
typedef enum {
  HAL_UART_OK = 0,              /**< 操作成功 */
  HAL_UART_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_UART_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_UART_ERROR_HARDWARE,      /**< 硬件错误 */
  HAL_UART_ERROR_BUSY,          /**< 设备忙 */
  HAL_UART_ERROR_UNSUPPORTED,   /**< 不支持的操作 */
  HAL_UART_ERROR_TIMEOUT,       /**< 超时 */
} hal_uart_error_t;

/**
 * @brief UART实例定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_INSTANCE_1 = 0, /**< UART实例1 */
  HAL_UART_INSTANCE_2,     /**< UART实例2 */
  HAL_UART_INSTANCE_3,     /**< UART实例3 */
  HAL_UART_INSTANCE_4,     /**< UART实例4 */
  HAL_UART_INSTANCE_LEN    /**< UART实例数量 */
} hal_uart_instance_t;

/**
 * @brief UART波特率定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_BAUDRATE_9600 = 9600,
  HAL_UART_BAUDRATE_19200 = 19200,
  HAL_UART_BAUDRATE_38400 = 38400,
  HAL_UART_BAUDRATE_57600 = 57600,
  HAL_UART_BAUDRATE_115200 = 115200,
  HAL_UART_BAUDRATE_230400 = 230400,
  HAL_UART_BAUDRATE_460800 = 460800,
  HAL_UART_BAUDRATE_921600 = 921600,
  HAL_UART_BAUDRATE_2000000 = 2000000,
  HAL_UART_BAUDRATE_3000000 = 3000000
} hal_uart_baudrate_t;

/**
 * @brief UART数据位定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_WORDLENGTH_8B = 0, /**< 8数据位 */
  HAL_UART_WORDLENGTH_9B      /**< 9数据位 */
} hal_uart_wordlength_t;

/**
 * @brief UART停止位定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_STOPBITS_1 = 0, /**< 1停止位 */
  HAL_UART_STOPBITS_2      /**< 2停止位 */
} hal_uart_stopbits_t;

/**
 * @brief UART校验位定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_PARITY_NONE = 0, /**< 无校验 */
  HAL_UART_PARITY_EVEN,     /**< 偶校验 */
  HAL_UART_PARITY_ODD       /**< 奇校验 */
} hal_uart_parity_t;

/**
 * @brief UART硬件流控制定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_HWCONTROL_NONE = 0, /**< 无流控制 */
  HAL_UART_HWCONTROL_RTS,      /**< RTS流控制 */
  HAL_UART_HWCONTROL_CTS,      /**< CTS流控制 */
  HAL_UART_HWCONTROL_RTS_CTS   /**< RTS/CTS流控制 */
} hal_uart_hwcontrol_t;

/**
 * @brief UART模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_MODE_TX_RX = 0, /**< 发送和接收模式 */
  HAL_UART_MODE_TX,        /**< 仅发送模式 */
  HAL_UART_MODE_RX         /**< 仅接收模式 */
} hal_uart_mode_t;

/**
 * @brief UART DMA模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_UART_DMA_DISABLE = 0, /**< 禁用DMA */
  HAL_UART_DMA_TX,          /**< 仅发送DMA */
  HAL_UART_DMA_RX,          /**< 仅接收DMA */
  HAL_UART_DMA_TX_RX        /**< 发送和接收DMA */
} hal_uart_dma_mode_t;

/**
 * @brief UART DMA配置结构体
 */
typedef struct {
  hal_uart_dma_mode_t dma_mode; /**< DMA模式 */
  uint32_t tx_dma_buffer_size;  /**< 发送DMA缓冲区大小 */
  uint32_t rx_dma_buffer_size;  /**< 接收DMA缓冲区大小 */
  uint8_t rx_idle_timeout;      /**< 接收空闲超时(ms) */
  bool circular_mode_rx;        /**< 是否循环模式 */
} hal_uart_dma_config_t;

/**
 * @brief UART配置结构体
 */
typedef struct {
  hal_uart_instance_t instance;     /**< UART实例 */
  hal_uart_baudrate_t baudrate;     /**< 波特率 */
  hal_uart_wordlength_t wordlength; /**< 数据位 */
  hal_uart_stopbits_t stopbits;     /**< 停止位 */
  hal_uart_parity_t parity;         /**< 校验位 */
  hal_uart_hwcontrol_t hwcontrol;   /**< 硬件流控制 */
  hal_uart_mode_t mode;             /**< 工作模式 */
  uint32_t timeout;                 /**< 超时时间(ms) */
  hal_uart_dma_config_t dma_config; /**< DMA配置 */
} hal_uart_config_t;

/**
 * @brief UART DMA传输状态
 */
typedef struct {
  bool tx_busy;            /**< 发送忙标志 */
  bool rx_busy;            /**< 接收忙标志 */
  uint32_t tx_total_size;  /**< 发送总字节数 */
  uint32_t rx_total_size;  /**< 接收总字节数 */
  uint32_t tx_current_pos; /**< 发送当前位置 */
  uint32_t rx_current_pos; /**< 接收当前位置 */
} hal_uart_dma_status_t;

/**
 * @brief UART 上下文结构体前向声明
 */
typedef struct hal_uart_context hal_uart_context_t;

/**
 * @brief UART 中断回调函数类型
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param user_data 用户自定义数据指针
 */
typedef void (*hal_uart_callback_t)(hal_uart_context_t *ctx,
                                     hal_uart_instance_t instance,
                                     void *user_data);

/**
 * @brief UART 上下文结构体
 *
 * 用于保存 UART 实例的状态信息，支持多实例操作。
 */
struct hal_uart_context {
  const struct hal_uart_ops *ops;     /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized;        /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_uart_callback_t callback;         /**< 中断回调函数指针 */
  void *user_data;                      /**< 用户自定义数据 */
  hal_uart_config_t config;            /**< 当前 UART 配置 */
};

/**
 * @brief UART 操作函数结构体
 *
 * 定义了平台特定的 UART 操作接口，各平台需要实现这些函数。
 */
typedef struct hal_uart_ops {
  /**
   * @brief 初始化 UART
   * @param ctx UART 上下文指针
   * @param config UART 配置结构体指针
   * @return 操作结果错误码
   */
  hal_uart_error_t (*init)(hal_uart_context_t *ctx,
                           const hal_uart_config_t *config);

  /**
   * @brief 反初始化 UART
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @return 操作结果错误码
   */
  hal_uart_error_t (*deinit)(hal_uart_context_t *ctx,
                             hal_uart_instance_t instance);

  /**
   * @brief 发送数据（阻塞）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 要发送的数据指针
   * @param size 要发送的数据大小
   * @return 操作结果错误码
   */
  hal_uart_error_t (*send)(hal_uart_context_t *ctx,
                           hal_uart_instance_t instance,
                           const uint8_t *data,
                           uint16_t size);

  /**
   * @brief 接收数据（阻塞）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 接收数据缓冲区指针
   * @param size 要接收的数据大小
   * @return 操作结果错误码
   */
  hal_uart_error_t (*receive)(hal_uart_context_t *ctx,
                              hal_uart_instance_t instance,
                              uint8_t *data,
                              uint16_t size);

  /**
   * @brief 发送数据（异步）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 要发送的数据指针
   * @param size 要发送的数据大小
   * @param sent_size 输出参数，返回已发送的字节数
   * @return 操作结果错误码
   */
  hal_uart_error_t (*send_async)(hal_uart_context_t *ctx,
                                  hal_uart_instance_t instance,
                                  const uint8_t *data,
                                  uint16_t size,
                                  uint16_t *sent_size);

  /**
   * @brief 接收数据（异步）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 接收数据缓冲区指针
   * @param size 要接收的数据大小
   * @param received_size 输出参数，返回已接收的字节数
   * @return 操作结果错误码
   */
  hal_uart_error_t (*receive_async)(hal_uart_context_t *ctx,
                                     hal_uart_instance_t instance,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint16_t *received_size);

  /**
   * @brief 发送数据（DMA）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 要发送的数据指针
   * @param size 要发送的数据大小
   * @return 操作结果错误码
   */
  hal_uart_error_t (*send_dma)(hal_uart_context_t *ctx,
                               hal_uart_instance_t instance,
                               const uint8_t *data,
                               uint32_t size);

  /**
   * @brief 接收数据（DMA）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 接收数据缓冲区指针
   * @param size 要接收的数据大小
   * @return 操作结果错误码
   */
  hal_uart_error_t (*receive_dma)(hal_uart_context_t *ctx,
                                  hal_uart_instance_t instance,
                                  uint8_t *data,
                                  uint32_t size);

  /**
   * @brief 接收数据（DMA，空闲中断检测）
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param data 接收数据缓冲区指针
   * @param size 要接收的数据大小
   * @return 操作结果错误码
   */
  hal_uart_error_t (*receive_dma_to_idle)(hal_uart_context_t *ctx,
                                            hal_uart_instance_t instance,
                                            uint8_t *data,
                                            uint32_t size);

  /**
   * @brief 中止发送
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @return 操作结果错误码
   */
  hal_uart_error_t (*abort_send)(hal_uart_context_t *ctx,
                                 hal_uart_instance_t instance);

  /**
   * @brief 中止接收
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @return 操作结果错误码
   */
  hal_uart_error_t (*abort_receive)(hal_uart_context_t *ctx,
                                    hal_uart_instance_t instance);

  /**
   * @brief 中止DMA发送
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @return 操作结果错误码
   */
  hal_uart_error_t (*abort_send_dma)(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance);

  /**
   * @brief 中止DMA接收
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @return 操作结果错误码
   */
  hal_uart_error_t (*abort_receive_dma)(hal_uart_context_t *ctx,
                                         hal_uart_instance_t instance);

  /**
   * @brief 获取发送字节数
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param count 输出参数，返回发送字节数
   * @return 操作结果错误码
   */
  hal_uart_error_t (*get_tx_count)(hal_uart_context_t *ctx,
                                   hal_uart_instance_t instance,
                                   uint32_t *count);

  /**
   * @brief 获取接收字节数
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param count 输出参数，返回接收字节数
   * @return 操作结果错误码
   */
  hal_uart_error_t (*get_rx_count)(hal_uart_context_t *ctx,
                                   hal_uart_instance_t instance,
                                   uint32_t *count);

  /**
   * @brief 获取DMA状态
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param status 输出参数，返回DMA状态
   * @return 操作结果错误码
   */
  hal_uart_error_t (*get_dma_status)(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance,
                                      hal_uart_dma_status_t *status);

  /**
   * @brief 设置波特率
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param baudrate 波特率
   * @return 操作结果错误码
   */
  hal_uart_error_t (*set_baudrate)(hal_uart_context_t *ctx,
                                    hal_uart_instance_t instance,
                                    hal_uart_baudrate_t baudrate);

  /**
   * @brief 设置接收超时
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   * @param timeout 超时时间(ms)
   * @return 操作结果错误码
   */
  hal_uart_error_t (*set_rx_timeout)(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance,
                                      uint32_t timeout);

  /**
   * @brief UART中断处理
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   */
  void (*irq_handler)(hal_uart_context_t *ctx, hal_uart_instance_t instance);

  /**
   * @brief DMA中断处理
   * @param ctx UART 上下文指针
   * @param instance UART 实例
   */
  void (*dma_irq_handler)(hal_uart_context_t *ctx, hal_uart_instance_t instance);
} hal_uart_ops_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/**
 * @brief 进入临界区宏
 *
 * 在多线程/中断环境中保护共享资源，防止竞态条件。
 * 当前为空实现，可根据实际平台需求替换为真正的临界区保护代码。
 */
#define HAL_UART_ENTER_CRITICAL() \
  do {                            \
  } while (0)

/**
 * @brief 退出临界区宏
 *
 * 与 HAL_UART_ENTER_CRITICAL 配对使用。
 */
#define HAL_UART_EXIT_CRITICAL() \
  do {                           \
  } while (0)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化 UART
 * @param ctx UART 上下文指针
 * @param config UART 配置结构体指针
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_init(hal_uart_context_t *ctx,
                               const hal_uart_config_t *config);

/**
 * @brief 反初始化 UART
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_deinit(hal_uart_context_t *ctx,
                                 hal_uart_instance_t instance);

/**
 * @brief 发送数据（阻塞）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 要发送的数据指针
 * @param size 要发送的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send(hal_uart_context_t *ctx,
                               hal_uart_instance_t instance,
                               const uint8_t *data,
                               uint16_t size);

/**
 * @brief 接收数据（阻塞）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 接收数据缓冲区指针
 * @param size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive(hal_uart_context_t *ctx,
                                  hal_uart_instance_t instance,
                                  uint8_t *data,
                                  uint16_t size);

/**
 * @brief 发送数据（异步）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 要发送的数据指针
 * @param size 要发送的数据大小
 * @param sent_size 输出参数，返回已发送的字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send_async(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance,
                                      const uint8_t *data,
                                      uint16_t size,
                                      uint16_t *sent_size);

/**
 * @brief 接收数据（异步）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 接收数据缓冲区指针
 * @param size 要接收的数据大小
 * @param received_size 输出参数，返回已接收的字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_async(hal_uart_context_t *ctx,
                                         hal_uart_instance_t instance,
                                         uint8_t *data,
                                         uint16_t size,
                                         uint16_t *received_size);

/**
 * @brief 发送数据（DMA）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 要发送的数据指针
 * @param size 要发送的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_send_dma(hal_uart_context_t *ctx,
                                    hal_uart_instance_t instance,
                                    const uint8_t *data,
                                    uint32_t size);

/**
 * @brief 接收数据（DMA）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 接收数据缓冲区指针
 * @param size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_dma(hal_uart_context_t *ctx,
                                       hal_uart_instance_t instance,
                                       uint8_t *data,
                                       uint32_t size);

/**
 * @brief 接收数据（DMA，空闲中断检测）
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param data 接收数据缓冲区指针
 * @param size 要接收的数据大小
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_receive_dma_to_idle(hal_uart_context_t *ctx,
                                               hal_uart_instance_t instance,
                                               uint8_t *data,
                                               uint32_t size);

/**
 * @brief 中止发送
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_send(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance);

/**
 * @brief 中止接收
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_receive(hal_uart_context_t *ctx,
                                         hal_uart_instance_t instance);

/**
 * @brief 中止DMA发送
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_send_dma(hal_uart_context_t *ctx,
                                           hal_uart_instance_t instance);

/**
 * @brief 中止DMA接收
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_abort_receive_dma(hal_uart_context_t *ctx,
                                              hal_uart_instance_t instance);

/**
 * @brief 获取发送字节数
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param count 输出参数，返回发送字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_tx_count(hal_uart_context_t *ctx,
                                        hal_uart_instance_t instance,
                                        uint32_t *count);

/**
 * @brief 获取接收字节数
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param count 输出参数，返回接收字节数
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_rx_count(hal_uart_context_t *ctx,
                                        hal_uart_instance_t instance,
                                        uint32_t *count);

/**
 * @brief 获取DMA状态
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param status 输出参数，返回DMA状态
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_dma_status(hal_uart_context_t *ctx,
                                           hal_uart_instance_t instance,
                                           hal_uart_dma_status_t *status);

/**
 * @brief 设置波特率
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param baudrate 波特率
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_baudrate(hal_uart_context_t *ctx,
                                         hal_uart_instance_t instance,
                                         hal_uart_baudrate_t baudrate);

/**
 * @brief 设置接收超时
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param timeout 超时时间(ms)
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_rx_timeout(hal_uart_context_t *ctx,
                                           hal_uart_instance_t instance,
                                           uint32_t timeout);

/**
 * @brief UART中断处理
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 */
void hal_uart_irq_handler(hal_uart_context_t *ctx, hal_uart_instance_t instance);

/**
 * @brief DMA中断处理
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 */
void hal_uart_dma_irq_handler(hal_uart_context_t *ctx, hal_uart_instance_t instance);

/**
 * @brief 设置 UART 操作函数
 * @param ctx UART 上下文指针
 * @param ops UART 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_uart_error_t hal_uart_set_ops(hal_uart_context_t *ctx,
                                  const hal_uart_ops_t *ops);

/**
 * @brief STM32 平台 UART 上下文初始化函数
 * @param ctx UART 上下文指针
 * @return 操作结果错误码
 *
 * 使用此函数初始化 STM32 平台的 UART 上下文，会自动设置好平台特定的操作函数。
 */
hal_uart_error_t stm32_uart_init_context(hal_uart_context_t *ctx);

/**
 * @brief 检查UART是否已初始化
 * @param ctx UART 上下文指针
 * @param initialized 输出参数，返回初始化状态（true=已初始化，false=未初始化）
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_is_initialized(hal_uart_context_t *ctx, bool *initialized);

/**
 * @brief 获取当前UART配置
 * @param ctx UART 上下文指针
 * @param config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_get_config(hal_uart_context_t *ctx, hal_uart_config_t *config);

/**
 * @brief 更新UART配置
 * @param ctx UART 上下文指针
 * @param config 新的配置结构体指针
 * @return 操作结果错误码
 *
 * @note 此函数会更新所有配置参数
 */
hal_uart_error_t hal_uart_update_config(hal_uart_context_t *ctx, const hal_uart_config_t *config);

/**
 * @brief 设置数据位
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param wordlength 数据位
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_wordlength(hal_uart_context_t *ctx,
                                          hal_uart_instance_t instance,
                                          hal_uart_wordlength_t wordlength);

/**
 * @brief 设置停止位
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param stopbits 停止位
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_stopbits(hal_uart_context_t *ctx,
                                        hal_uart_instance_t instance,
                                        hal_uart_stopbits_t stopbits);

/**
 * @brief 设置校验位
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param parity 校验位
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_parity(hal_uart_context_t *ctx,
                                      hal_uart_instance_t instance,
                                      hal_uart_parity_t parity);

/**
 * @brief 设置硬件流控制
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param hwcontrol 硬件流控制
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_hwcontrol(hal_uart_context_t *ctx,
                                         hal_uart_instance_t instance,
                                         hal_uart_hwcontrol_t hwcontrol);

/**
 * @brief 设置工作模式
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @param mode 工作模式
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_set_mode(hal_uart_context_t *ctx,
                                    hal_uart_instance_t instance,
                                    hal_uart_mode_t mode);

/**
 * @brief 刷新发送缓冲区
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_flush_tx(hal_uart_context_t *ctx, hal_uart_instance_t instance);

/**
 * @brief 刷新接收缓冲区
 * @param ctx UART 上下文指针
 * @param instance UART 实例
 * @return 操作结果错误码
 */
hal_uart_error_t hal_uart_flush_rx(hal_uart_context_t *ctx, hal_uart_instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_UART_H */
