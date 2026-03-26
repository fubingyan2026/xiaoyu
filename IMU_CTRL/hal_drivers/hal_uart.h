//
// Created by fubingyan on 25-9-27.
//
// hal_uart.h
#ifndef __HAL_UART_H
#define __HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/**
  * @brief UART实例定义
  */
typedef enum
{
    HAL_UART_INSTANCE_1 = 0, /**< UART实例1 */
    HAL_UART_INSTANCE_2, /**< UART实例2 */
    HAL_UART_INSTANCE_3, /**< UART实例3 */
    HAL_UART_INSTANCE_4, /**< UART实例4 */
    HAL_UART_INSTANCE_LEN /**< UART实例数量 */
} hal_uart_instance_e;

/**
  * @brief UART波特率定义
  */
typedef enum
{
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
} hal_uart_baudrate_e;

/**
  * @brief UART数据位定义
  */
typedef enum
{
    HAL_UART_WORDLENGTH_8B = 0, /**< 8数据位 */
    HAL_UART_WORDLENGTH_9B /**< 9数据位 */
} hal_uart_wordlength_e;

/**
  * @brief UART停止位定义
  */
typedef enum
{
    HAL_UART_STOPBITS_1 = 0, /**< 1停止位 */
    HAL_UART_STOPBITS_2 /**< 2停止位 */
} hal_uart_stopbits_e;

/**
  * @brief UART校验位定义
  */
typedef enum
{
    HAL_UART_PARITY_NONE = 0, /**< 无校验 */
    HAL_UART_PARITY_EVEN, /**< 偶校验 */
    HAL_UART_PARITY_ODD /**< 奇校验 */
} hal_uart_parity_e;

/**
  * @brief UART硬件流控制定义
  */
typedef enum
{
    HAL_UART_HWCONTROL_NONE = 0, /**< 无流控制 */
    HAL_UART_HWCONTROL_RTS, /**< RTS流控制 */
    HAL_UART_HWCONTROL_CTS, /**< CTS流控制 */
    HAL_UART_HWCONTROL_RTS_CTS /**< RTS/CTS流控制 */
} hal_uart_hwcontrol_e;

/**
  * @brief UART模式定义
  */
typedef enum
{
    HAL_UART_MODE_TX_RX = 0, /**< 发送和接收模式 */
    HAL_UART_MODE_TX, /**< 仅发送模式 */
    HAL_UART_MODE_RX /**< 仅接收模式 */
} hal_uart_mode_e;

/**
  * @brief UART DMA模式定义
  */
typedef enum
{
    HAL_UART_DMA_DISABLE = 0, /**< 禁用DMA */
    HAL_UART_DMA_TX, /**< 仅发送DMA */
    HAL_UART_DMA_RX, /**< 仅接收DMA */
    HAL_UART_DMA_TX_RX /**< 发送和接收DMA */
} hal_uart_dma_mode_e;

/**
  * @brief UART DMA配置结构体
  */
typedef struct
{
    hal_uart_dma_mode_e dma_mode; /**< DMA模式 */
    uint32_t tx_dma_buffer_size; /**< 发送DMA缓冲区大小 */
    uint32_t rx_dma_buffer_size; /**< 接收DMA缓冲区大小 */
    uint8_t rx_idle_timeout; /**< 接收空闲超时(ms) */
    bool circular_mode_rx; /**< 是否循环模式 */
} hal_uart_dma_config_t;

/**
  * @brief UART配置结构体
  */
typedef struct
{
    hal_uart_instance_e instance; /**< UART实例 */
    hal_uart_baudrate_e baudrate; /**< 波特率 */
    hal_uart_wordlength_e wordlength; /**< 数据位 */
    hal_uart_stopbits_e stopbits; /**< 停止位 */
    hal_uart_parity_e parity; /**< 校验位 */
    hal_uart_hwcontrol_e hwcontrol; /**< 硬件流控制 */
    hal_uart_mode_e mode; /**< 工作模式 */
    uint32_t timeout; /**< 超时时间(ms) */
    hal_uart_dma_config_t dma_config; /**< DMA配置 */
} hal_uart_config_t;

/**
  * @brief UART DMA传输状态
  */
