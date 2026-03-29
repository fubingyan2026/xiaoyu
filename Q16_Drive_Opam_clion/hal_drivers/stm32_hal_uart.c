//
// Created by fubingyan on 25-9-27.
//

/**
 * @file    stm32_hal_uart.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-27
 * @brief   STM32平台硬件抽象层 - UART实现
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
#include <string.h>

#include "hal_uart.h"
#include "main.h"
#include "stm32g4xx_hal.h"

/* Private variables ---------------------------------------------------------*/
/* UART句柄定义 */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;

/* DMA句柄定义 */
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_uart4_tx;
extern DMA_HandleTypeDef hdma_uart4_rx;

/* UART实例映射表 */
static UART_HandleTypeDef* uart_handle_map[HAL_UART_INSTANCE_LEN] = {
    &huart1,
    NULL,
    NULL,
    NULL,
};

/* UART实例基地址映射 */
static USART_TypeDef* uart_base_map[HAL_UART_INSTANCE_LEN] = {
    USART1,
    USART2,
    USART3,
    UART4,
};

/* DMA句柄映射表 */
static DMA_HandleTypeDef* uart_dma_tx_handle_map[HAL_UART_INSTANCE_LEN] = {
    &hdma_usart1_tx,
    NULL,
    NULL,
    NULL,
};

static DMA_HandleTypeDef* uart_dma_rx_handle_map[HAL_UART_INSTANCE_LEN] = {
    &hdma_usart1_rx,
    NULL,
    NULL,
    NULL,
};

/* DMA状态跟踪 */
typedef struct {
  bool tx_busy;
  bool rx_busy;
  uint32_t tx_total_size;
  uint32_t rx_total_size;
  uint32_t tx_current_pos;
  uint32_t rx_current_pos;
  uint32_t rx_timeout;
  uint32_t last_rx_tick;
  bool circular_mode;
  uint32_t rx_half_transfers;
  uint32_t rx_full_transfers;
} uart_dma_status_t;

static uart_dma_status_t dma_status[HAL_UART_INSTANCE_LEN] = {0};

/* Private function prototypes -----------------------------------------------*/
static hal_uart_error_t stm32_uart_init(hal_uart_context_t* ctx,
                                        const hal_uart_config_t* config);
static hal_uart_error_t stm32_uart_deinit(hal_uart_context_t* ctx,
                                          hal_uart_instance_t instance);
static hal_uart_error_t stm32_uart_send(hal_uart_context_t* ctx,
                                        hal_uart_instance_t instance,
                                        const uint8_t* data, uint16_t size);
static hal_uart_error_t stm32_uart_receive(hal_uart_context_t* ctx,
                                           hal_uart_instance_t instance,
                                           uint8_t* data, uint16_t size);
static hal_uart_error_t stm32_uart_send_async(hal_uart_context_t* ctx,
                                              hal_uart_instance_t instance,
                                              const uint8_t* data,
                                              uint16_t size,
                                              uint16_t* sent_size);
static hal_uart_error_t stm32_uart_receive_async(hal_uart_context_t* ctx,
                                                 hal_uart_instance_t instance,
                                                 uint8_t* data, uint16_t size,
                                                 uint16_t* received_size);
static hal_uart_error_t stm32_uart_send_dma(hal_uart_context_t* ctx,
                                            hal_uart_instance_t instance,
                                            const uint8_t* data, uint32_t size);
static hal_uart_error_t stm32_uart_receive_dma(hal_uart_context_t* ctx,
                                               hal_uart_instance_t instance,
                                               uint8_t* data, uint32_t size);
static hal_uart_error_t stm32_uart_receive_dma_to_idle(
    hal_uart_context_t* ctx, hal_uart_instance_t instance, uint8_t* data,
    uint32_t size);
static hal_uart_error_t stm32_uart_abort_send(hal_uart_context_t* ctx,
                                              hal_uart_instance_t instance);
