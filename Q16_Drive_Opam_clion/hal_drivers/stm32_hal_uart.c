// stm32_hal_uart.c
#include "hal_uart.h"
#include "memory_pool/memory_pool.h"
#include "stm32g4xx_hal.h"
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
static UART_HandleTypeDef *uart_handle_map[HAL_UART_INSTANCE_LEN] = {
    &huart1,  // HAL_UART_INSTANCE_1
    NULL,     // HAL_UART_INSTANCE_2
    NULL,     // HAL_UART_INSTANCE_3
    NULL,     // HAL_UART_INSTANCE_4
};

/* UART实例基地址映射 */
static USART_TypeDef *uart_base_map[HAL_UART_INSTANCE_LEN] = {
    USART1,
    USART2,
    USART3,
    UART4,
};

/* DMA句柄映射表 */
static DMA_HandleTypeDef *uart_dma_tx_handle_map[HAL_UART_INSTANCE_LEN] = {
    &hdma_usart1_tx,
    NULL,
    NULL,
    NULL,
};

static DMA_HandleTypeDef *uart_dma_rx_handle_map[HAL_UART_INSTANCE_LEN] = {
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
  bool circular_mode;         /**< 是否循环模式 */
  uint32_t rx_half_transfers; /**< 半传输次数（用于循环模式） */
  uint32_t rx_full_transfers; /**< 全传输次数（用于循环模式） */
} uart_dma_status_t;

uart_dma_status_t dma_status[HAL_UART_INSTANCE_LEN] = {0};

static const hal_uart_ops_t stm32_uart_ops;

/* Private function prototypes */
static bool stm32_uart_init(const hal_uart_config_t *config);
static bool stm32_uart_deinit(hal_uart_instance_e instance);
static bool stm32_uart_send(hal_uart_instance_e instance, const uint8_t *data,
                            uint16_t size);
static bool stm32_uart_receive(hal_uart_instance_e instance, uint8_t *data,
                               uint16_t size);
static uint16_t stm32_uart_send_async(hal_uart_instance_e instance,
                                      const uint8_t *data, uint16_t size);
static uint16_t stm32_uart_receive_async(hal_uart_instance_e instance,
                                         uint8_t *data, uint16_t size);
static bool stm32_uart_send_dma(hal_uart_instance_e instance,
                                const uint8_t *data, uint32_t size);
static bool stm32_uart_receive_dma(hal_uart_instance_e instance, uint8_t *data,
                                   uint32_t size);
static bool stm32_uart_receive_dma_to_idle(hal_uart_instance_e instance,
                                           uint8_t *data, uint32_t size);
static bool stm32_uart_abort_send(hal_uart_instance_e instance);
static bool stm32_uart_abort_receive(hal_uart_instance_e instance);
static bool stm32_uart_abort_send_dma(hal_uart_instance_e instance);
static bool stm32_uart_abort_receive_dma(hal_uart_instance_e instance);
static uint32_t stm32_uart_get_tx_count(hal_uart_instance_e instance);
static uint32_t stm32_uart_get_rx_count(hal_uart_instance_e instance);
static hal_uart_dma_status_t stm32_uart_get_dma_status(
    hal_uart_instance_e instance);
static bool stm32_uart_set_baudrate(hal_uart_instance_e instance,
                                    hal_uart_baudrate_e baudrate);
static bool stm32_uart_set_rx_timeout(hal_uart_instance_e instance,
                                      uint32_t timeout);
static void stm32_uart_irq_handler(hal_uart_instance_e instance);
static void stm32_uart_dma_irq_handler(hal_uart_instance_e instance);

static bool validate_uart_instance(hal_uart_instance_e instance);
static void enable_uart_clock(hal_uart_instance_e instance);
static void enable_uart_dma_clock(hal_uart_instance_e instance);
static void configure_uart_interrupts(hal_uart_instance_e instance);
static void configure_uart_dma(hal_uart_instance_e instance,
                               const hal_uart_dma_config_t *dma_config);
static uint32_t convert_baudrate(hal_uart_baudrate_e baudrate);
static uint32_t convert_wordlength(hal_uart_wordlength_e wordlength);
static uint32_t convert_stopbits(hal_uart_stopbits_e stopbits);
static uint32_t convert_parity(hal_uart_parity_e parity);
static uint32_t convert_hwcontrol(hal_uart_hwcontrol_e hwcontrol);
static uint32_t convert_mode(hal_uart_mode_e mode);

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
    .dma_irq_handler = stm32_uart_dma_irq_handler};

