/**
 * @file    usart_receive.c
 * @brief   串口数据接收 - UART DMA接收与FIFO缓冲
 * @author  fubingyan qq:3245784484
 * @date    2024-06-24
 * @version V2.2
 *
 * @note    负责UART1的DMA接收和FIFO缓冲
 * @note    编码规范: MISRA C:2012, AUTOSAR C++14
 *
 * 修订历史:
 * | 版本  | 日期       | 作者   | 描述                  |
 * |-------|------------|--------|-----------------------|
 * | 2.2   | 2025-04-06 | FOC团队| 移除协议解析到usart_protocol |
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,使用静态FIFO |
 * | 1.0   | 2024-06-24 | fubingyan| 初始版本           |
 */

#include "usart_receive.h"

#include <string.h>

#include "daemon.h"
#include "hal_uart.h"
#include "kfifo.h"
#include "stm32g4xx_hal.h"
#include "usart.h"
#include "usart_protocol.h"

extern DMA_HandleTypeDef hdma_usart1_rx;

/*============================================================================
 * 静态FIFO缓冲区定义 - 避免动态内存分配
 *============================================================================*/
#define UART_RX_FIFO_SIZE 128U
#define UART_TX_FIFO_SIZE 1024U

static uint8_t s_uart1_rx_buffer[UART_RX_FIFO_SIZE];
static uint32_t s_uart1_rx_lock = 0U;

static uint8_t s_uart1_tx_buffer[UART_TX_FIFO_SIZE];
static uint32_t s_uart1_tx_lock = 0U;

static kfifo_t s_fifo_usart1_rx;
static kfifo_t s_fifo_usart1_tx;

/**
 * @brief FIFO缓冲区指针 - 供外部访问
 * @note 使用静态初始化避免NULL检查
 */
kfifo_t* fifo_usart1_rx = &s_fifo_usart1_rx;
kfifo_t* fifo_usart1_tx = &s_fifo_usart1_tx;

/* UART上下文 */
hal_uart_context_t uart1_ctx;

/* 协议上下文实例 */
usart_protocol_context_t uart1_protocol_ctx;

/*============================================================================
 * UART1配置 - DMA模式
 *============================================================================*/
static hal_uart_config_t s_uart1_config = {
    .instance = HAL_UART_INSTANCE_1,
    .baudrate = HAL_UART_BAUDRATE_115200,
    .wordlength = HAL_UART_WORDLENGTH_8B,
    .stopbits = HAL_UART_STOPBITS_1,
    .parity = HAL_UART_PARITY_NONE,
    .hwcontrol = HAL_UART_HWCONTROL_NONE,
    .mode = HAL_UART_MODE_TX_RX,
    .timeout = 100U,
    .dma_config = {
        .dma_mode = HAL_UART_DMA_TX_RX,
        .tx_dma_buffer_size = UART_TX_FIFO_SIZE,
        .rx_dma_buffer_size = UART_RX_FIFO_SIZE,
        .rx_idle_timeout = 10U,
        .circular_mode_rx = true,
    }};

const daemon_config_t daemon_config_rx = {
    .offline_cb = NULL,
    .owner_ptr = NULL,
    .name = "uart1_rx",
    .reload_timeout_ms = 1000U,
    .init_wait_time_ms = 1500U,
};

const daemon_config_t daemon_config_tx = {
    .offline_cb = NULL,
    .owner_ptr = NULL,
    .name = "uart1_tx",
    .reload_timeout_ms = 1000U,
    .init_wait_time_ms = 1500U,
};

daemon_context_t* uart1_rx_ctx = NULL;
daemon_context_t* uart1_tx_ctx = NULL;

/*============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化UART1的中断和DMA接收
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 使用静态FIFO缓冲区,避免动态内存分配
 */
void uart_it_init(void) {
  kfifo_init(&s_fifo_usart1_rx, s_uart1_rx_buffer, UART_RX_FIFO_SIZE,
             &s_uart1_rx_lock);
  fifo_usart1_rx = &s_fifo_usart1_rx;

  kfifo_init(&s_fifo_usart1_tx, s_uart1_tx_buffer, UART_TX_FIFO_SIZE,
             &s_uart1_tx_lock);
  fifo_usart1_tx = &s_fifo_usart1_tx;

  uart1_rx_ctx = daemon_register(&daemon_config_rx);
  uart1_tx_ctx = daemon_register(&daemon_config_tx);
  if (uart1_rx_ctx == NULL || uart1_tx_ctx == NULL) {
    return;
  }

  /* 初始化UART上下文 */
  if (stm32_uart_init_context(&uart1_ctx) != HAL_UART_OK) {
    return;
  }

  if (hal_uart_init(&uart1_ctx, &s_uart1_config) != HAL_UART_OK) {
    return;
  }

  if (fifo_usart1_rx != NULL) {
    (void)hal_uart_receive_dma_to_idle(&uart1_ctx, HAL_UART_INSTANCE_1,
                                       fifo_usart1_rx->buffer,
                                       fifo_usart1_rx->size);
  }

  // 初始化协议模块
  usart_protocol_config_t protocol_cfg = {
      .name = "s7_uart_protocol",
      .uart_ctx = &uart1_ctx,
      .uart_instance = HAL_UART_INSTANCE_1,
  };
  (void)usart_protocol_init(&uart1_protocol_ctx, &protocol_cfg);
}

/**
 * @brief 处理UART数据的任务函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 须在主循环中周期性调用
 */
void uart_process_task(void) {
  static uint8_t rx_data[UART_RX_FIFO_SIZE];

  if (fifo_usart1_rx == NULL) {
    return;
  }

  const uint16_t rx_len =
      kfifo_get(fifo_usart1_rx, rx_data, (uint16_t)sizeof(rx_data));
  if (rx_len > 0U) {
    usart_protocol_feed(&uart1_protocol_ctx, rx_data, rx_len);
  }
  usart_protocol_stream_task(&uart1_protocol_ctx);
  usart_protocol_parse_task(&uart1_protocol_ctx);

  daemon_reload(uart1_rx_ctx);
  daemon_reload(uart1_tx_ctx);
}

/**
 * @brief UART1空闲中断回调函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: YES
 */
void uart1_idle_callback(void) {
  if (RESET != __HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
    (void)__HAL_UART_CLEAR_IDLEFLAG(&huart1);

    if ((fifo_usart1_rx != NULL) && (fifo_usart1_rx->size > 0U)) {
      const uint32_t rx_dataDMALength =
          (uint32_t)(fifo_usart1_rx->size -
                     __HAL_DMA_GET_COUNTER(&hdma_usart1_rx));
      (void)kfifo_move_in(fifo_usart1_rx, rx_dataDMALength);
    }
  }
}