static hal_uart_error_t stm32_uart_abort_receive(hal_uart_context_t* ctx,
                                                 hal_uart_instance_t instance);
static hal_uart_error_t stm32_uart_abort_send_dma(hal_uart_context_t* ctx,
                                                  hal_uart_instance_t instance);
static hal_uart_error_t stm32_uart_abort_receive_dma(
    hal_uart_context_t* ctx, hal_uart_instance_t instance);
static hal_uart_error_t stm32_uart_get_tx_count(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                uint32_t* count);
static hal_uart_error_t stm32_uart_get_rx_count(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                uint32_t* count);
static hal_uart_error_t stm32_uart_get_dma_status(
    hal_uart_context_t* ctx, hal_uart_instance_t instance,
    hal_uart_dma_status_t* status);
static hal_uart_error_t stm32_uart_set_baudrate(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                hal_uart_baudrate_t baudrate);
static hal_uart_error_t stm32_uart_set_rx_timeout(hal_uart_context_t* ctx,
                                                  hal_uart_instance_t instance,
                                                  uint32_t timeout);
static void stm32_uart_irq_handler(hal_uart_context_t* ctx,
                                   hal_uart_instance_t instance);
static void stm32_uart_dma_irq_handler(hal_uart_context_t* ctx,
                                       hal_uart_instance_t instance);

static bool validate_uart_instance(hal_uart_instance_t instance);
static void enable_uart_clock(hal_uart_instance_t instance);
static void enable_uart_dma_clock(hal_uart_instance_t instance);
static void configure_uart_interrupts(hal_uart_instance_t instance);
static void configure_uart_dma(hal_uart_instance_t instance,
                               const hal_uart_dma_config_t* dma_config);
static uint32_t convert_baudrate(hal_uart_baudrate_t baudrate);
static uint32_t convert_wordlength(hal_uart_wordlength_t wordlength);
static uint32_t convert_stopbits(hal_uart_stopbits_t stopbits);
static uint32_t convert_parity(hal_uart_parity_t parity);
static uint32_t convert_hwcontrol(hal_uart_hwcontrol_t hwcontrol);
static uint32_t convert_mode(hal_uart_mode_t mode);