/**
 * @brief  验证UART实例是否有效
 */
static bool validate_uart_instance(const hal_uart_instance_e instance) {
  if (instance >= HAL_UART_INSTANCE_LEN) return false;
  if (uart_handle_map[instance] == NULL || uart_base_map[instance] == NULL)
    return false;
  return true;
}

/**
 * @brief  启用UART时钟
 */
static void enable_uart_clock(const hal_uart_instance_e instance) {
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
 */
static void enable_uart_dma_clock(hal_uart_instance_e instance) {
  // STM32G431的DMA时钟已经默认开启
  // 这里可以根据需要启用特定的DMA时钟
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
}

/**
 * @brief  配置UART中断
 */
static void configure_uart_interrupts(const hal_uart_instance_e instance) {
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
 */
static void configure_uart_dma(const hal_uart_instance_e instance,
                               const hal_uart_dma_config_t *dma_config) {
  if (dma_config == NULL || dma_config->dma_mode == HAL_UART_DMA_DISABLE) {
    return;
  }
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  // 配置发送DMA
  if (dma_config->dma_mode == HAL_UART_DMA_TX ||
      dma_config->dma_mode == HAL_UART_DMA_TX_RX) {
    if (uart_dma_tx_handle_map[instance] != NULL) {
      huart->hdmatx = uart_dma_tx_handle_map[instance];
      // 设置DMA模式（普通模式或循环模式）
      huart->hdmatx->Init.Mode = DMA_NORMAL;
      // 重新初始化DMA以应用模式更改
      HAL_DMA_Init(huart->hdmatx);
      // 启用DMA发送中断
      __HAL_DMA_ENABLE_IT(huart->hdmatx, DMA_IT_TC);
      __HAL_DMA_ENABLE_IT(huart->hdmatx, DMA_IT_TE);
      // 配置DMA中断
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
  // 配置接收DMA
  if (dma_config->dma_mode == HAL_UART_DMA_RX ||
      dma_config->dma_mode == HAL_UART_DMA_TX_RX) {
    if (uart_dma_rx_handle_map[instance] != NULL) {
      huart->hdmarx = uart_dma_rx_handle_map[instance];
      // 设置DMA模式（普通模式或循环模式）
      if (dma_config->circular_mode_rx)
        huart->hdmarx->Init.Mode = DMA_CIRCULAR;
      else
        huart->hdmarx->Init.Mode = DMA_NORMAL;
      // 重新初始化DMA以应用模式更改
      HAL_DMA_Init(huart->hdmarx);
      // 启用DMA接收中断
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_TC);
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_HT);  // 半传输中断
      __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_TE);
      // 配置DMA中断
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
      // 启用UART空闲中断（用于DMA接收空闲检测）
      __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
    }
  }
}

/**
 * @brief  转换波特率枚举到数值
 */
static uint32_t convert_baudrate(const hal_uart_baudrate_e baudrate) {
  return (uint32_t)baudrate;
}

/**
 * @brief  转换数据位枚举到HAL定义
 */
static uint32_t convert_wordlength(const hal_uart_wordlength_e wordlength) {
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
 */
static uint32_t convert_stopbits(hal_uart_stopbits_e stopbits) {
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
 */
static uint32_t convert_parity(const hal_uart_parity_e parity) {
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
 */
static uint32_t convert_hwcontrol(hal_uart_hwcontrol_e hwcontrol) {
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
 */
static uint32_t convert_mode(const hal_uart_mode_e mode) {
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

void platform_uart_init(void) {
  /* 设置UART操作函数 */
  hal_uart_set_ops(&stm32_uart_ops);
}

static bool stm32_uart_init(const hal_uart_config_t *config) {
  if (config == NULL || !validate_uart_instance(config->instance)) {
    return false;
  }
  UART_HandleTypeDef *huart = uart_handle_map[config->instance];
  huart->Instance = uart_base_map[config->instance];
  /* 启用UART时钟 */
  enable_uart_clock(config->instance);
  /* 启用DMA时钟（如果需要） */
  if (config->dma_config.dma_mode != HAL_UART_DMA_DISABLE) {
    enable_uart_dma_clock(config->instance);
  }
  /* 配置UART参数 */
  huart->Init.BaudRate = convert_baudrate(config->baudrate);
  huart->Init.WordLength = convert_wordlength(config->wordlength);
  huart->Init.StopBits = convert_stopbits(config->stopbits);
  huart->Init.Parity = convert_parity(config->parity);
  huart->Init.Mode = convert_mode(config->mode);
  huart->Init.HwFlowCtl = convert_hwcontrol(config->hwcontrol);
  huart->Init.OverSampling = UART_OVERSAMPLING_16;
  huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  /* 高级特性初始化 */
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(huart) != HAL_OK) {
    return false;
  }
  /* 配置DMA（如果需要） */
  if (config->dma_config.dma_mode != HAL_UART_DMA_DISABLE) {
    configure_uart_dma(config->instance, &config->dma_config);
    // 初始化DMA状态
    // 初始化DMA状态
    dma_status[config->instance].rx_timeout =
        config->dma_config.rx_idle_timeout;
    dma_status[config->instance].last_rx_tick = HAL_GetTick();
    dma_status[config->instance].circular_mode =
        config->dma_config.circular_mode_rx;
  }
  /* 如果启用接收，配置中断 */
  if (config->mode == HAL_UART_MODE_TX_RX || config->mode == HAL_UART_MODE_RX) {
    configure_uart_interrupts(config->instance);
    /* 如果不是DMA模式，启动中断接收 */
    if (config->dma_config.dma_mode == HAL_UART_DMA_DISABLE) {
      if (HAL_UART_Receive_IT(huart, huart->pRxBuffPtr, 1) != HAL_OK) {
        return false;
      }
    }
  }
  return true;
}

static bool stm32_uart_deinit(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) {
    return false;
  }
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  /* 中止DMA传输 */
  if (dma_status[instance].tx_busy) {
    HAL_UART_DMAStop(huart);
  }
  if (dma_status[instance].rx_busy) {
    HAL_UART_DMAStop(huart);
  }
  if (HAL_UART_DeInit(huart) != HAL_OK) {
    return false;
  }
  /* 禁用UART时钟 */
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
  /* 重置DMA状态 */
  __memset(&dma_status[instance], 0, sizeof(uart_dma_status_t));
  return true;
}

static bool stm32_uart_send(const hal_uart_instance_e instance,
                            const uint8_t *data, const uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_Transmit(huart, (uint8_t *)data, size,
                        huart->Init.BaudRate / 100) != HAL_OK)
    return false;
  return true;
}

static bool stm32_uart_receive(const hal_uart_instance_e instance,
                               uint8_t *data, const uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_Receive(huart, data, size, huart->Init.BaudRate / 100) != HAL_OK)
    return false;
  return true;
}

static uint16_t stm32_uart_send_async(const hal_uart_instance_e instance,
                                      const uint8_t *data,
                                      const uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance)) return 0;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_Transmit_IT(huart, (uint8_t *)data, size) != HAL_OK) return 0;
  return size;
}