typedef struct
{
    bool tx_busy; /**< 发送忙标志 */
    bool rx_busy; /**< 接收忙标志 */
    uint32_t tx_total_size; /**< 发送总字节数 */
    uint32_t rx_total_size; /**< 接收总字节数 */
    uint32_t tx_current_pos; /**< 发送当前位置 */
    uint32_t rx_current_pos; /**< 接收当前位置 */
} hal_uart_dma_status_t;

/**
  * @brief UART操作函数指针结构体
  */
typedef struct
{
    bool (*init)(const hal_uart_config_t* config);
    bool (*deinit)(hal_uart_instance_e instance);
    bool (*send)(hal_uart_instance_e instance, const uint8_t* data, uint16_t size);
    bool (*receive)(hal_uart_instance_e instance, uint8_t* data, uint16_t size);
    uint16_t (*send_async)(hal_uart_instance_e instance, const uint8_t* data, uint16_t size);
    uint16_t (*receive_async)(hal_uart_instance_e instance, uint8_t* data, uint16_t size);
    bool (*send_dma)(hal_uart_instance_e instance, const uint8_t* data, uint32_t size);
    bool (*receive_dma)(hal_uart_instance_e instance, uint8_t* data, uint32_t size);
    bool (*receive_dma_to_idle)(hal_uart_instance_e instance, uint8_t* data, uint32_t size);
    bool (*abort_send)(hal_uart_instance_e instance);
    bool (*abort_receive)(hal_uart_instance_e instance);
    bool (*abort_send_dma)(hal_uart_instance_e instance);
    bool (*abort_receive_dma)(hal_uart_instance_e instance);
    uint32_t (*get_tx_count)(hal_uart_instance_e instance);
    uint32_t (*get_rx_count)(hal_uart_instance_e instance);
    hal_uart_dma_status_t (*get_dma_status)(hal_uart_instance_e instance);
    bool (*set_baudrate)(hal_uart_instance_e instance, hal_uart_baudrate_e baudrate);
    bool (*set_rx_timeout)(hal_uart_instance_e instance, uint32_t timeout);
    void (*irq_handler)(hal_uart_instance_e instance);
    void (*dma_irq_handler)(hal_uart_instance_e instance);
} hal_uart_ops_t;

/* Exported functions prototypes */
void hal_uart_set_ops(const hal_uart_ops_t* ops);
bool hal_uart_init(const hal_uart_config_t* config);
bool hal_uart_deinit(hal_uart_instance_e instance);
bool hal_uart_send(hal_uart_instance_e instance, const uint8_t* data, uint16_t size);
bool hal_uart_receive(hal_uart_instance_e instance, uint8_t* data, uint16_t size);
uint16_t hal_uart_send_async(hal_uart_instance_e instance, const uint8_t* data, uint16_t size);
uint16_t hal_uart_receive_async(hal_uart_instance_e instance, uint8_t* data, uint16_t size);
bool hal_uart_send_dma(hal_uart_instance_e instance, const uint8_t* data, uint32_t size);
bool hal_uart_receive_dma(hal_uart_instance_e instance, uint8_t* data, uint32_t size);
bool hal_uart_receive_dma_to_idle(hal_uart_instance_e instance, uint8_t* data, uint32_t size);
bool hal_uart_abort_send(hal_uart_instance_e instance);
bool hal_uart_abort_receive(hal_uart_instance_e instance);
bool hal_uart_abort_send_dma(hal_uart_instance_e instance);
bool hal_uart_abort_receive_dma(hal_uart_instance_e instance);
uint32_t hal_uart_get_tx_count(hal_uart_instance_e instance);
uint32_t hal_uart_get_rx_count(hal_uart_instance_e instance);
hal_uart_dma_status_t hal_uart_get_dma_status(hal_uart_instance_e instance);
bool hal_uart_set_baudrate(hal_uart_instance_e instance, hal_uart_baudrate_e baudrate);
bool hal_uart_set_rx_timeout(hal_uart_instance_e instance, uint32_t timeout);
void hal_uart_irq_handler(hal_uart_instance_e instance);
void hal_uart_dma_irq_handler(hal_uart_instance_e instance);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_UART_H */