/* UART操作函数结构体 */
static const hal_uart_ops_t stm32_uart_ops = {
    .init = stm32_uart_init,
    .deinit = stm32_uart_deinit,
    .send = stm32_uart_send,
    .receive = stm32_uart_receive,
    .send_async = stm32_uart_send_async,
    .receive_async = stm32_uart_receive_async,
    .send_dma = stm32_uart_send_dma,
    .receive_dma = stm32_uart_receive_dma,
    .receive_dma_to_idle = stm32_uart_receive_dma_to_idle,
    .abort_send = stm32_uart_abort_send,
    .abort_receive = stm32_uart_abort_receive,
    .abort_send_dma = stm32_uart_abort_send_dma,
    .abort_receive_dma = stm32_uart_abort_receive_dma,
    .get_tx_count = stm32_uart_get_tx_count,
    .get_rx_count = stm32_uart_get_rx_count,
    .get_dma_status = stm32_uart_get_dma_status,
    .set_baudrate = stm32_uart_set_baudrate,
    .set_rx_timeout = stm32_uart_set_rx_timeout,
    .irq_handler = stm32_uart_irq_handler,
    .dma_irq_handler = stm32_uart_dma_irq_handler,
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化UART上下文并设置操作函数
 * @param  ctx: UART上下文指针
 * @return HAL_UART_OK 成功，其他值为错误码
 */
hal_uart_error_t stm32_uart_init_context(hal_uart_context_t* ctx) {
  if (ctx == NULL) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  ctx->ops = &stm32_uart_ops;
  ctx->initialized = 0;
  ctx->callback = NULL;
  ctx->user_data = NULL;

  return HAL_UART_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  验证UART实例是否有效
 * @param  instance: UART实例
 * @return true 有效，false 无效
 */
static bool validate_uart_instance(hal_uart_instance_t instance) {
  if (instance >= HAL_UART_INSTANCE_LEN) return false;
  if (uart_handle_map[instance] == NULL || uart_base_map[instance] == NULL)
    return false;
  return true;
}

/**
 * @brief  启用UART时钟
 * @param  instance: UART实例
 */
static void enable_uart_clock(hal_uart_instance_t instance) {
  switch (instance) {
    case HAL_UART_INSTANCE_1:
      __HAL_RCC_USART1_CLK_ENABLE();
      break;
    case HAL_UART_INSTANCE_2:
      __HAL_RCC_USART2_CLK_ENABLE();
      break;
    case HAL_UART_INSTANCE_3:
      __HAL_RCC_USART3_CLK_ENABLE();
      break;
    case HAL_UART_INSTANCE_4:
      __HAL_RCC_UART4_CLK_ENABLE();
      break;
    default:
      break;
  }
}

/**
 * @brief  启用UART DMA时钟
 * @param  instance: UART实例
 */
static void enable_uart_dma_clock(hal_uart_instance_t instance) {
  (void)instance;
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
}

/**
 * @brief  配置UART中断
 * @param  instance: UART实例
 */
static void configure_uart_interrupts(hal_uart_instance_t instance) {
  switch (instance) {
    case HAL_UART_INSTANCE_1:
      HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(USART1_IRQn);
      break;
    case HAL_UART_INSTANCE_2:
      HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(USART2_IRQn);
      break;
    case HAL_UART_INSTANCE_3:
      HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(USART3_IRQn);
      break;
    case HAL_UART_INSTANCE_4:
      HAL_NVIC_SetPriority(UART4_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(UART4_IRQn);
      break;
    default:
      break;
  }
}

/**
 * @brief  配置UART DMA
 * @param  instance: UART实例
 * @param  dma_config: DMA配置结构体指针
 */
static void configure_uart_dma(hal_uart_instance_t instance,
                               const hal_uart_dma_config_t* dma_config) {
  if (dma_config == NULL || dma_config->dma_mode == HAL_UART_DMA_DISABLE) {
    return;
  }
  UART_HandleTypeDef* huart = uart_handle_map[instance];

  if (dma_config->dma_mode == HAL_UART_DMA_TX ||
      dma_config->dma_mode == HAL_UART_DMA_TX_RX) {
    if (uart_dma_tx_handle_map[instance] != NULL) {
      huart->hdmatx = uart_dma_tx_handle_map[instance];
      huart->hdmatx->Init.Mode = DMA_NORMAL;
      HAL_DMA_Init(huart->hdmatx);
      __HAL_DMA_ENABLE_IT(huart->hdmatx, DMA_IT_TC);
      __HAL_DMA_ENABLE_IT(huart->hdmatx, DMA_IT_TE);

      switch (instance) {
        case HAL_UART_INSTANCE_1:
          HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
          break;
        case HAL_UART_INSTANCE_2:
          HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
          break;
        case HAL_UART_INSTANCE_3:
          HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
          break;
        case HAL_UART_INSTANCE_4:
          HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
          break;
        default:
          break;
      }
    }
  }

  if (dma_config->dma_mode == HAL_UART_DMA_RX ||
      dma_config->dma_mode == HAL_UART_DMA_TX_RX) {
    if (uart_dma_rx_handle_map[instance] != NULL) {
      huart->hdmarx = uart_dma_rx_handle_map[instance];
      if (dma_config->circular_mode_rx)
        huart->hdmarx->Init.Mode = DMA_CIRCULAR;
      else
        huart->hdmarx->Init.Mode = DMA_NORMAL;
      HAL_DMA_Init(huart->hdmarx);
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_TC);
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_HT);
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_TE);

      switch (instance) {
        case HAL_UART_INSTANCE_1:
          HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
          break;
        case HAL_UART_INSTANCE_2:
          HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
          break;
        case HAL_UART_INSTANCE_3:
          HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
          break;
        case HAL_UART_INSTANCE_4:
          HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 4, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
          break;
        default:
          break;
      }
      __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    }
  }
}