static uint16_t stm32_uart_receive_async(const hal_uart_instance_e instance,
                                         uint8_t *data, const uint16_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance)) return 0;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_Receive_IT(huart, data, size) != HAL_OK) return 0;
  return size;
}

static bool stm32_uart_send_dma(const hal_uart_instance_e instance,
                                const uint8_t *data, const uint32_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (huart->hdmatx == NULL) return false;         // DMA未配置
  if (dma_status[instance].tx_busy) return false;  // 发送忙
  if (HAL_UART_Transmit_DMA(huart, (uint8_t *)data, size) != HAL_OK)
    return false;
  // 更新DMA状态
  dma_status[instance].tx_busy = true;
  dma_status[instance].tx_total_size = size;
  dma_status[instance].tx_current_pos = 0;
  return true;
}

static bool stm32_uart_receive_dma(const hal_uart_instance_e instance,
                                   uint8_t *data, const uint32_t size) {
  if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (huart->hdmarx == NULL) return false;         // DMA未配置
  if (dma_status[instance].rx_busy) return false;  // 接收忙
  if (HAL_UART_Receive_DMA(huart, data, size) != HAL_OK) return false;
  // 更新DMA状态
  dma_status[instance].rx_busy = true;
  dma_status[instance].rx_total_size = size;
  dma_status[instance].rx_current_pos = 0;
  dma_status[instance].last_rx_tick = HAL_GetTick();
  return true;
}

