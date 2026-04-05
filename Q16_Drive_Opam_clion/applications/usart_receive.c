/**
 * @file    usart_receive.c
 * @brief   串口数据接收 - UART DMA接收与协议解析实现
 * @author  fubingyan qq:3245784484
 * @date    2024-06-24
 * @version V2.1
 *
 * @note    负责UART1的DMA接收、FIFO缓冲和A5协议解析
 * @note    编码规范: MISRA C:2012, AUTOSAR C++14
 *
 * 修订历史:
 * | 版本  | 日期       | 作者   | 描述                  |
 * |-------|------------|--------|-----------------------|
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,使用静态FIFO |
 * | 1.0   | 2024-06-24 | fubingyan| 初始版本           |
 */

#include "usart_receive.h"

#include <string.h>

#include "app.h"
#include "crc16.h"
#include "daemon.h"
#include "debug/debug.h"
#include "hal_uart.h"
#include "kfifo/kfifo.h"
#include "lwshell/lwshell.h"
#include "protocol_parser.h"
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
static kfifo_t* s_fifo_usart1_rx_ptr = NULL;

static kfifo_t s_fifo_usart1_tx;
static kfifo_t* s_fifo_usart1_tx_ptr = NULL;

/**
 * @brief FIFO缓冲区指针 - 供外部访问
 * @note 使用静态初始化避免NULL检查
 */
kfifo_t* fifo_usart1_rx = &s_fifo_usart1_rx;
kfifo_t* fifo_usart1_tx = &s_fifo_usart1_tx;

/* UART上下文 */
hal_uart_context_t uart1_ctx;

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

/*============================================================================
 * A5协议帧长度计算回调
 *============================================================================*/

/**
 * @brief 计算A5协议帧长度
 * @param[in] buffer 数据缓冲区
 * @param[in] len 当前数据长度
 * @return uint16_t 完整帧长度,0表示不完整
 */
/*============================================================================
 * S7协议帧长度计算回调
 *============================================================================*/

/**
 * @brief 计算S7协议帧长度
 * @param[in] buffer 数据缓冲区
 * @param[in] len 当前数据长度
 * @return uint16_t 完整帧长度,0表示不完整
 * @note S7帧格式: head(2) + proto_id(1) + msg_type(1) + func(1) + data(n) +
 * crc(2) + tail(1) 最小帧长: 8字节 (无数据时)
 */
uint16_t s7_get_frame_len(uint8_t* buffer, uint16_t len) {
  if (len < 8U) {
    return 0U;
  }

  for (uint16_t i = 7U; i < len; i++) {
    if (buffer[i] == S7_FRAME_TAIL) {
      return (uint16_t)(i + 1U);
    }
  }

  return 0U;
}

/*============================================================================
 * S7协议校验回调
 *============================================================================*/

/**
 * @brief S7协议帧校验 - CRC16 Modbus
 * @param[in] buffer 数据缓冲区
 * @param[in] len 数据长度
 * @return 协议解析器错误码
 */
protocol_parser_error_t s7_check_cb(uint8_t* buffer, uint16_t len) {
  if (len < 8U) {
    return PROTOCOL_PARSER_ERROR_CHECKSUM;
  }
  /* CRC16计算范围: 从协议ID到CRC之前 */
  uint16_t calc = crc16_modbus(&buffer[2U], (uint16_t)(len - 5U));
  uint16_t recv =
      (uint16_t)buffer[len - 3U] | ((uint16_t)buffer[len - 2U] << 8);
  return (calc == recv) ? PROTOCOL_PARSER_OK : PROTOCOL_PARSER_ERROR_CHECKSUM;
}

protocol_parser_context_t s_s7_protocol_frame;

/**
 * @brief 初始化S7协议
 * @param[in] frame 协议帧结构指针
 */
void init_s7_protocol(protocol_parser_context_t* frame) {
  static uint8_t out_buf[UART_RX_FIFO_SIZE];
  protocol_parser_config_t cfg = {
      .name = "S7_Protocol",
      .header = (const uint8_t*)"\x68\x68",
      .header_len = 2U,
      .footer = (const uint8_t*)"\x16",
      .footer_len = 1U,
      .input_buffer_len = UART_RX_FIFO_SIZE,
      .output_buffer = out_buf,
      .output_buffer_len = sizeof(out_buf),
      .get_len_cb = s7_get_frame_len,
      .check_cb = s7_check_cb,
  };
  (void)protocol_parser_init(frame, &cfg);
}

/*============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化UART1的中断和DMA接收
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 使用静态FIFO缓冲区,避免动态内存分配
 */
void uart_it_init(void) {
  s_fifo_usart1_rx_ptr =
      kfifo_init(s_uart1_rx_buffer, UART_RX_FIFO_SIZE, 0, &s_uart1_rx_lock);

  if (s_fifo_usart1_rx_ptr != NULL) {
    (void)memcpy(&s_fifo_usart1_rx, s_fifo_usart1_rx_ptr, sizeof(kfifo_t));
  }
  fifo_usart1_rx = &s_fifo_usart1_rx;

  s_fifo_usart1_tx_ptr =
      kfifo_init(s_uart1_tx_buffer, UART_TX_FIFO_SIZE, 0, &s_uart1_tx_lock);

  if (s_fifo_usart1_tx_ptr != NULL) {
    (void)memcpy(&s_fifo_usart1_tx, s_fifo_usart1_tx_ptr, sizeof(kfifo_t));
  }
  fifo_usart1_tx = &s_fifo_usart1_tx;

  const daemon_config_t daemon_config_rx = {
      .offline_cb = NULL,
      .owner_ptr = NULL,
      .name = "uart1_rx",
      .reload_timeout_ms = 1000U,
      .init_wait_time_ms = 1500U,
  };
  (void)daemon_register(&daemon_config_rx);

  const daemon_config_t daemon_config_tx = {
      .offline_cb = NULL,
      .owner_ptr = NULL,
      .name = "uart1_tx",
      .reload_timeout_ms = 1000U,
      .init_wait_time_ms = 1500U,
  };
  (void)daemon_register(&daemon_config_tx);

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

  (void)init_s7_protocol(&s_s7_protocol_frame);
}

/**
 * @brief 处理UART数据的任务函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 须在主循环中周期性调用
 */
void uart_process_task(void) {
  static uint8_t rx_data[UART_RX_FIFO_SIZE];
  uint16_t flen;
  uint8_t* p_frame;

  if (fifo_usart1_rx == NULL) {
    return;
  }

  const uint16_t rx_len =
      kfifo_get(fifo_usart1_rx, rx_data, (uint16_t)sizeof(rx_data));
  if (rx_len > 0U) {
    (void)protocol_parser_feed(&s_s7_protocol_frame, rx_data, rx_len);
    // lwshell_input(rx_data, rx_len);
  }

  const protocol_parser_error_t err =
      protocol_parser_parse(&s_s7_protocol_frame, &flen, &p_frame);
  if (err == PROTOCOL_PARSER_OK && p_frame != NULL) {
    usart_protocol_process_frame(p_frame, flen);
  }

  (void)protocol_parser_tick(&s_s7_protocol_frame);
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