/**
 * @brief  转换波特率枚举到数值
 * @param  baudrate: 波特率枚举
 * @return 波特率数值
 */
static uint32_t convert_baudrate(hal_uart_baudrate_t baudrate) {
  return (uint32_t)baudrate;
}

/**
 * @brief  转换数据位枚举到HAL定义
 * @param  wordlength: 数据位枚举
 * @return HAL数据位定义
 */
static uint32_t convert_wordlength(hal_uart_wordlength_t wordlength) {
  switch (wordlength) {
    case HAL_UART_WORDLENGTH_8B:
      return UART_WORDLENGTH_8B;
    case HAL_UART_WORDLENGTH_9B:
      return UART_WORDLENGTH_9B;
    default:
      return UART_WORDLENGTH_8B;
  }
}

/**
 * @brief  转换停止位枚举到HAL定义
 * @param  stopbits: 停止位枚举
 * @return HAL停止位定义
 */
static uint32_t convert_stopbits(hal_uart_stopbits_t stopbits) {
  switch (stopbits) {
    case HAL_UART_STOPBITS_1:
      return UART_STOPBITS_1;
    case HAL_UART_STOPBITS_2:
      return UART_STOPBITS_2;
    default:
      return UART_STOPBITS_1;
  }
}

/**
 * @brief  转换校验位枚举到HAL定义
 * @param  parity: 校验位枚举
 * @return HAL校验位定义
 */
static uint32_t convert_parity(hal_uart_parity_t parity) {
  switch (parity) {
    case HAL_UART_PARITY_NONE:
      return UART_PARITY_NONE;
    case HAL_UART_PARITY_EVEN:
      return UART_PARITY_EVEN;
    case HAL_UART_PARITY_ODD:
      return UART_PARITY_ODD;
    default:
      return UART_PARITY_NONE;
  }
}

/**
 * @brief  转换硬件流控制枚举到HAL定义
 * @param  hwcontrol: 硬件流控制枚举
 * @return HAL硬件流控制定义
 */
static uint32_t convert_hwcontrol(hal_uart_hwcontrol_t hwcontrol) {
  switch (hwcontrol) {
    case HAL_UART_HWCONTROL_NONE:
      return UART_HWCONTROL_NONE;
    case HAL_UART_HWCONTROL_RTS:
      return UART_HWCONTROL_RTS;
    case HAL_UART_HWCONTROL_CTS:
      return UART_HWCONTROL_CTS;
    case HAL_UART_HWCONTROL_RTS_CTS:
      return UART_HWCONTROL_RTS_CTS;
    default:
      return UART_HWCONTROL_NONE;
  }
}

/**
 * @brief  转换模式枚举到HAL定义
 * @param  mode: 模式枚举
 * @return HAL模式定义
 */
static uint32_t convert_mode(hal_uart_mode_t mode) {
  switch (mode) {
    case HAL_UART_MODE_TX_RX:
      return UART_MODE_TX_RX;
    case HAL_UART_MODE_TX:
      return UART_MODE_TX;
    case HAL_UART_MODE_RX:
      return UART_MODE_RX;
    default:
      return UART_MODE_TX_RX;
  }
}