static bool stm32_uart_receive_dma_to_idle(const hal_uart_instance_e instance,
                                           uint8_t *data, const uint32_t size) {
  // 使用DMA接收，配合空闲中断检测帧结束
  if (!stm32_uart_receive_dma(instance, data, size)) return false;
  // 启用空闲中断
  const UART_HandleTypeDef *huart = uart_handle_map[instance];
  __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
  return true;
}

static bool stm32_uart_abort_send(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_AbortTransmit(huart) != HAL_OK) return false;
  return true;
}

static bool stm32_uart_abort_receive(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_AbortReceive(huart) != HAL_OK) return false;
  return true;
}

static bool stm32_uart_abort_send_dma(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_DMAStop(huart) != HAL_OK) return false;
  // 更新DMA状态
  dma_status[instance].tx_busy = false;
  return true;
}

static bool stm32_uart_abort_receive_dma(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  if (HAL_UART_DMAStop(huart) != HAL_OK) return false;
  // 更新DMA状态
  dma_status[instance].rx_busy = false;
  return true;
}

static uint32_t stm32_uart_get_tx_count(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return 0;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  return huart->TxXferCount;
}

static uint32_t stm32_uart_get_rx_count(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return 0;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  return huart->RxXferCount;
}

static hal_uart_dma_status_t stm32_uart_get_dma_status(
    const hal_uart_instance_e instance) {
  hal_uart_dma_status_t status = {0};
  if (!validate_uart_instance(instance)) return status;
  // 计算DMA传输进度
  if (dma_status[instance].tx_busy) {
    const UART_HandleTypeDef *huart = uart_handle_map[instance];
    if (huart->hdmatx != NULL) {
      dma_status[instance].tx_current_pos =
          dma_status[instance].tx_total_size -
          __HAL_DMA_GET_COUNTER(huart->hdmatx);
    }
  }
  if (dma_status[instance].rx_busy) {
    const UART_HandleTypeDef *huart = uart_handle_map[instance];
    if (huart->hdmarx != NULL) {
      dma_status[instance].rx_current_pos =
          dma_status[instance].rx_total_size -
          __HAL_DMA_GET_COUNTER(huart->hdmarx);
    }
  }
  // 转换为通用状态结构
  __memcpy(&status, &dma_status[instance], sizeof(hal_uart_dma_status_t));
  return status;
}

static bool stm32_uart_set_baudrate(const hal_uart_instance_e instance,
                                    const hal_uart_baudrate_e baudrate) {
  if (!validate_uart_instance(instance)) return false;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  /* 先停止UART */
  if (HAL_UART_DeInit(huart) != HAL_OK) return false;
  /* 更新波特率 */
  huart->Init.BaudRate = convert_baudrate(baudrate);
  /* 重新初始化UART */
  if (HAL_UART_Init(huart) != HAL_OK) return false;
  return true;
}

static bool stm32_uart_set_rx_timeout(const hal_uart_instance_e instance,
                                      const uint32_t timeout) {
  if (!validate_uart_instance(instance)) return false;
  dma_status[instance].rx_timeout = timeout;
  return true;
}

static void stm32_uart_irq_handler(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return;
  UART_HandleTypeDef *huart = uart_handle_map[instance];
  HAL_UART_IRQHandler(huart);
}

static void stm32_uart_dma_irq_handler(const hal_uart_instance_e instance) {
  if (!validate_uart_instance(instance)) return;
  const UART_HandleTypeDef *huart = uart_handle_map[instance];
  // 处理发送DMA中断
  if (huart->hdmatx != NULL) HAL_DMA_IRQHandler(huart->hdmatx);
  // 处理接收DMA中断
  if (huart->hdmarx != NULL) HAL_DMA_IRQHandler(huart->hdmarx);
}

//
// /* UART中断处理函数 */
// void USART1_IRQHandler(void)
// {
//     hal_uart_irq_handler(HAL_UART_INSTANCE_1);
// }

// void USART2_IRQHandler(void)
// {
//     hal_uart_irq_handler(HAL_UART_INSTANCE_2);
// }
//
// void USART3_IRQHandler(void)
// {
//     hal_uart_irq_handler(HAL_UART_INSTANCE_3);
// }
//
// void UART4_IRQHandler(void)
// {
//     hal_uart_irq_handler(HAL_UART_INSTANCE_4);
// }
//
// /* DMA中断处理函数 */
// void DMA1_Channel1_IRQHandler(void)
// {
//     // USART1 TX DMA
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_1);
// }