/**
 * @brief  初始化UART
 * @param  ctx: UART上下文指针
 * @param  config: UART配置结构体指针
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_init(hal_uart_context_t* ctx,
                                        const hal_uart_config_t* config) {
  if (config == NULL || !validate_uart_instance(config->instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[config->instance];
  huart->Instance = uart_base_map[config->instance];

  enable_uart_clock(config->instance);

  if (config->dma_config.dma_mode != HAL_UART_DMA_DISABLE) {
    enable_uart_dma_clock(config->instance);
  }

  huart->Init.BaudRate = convert_baudrate(config->baudrate);
  huart->Init.WordLength = convert_wordlength(config->wordlength);
  huart->Init.StopBits = convert_stopbits(config->stopbits);
  huart->Init.Parity = convert_parity(config->parity);
  huart->Init.Mode = convert_mode(config->mode);
  huart->Init.HwFlowCtl = convert_hwcontrol(config->hwcontrol);
  huart->Init.OverSampling = UART_OVERSAMPLING_16;
  huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(huart) != HAL_OK) {
    return HAL_UART_ERROR_HARDWARE;
  }

  if (config->dma_config.dma_mode != HAL_UART_DMA_DISABLE) {
    configure_uart_dma(config->instance, &config->dma_config);
    dma_status[config->instance].rx_timeout =
        config->dma_config.rx_idle_timeout;
    dma_status[config->instance].last_rx_tick = HAL_GetTick();
    dma_status[config->instance].circular_mode =
        config->dma_config.circular_mode_rx;
  }

  if (config->mode == HAL_UART_MODE_TX_RX || config->mode == HAL_UART_MODE_RX) {
    configure_uart_interrupts(config->instance);
  }

  return HAL_UART_OK;
}

/**
 * @brief  反初始化 UART
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_deinit(hal_uart_context_t* ctx,
                                          hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) {
    return HAL_UART_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];

  if (dma_status[instance].tx_busy) {
    HAL_UART_DMAStop(huart);
  }
  if (dma_status[instance].rx_busy) {
    HAL_UART_DMAStop(huart);
  }

  if (HAL_UART_DeInit(huart) != HAL_OK) {
    return HAL_UART_ERROR_HARDWARE;
  }

  switch (instance) {
    case HAL_UART_INSTANCE_1:
      __HAL_RCC_USART1_CLK_DISABLE();
      break;
    case HAL_UART_INSTANCE_2:
      __HAL_RCC_USART2_CLK_DISABLE();
      break;
    case HAL_UART_INSTANCE_3:
      __HAL_RCC_USART3_CLK_DISABLE();
      break;
    case HAL_UART_INSTANCE_4:
      __HAL_RCC_UART4_CLK_DISABLE();
      break;
    default:
      break;
  }

  memset(&dma_status[instance], 0, sizeof(uart_dma_status_t));

  return HAL_UART_OK;
}

/**
 * @brief  发送数据（阻塞）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 要发送的数据指针
 * @param  size: 要发送的数据大小
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_send(hal_uart_context_t* ctx,
                                        hal_uart_instance_t instance,
                                        const uint8_t* data, uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_Transmit(huart, (uint8_t*)data, size,
                        huart->Init.BaudRate / 100) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  return HAL_UART_OK;
}

/**
 * @brief  接收数据（阻塞）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 接收数据缓冲区指针
 * @param  size: 要接收的数据大小
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_receive(hal_uart_context_t* ctx,
                                           hal_uart_instance_t instance,
                                           uint8_t* data, uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_Receive(huart, data, size, huart->Init.BaudRate / 100) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  return HAL_UART_OK;
}

/**
 * @brief  发送数据（异步）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 要发送的数据指针
 * @param  size: 要发送的数据大小
 * @param  sent_size: 输出参数，返回已发送的字节数
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_send_async(hal_uart_context_t* ctx,
                                              hal_uart_instance_t instance,
                                              const uint8_t* data,
                                              uint16_t size,
                                              uint16_t* sent_size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_Transmit_IT(huart, (uint8_t*)data, size) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  *sent_size = size;
  return HAL_UART_OK;
}

/**
 * @brief  接收数据（异步）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 接收数据缓冲区指针
 * @param  size: 要接收的数据大小
 * @param  received_size: 输出参数，返回已接收的字节数
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_receive_async(hal_uart_context_t* ctx,
                                                 hal_uart_instance_t instance,
                                                 uint8_t* data, uint16_t size,
                                                 uint16_t* received_size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_Receive_IT(huart, data, size) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  *received_size = size;
  return HAL_UART_OK;
}

/**
 * @brief  发送数据（DMA）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 要发送的数据指针
 * @param  size: 要发送的数据大小
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_send_dma(hal_uart_context_t* ctx,
                                            hal_uart_instance_t instance,
                                            const uint8_t* data,
                                            uint32_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (huart->hdmatx == NULL) return HAL_UART_ERROR_UNSUPPORTED;
  if (dma_status[instance].tx_busy) return HAL_UART_ERROR_BUSY;

  if (HAL_UART_Transmit_DMA(huart, (uint8_t*)data, size) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  dma_status[instance].tx_busy = true;
  dma_status[instance].tx_total_size = size;
  dma_status[instance].tx_current_pos = 0;

  return HAL_UART_OK;
}

/**
 * @brief  接收数据（DMA）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 接收数据缓冲区指针
 * @param  size: 要接收的数据大小
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_receive_dma(hal_uart_context_t* ctx,
                                               hal_uart_instance_t instance,
                                               uint8_t* data, uint32_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (huart->hdmarx == NULL) return HAL_UART_ERROR_UNSUPPORTED;
  if (dma_status[instance].rx_busy) return HAL_UART_ERROR_BUSY;

  if (HAL_UART_Receive_DMA(huart, data, size) != HAL_OK)
    return HAL_UART_ERROR_HARDWARE;

  dma_status[instance].rx_busy = true;
  dma_status[instance].rx_total_size = size;
  dma_status[instance].rx_current_pos = 0;
  dma_status[instance].last_rx_tick = HAL_GetTick();

  return HAL_UART_OK;
}

/**
 * @brief  接收数据（DMA，空闲中断检测）
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  data: 接收数据缓冲区指针
 * @param  size: 要接收的数据大小
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_receive_dma_to_idle(
    hal_uart_context_t* ctx, hal_uart_instance_t instance, uint8_t* data,
    uint32_t size) {
  hal_uart_error_t result = stm32_uart_receive_dma(ctx, instance, data, size);
  if (result != HAL_UART_OK) {
    return result;
  }

  UART_HandleTypeDef* huart = uart_handle_map[instance];
  __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

  return HAL_UART_OK;
}

/**
 * @brief  中止发送
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_abort_send(hal_uart_context_t* ctx,
                                              hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_AbortTransmit(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  return HAL_UART_OK;
}

/**
 * @brief  中止接收
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_abort_receive(hal_uart_context_t* ctx,
                                                 hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_AbortReceive(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  return HAL_UART_OK;
}

/**
 * @brief  中止DMA发送
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_abort_send_dma(
    hal_uart_context_t* ctx, hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_DMAStop(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  dma_status[instance].tx_busy = false;
  return HAL_UART_OK;
}

/**
 * @brief  中止DMA接收
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_abort_receive_dma(
    hal_uart_context_t* ctx, hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  if (HAL_UART_DMAStop(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  dma_status[instance].rx_busy = false;
  return HAL_UART_OK;
}

/**
 * @brief  获取发送字节数
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  count: 输出参数，返回发送字节数
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_get_tx_count(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                uint32_t* count) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  *count = huart->TxXferCount;

  return HAL_UART_OK;
}

/**
 * @brief  获取接收字节数
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  count: 输出参数，返回接收字节数
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_get_rx_count(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                uint32_t* count) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  *count = huart->RxXferCount;

  return HAL_UART_OK;
}

/**
 * @brief  获取DMA状态
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  status: 输出参数，返回DMA状态
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_get_dma_status(
    hal_uart_context_t* ctx, hal_uart_instance_t instance,
    hal_uart_dma_status_t* status) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];

  if (dma_status[instance].tx_busy) {
    if (huart->hdmatx != NULL) {
      dma_status[instance].tx_current_pos =
          dma_status[instance].tx_total_size -
          __HAL_DMA_GET_COUNTER(huart->hdmatx);
    }
  }

  if (dma_status[instance].rx_busy) {
    if (huart->hdmarx != NULL) {
      dma_status[instance].rx_current_pos =
          dma_status[instance].rx_total_size -
          __HAL_DMA_GET_COUNTER(huart->hdmarx);
    }
  }

  memcpy(status, &dma_status[instance], sizeof(hal_uart_dma_status_t));

  return HAL_UART_OK;
}

/**
 * @brief  设置波特率
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  baudrate: 波特率
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_set_baudrate(hal_uart_context_t* ctx,
                                                hal_uart_instance_t instance,
                                                hal_uart_baudrate_t baudrate) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];

  if (HAL_UART_DeInit(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  huart->Init.BaudRate = convert_baudrate(baudrate);

  if (HAL_UART_Init(huart) != HAL_OK) return HAL_UART_ERROR_HARDWARE;

  return HAL_UART_OK;
}

/**
 * @brief  设置接收超时
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 * @param  timeout: 超时时间(ms)
 * @return HAL_UART_OK 成功，其他值为错误码
 */
static hal_uart_error_t stm32_uart_set_rx_timeout(hal_uart_context_t* ctx,
                                                  hal_uart_instance_t instance,
                                                  uint32_t timeout) {
  if (!validate_uart_instance(instance)) return HAL_UART_ERROR_INVALID_PARAM;

  (void)ctx;
  dma_status[instance].rx_timeout = timeout;

  return HAL_UART_OK;
}

/**
 * @brief  UART中断处理
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 */
static void stm32_uart_irq_handler(hal_uart_context_t* ctx,
                                   hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return;

  (void)ctx;
  UART_HandleTypeDef* huart = uart_handle_map[instance];
  HAL_UART_IRQHandler(huart);
}

/**
 * @brief  DMA中断处理
 * @param  ctx: UART上下文指针
 * @param  instance: UART实例
 */
static void stm32_uart_dma_irq_handler(hal_uart_context_t* ctx,
                                       hal_uart_instance_t instance) {
  if (!validate_uart_instance(instance)) return;

  (void)ctx;
  const UART_HandleTypeDef* huart = uart_handle_map[instance];

  if (huart->hdmatx != NULL) HAL_DMA_IRQHandler(huart->hdmatx);

  if (huart->hdmarx != NULL) HAL_DMA_IRQHandler(huart->hdmarx);
}