// void DMA1_Channel2_IRQHandler(void)
// {
//     // USART1 RX DMA / USART2 TX DMA
//     // 需要根据具体配置确定是哪个UART
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_1);
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_2);
// }

// void DMA1_Channel3_IRQHandler(void)
// {
//     // USART2 RX DMA / USART3 TX DMA
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_2);
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_3);
// }
//
// void DMA1_Channel4_IRQHandler(void)
// {
//     // USART3 RX DMA / UART4 TX DMA
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_3);
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_4);
// }
//
// void DMA1_Channel5_IRQHandler(void)
// {
//     // UART4 RX DMA
//     hal_uart_dma_irq_handler(HAL_UART_INSTANCE_4);
// }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  // 查找对应的UART实例
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].tx_busy = false;
      dma_status[i].rx_busy = false;
      // 用户可以在这里添加错误处理
      break;
    }
  }
}

/* DMA传输完成回调函数 */
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart) {
  // 半传输完成回调（主要用于循环模式）
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      if (dma_status[i].circular_mode) {
        // 循环模式下的半传输完成
        // 用户可以在这里处理前半部分数据
        uint32_t half_pos = dma_status[i].tx_total_size / 2;
        // user_half_tx_callback(i, half_pos);
      }
      break;
    }
  }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  // 接收半传输完成回调（主要用于循环模式）
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].rx_half_transfers++;

      if (dma_status[i].circular_mode) {
        // 循环模式下的半传输完成
        uint32_t half_pos = dma_status[i].rx_total_size / 2;
        // 用户可以在这里处理前半部分数据
        // user_half_rx_callback(i, half_pos);
      }
      break;
    }
  }
}

/**
 * @brief UART传输完成回调函数
 *
 * 该函数在UART传输完成后被调用。它会根据当前的UART实例更新其状态。
 * 如果是普通模式，传输完成后将标记为不忙；如果是循环模式，则传输完成后保持忙状态，并重置位置。
 * 同时，关闭传输完成中断以避免反复进入回调。
 *
 * @param huart 指向UART句柄的指针
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  // 查找对应的UART实例
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      // 普通模式：传输完成，标记为不忙
      dma_status[i].tx_busy = false;
      // 循环模式：传输完成但保持忙状态
      dma_status[i].tx_current_pos = 0;          // 重置位置
      __HAL_UART_DISABLE_IT(huart, UART_IT_TC);  // 关闭，避免反复进
      // 用户可以在这里添加自定义处理
      break;
    }
  }
}

/**
 * @brief UART接收完成回调函数
 *
 * 该函数在UART DMA接收完成后被调用，用于更新相关状态。
 * 根据是否处于循环模式，更新对应的DMA状态信息。
 *
 * @param huart 指向UART句柄的指针
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  // 查找对应的UART实例
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      dma_status[i].rx_full_transfers++;
      if (!dma_status[i].circular_mode)
        dma_status[i].rx_busy = false;  // 普通模式：传输完成，标记为不忙
      else
        dma_status[i].rx_current_pos = 0;  // 循环模式：传输完成但保持忙状态
      // 用户可以在这里添加自定义处理
      break;
    }
  }
}

/* UART空闲中断回调（用于DMA接收空闲检测） */
void HAL_UART_IdleCallback(UART_HandleTypeDef *huart) {
  // 查找对应的UART实例
  for (int i = 0; i < HAL_UART_INSTANCE_LEN; i++) {
    if (uart_handle_map[i] == huart) {
      // 清除空闲中断标志
      __HAL_UART_CLEAR_IDLEFLAG(huart);
      // 计算当前接收的数据长度
      const uint32_t received_size =
          dma_status[i].rx_total_size - __HAL_DMA_GET_COUNTER(huart->hdmarx);
      if (received_size > 0) {
        // 用户可以在这里处理接收到的数据
        // user_idle_callback(i, received_size);
        if (!dma_status[i]
                 .circular_mode)  // 循环模式：自动继续接收，无需重新启动
        {
          // 普通模式：重新启动接收
          HAL_UART_Receive_DMA(huart, huart->pRxBuffPtr,
                               dma_status[i].rx_total_size);
        }
      }
      break;
    }
  }
}