/* HAL回调函数实现 */

/**
 * @brief  UART错误回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].tx_busy = false;
      dma_status[i].rx_busy = false;
      break;
    }
  }
}

/**
 * @brief  UART发送半完成回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      if (dma_status[i].circular_mode) {
        uint32_t half_pos = dma_status[i].tx_total_size / 2;
        (void)half_pos;
      }
      break;
    }
  }
}

/**
 * @brief  UART接收半完成回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].rx_half_transfers++;

      if (dma_status[i].circular_mode) {
        uint32_t half_pos = dma_status[i].rx_total_size / 2;
        (void)half_pos;
      }
      break;
    }
  }
}

/**
 * @brief  UART发送完成回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].tx_busy = false;
      dma_status[i].tx_current_pos = 0;
      __HAL_UART_DISABLE_IT(huart, UART_IT_TC);
      break;
    }
  }
}

/**
 * @brief  UART接收完成回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].rx_full_transfers++;
      if (!dma_status[i].circular_mode)
        dma_status[i].rx_busy = false;
      else
        dma_status[i].rx_current_pos = 0;
      break;
    }
  }
}

/**
 * @brief  UART空闲回调
 * @param  huart: UART句柄指针
 */
void HAL_UART_IdleCallback(UART_HandleTypeDef* huart) {
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      __HAL_UART_CLEAR_IDLEFLAG(huart);

      const uint32_t received_size =
          dma_status[i].rx_total_size - __HAL_DMA_GET_COUNTER(huart->hdmarx);

      if (received_size > 0) {
        if (!dma_status[i].circular_mode) {
          HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr,
                               dma_status[i].rx_total_size);
        }
      }
      break;
    }
  }
}
