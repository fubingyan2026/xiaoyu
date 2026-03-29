//
// Created by fubingyan on 25-9-21.
//
/// stm32_hal_fdcan.c
#include "fdcan.h"
#include "hal_fdcan.h"
#include "main.h"

/* 多FDCAN实例句柄 */
extern FDCAN_HandleTypeDef hfdcan1;
#if defined(FDCAN2)
extern FDCAN_HandleTypeDef hfdcan2;
#endif
#if defined(FDCAN3)
extern FDCAN_HandleTypeDef hfdcan3;
#endif

/* FDCAN实例映射表 */
static FDCAN_HandleTypeDef *fdcan_handle_map[HAL_FDCAN_INSTANCE_LEN] = {
    &hfdcan1,  // HAL_FDCAN_INSTANCE_1
#if defined(FDCAN2)
    &hfdcan2,  // HAL_FDCAN_INSTANCE_2
#else
    NULL,
#endif
#if defined(FDCAN3)
    &hfdcan3,  // HAL_FDCAN_INSTANCE_3
#else
    NULL,
#endif
};

/* FDCAN实例基地址映射 */
static FDCAN_GlobalTypeDef *fdcan_base_map[HAL_FDCAN_INSTANCE_LEN] = {
    FDCAN1,
#if defined(FDCAN2)
    FDCAN2,
#else
    NULL,
#endif
#if defined(FDCAN3)
    FDCAN3,
#else
    NULL,
#endif
};

static const hal_fdcan_ops_t stm32_fdcan_ops;

/* Private function prototypes */
static hal_fdcan_error_t stm32_fdcan_init(hal_fdcan_context_t *ctx,
                                            const hal_fdcan_config_t *config);
static hal_fdcan_error_t stm32_fdcan_deinit(hal_fdcan_context_t *ctx,
                                              hal_fdcan_instance_t instance);
static hal_fdcan_error_t stm32_fdcan_send(hal_fdcan_context_t *ctx,
                                            hal_fdcan_instance_t instance,
                                            const hal_fdcan_message_t *message);
static hal_fdcan_error_t stm32_fdcan_receive(hal_fdcan_context_t *ctx,
                                               hal_fdcan_instance_t instance,
                                               hal_fdcan_message_t *message);
static hal_fdcan_error_t stm32_fdcan_receive_level(hal_fdcan_context_t *ctx,
                                                    hal_fdcan_instance_t instance,
                                                    uint8_t fifo_address,
                                                    uint32_t *level);
static hal_fdcan_error_t stm32_fdcan_get_send_level(hal_fdcan_context_t *ctx,
                                                     hal_fdcan_instance_t instance,
                                                     uint32_t *level);
static hal_fdcan_error_t stm32_fdcan_set_filter(hal_fdcan_context_t *ctx,
                                                  hal_fdcan_instance_t instance,
                                                  const hal_fdcan_filter_t *filter);
static hal_fdcan_error_t stm32_fdcan_set_mode(hal_fdcan_context_t *ctx,
                                                hal_fdcan_instance_t instance,
                                                hal_fdcan_mode_t mode);
static hal_fdcan_error_t stm32_fdcan_get_error_count(hal_fdcan_context_t *ctx,
                                                       hal_fdcan_instance_t instance,
                                                       uint32_t *error_count);
static void stm32_fdcan_irq_handler(hal_fdcan_context_t *ctx, hal_fdcan_instance_t instance);
static uint8_t round_up_special(uint8_t x);
static uint8_t get_can_rx_length(uint32_t rx_DL);
static bool validate_fdcan_instance(hal_fdcan_instance_t instance);
static void enable_fdcan_clock(hal_fdcan_instance_t instance);
static void configure_fdcan_interrupts(hal_fdcan_instance_t instance);

/* FDCAN操作函数结构体 */
static const hal_fdcan_ops_t stm32_fdcan_ops = {
    .init = stm32_fdcan_init,
    .deinit = stm32_fdcan_deinit,
    .send = stm32_fdcan_send,
    .receive = stm32_fdcan_receive,
    .get_send_level = stm32_fdcan_get_send_level,
    .receive_level = stm32_fdcan_receive_level,
    .set_filter = stm32_fdcan_set_filter,
    .set_mode = stm32_fdcan_set_mode,
    .get_error_count = stm32_fdcan_get_error_count,
    .irq_handler = stm32_fdcan_irq_handler};

/**
 * @brief STM32 平台 FDCAN 上下文初始化函数
 * @param ctx FDCAN 上下文指针
 * @return 操作结果错误码
 */
hal_fdcan_error_t stm32_fdcan_init_context(hal_fdcan_context_t *ctx)
{
  if (ctx == NULL) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  ctx->ops = &stm32_fdcan_ops;
  ctx->initialized = 0;

  return HAL_FDCAN_OK;
}

/**
 * @brief  验证FDCAN实例是否有效
 */
static bool validate_fdcan_instance(hal_fdcan_instance_t instance) {
  if (instance >= HAL_FDCAN_INSTANCE_LEN) {
    return false;
  }
  if (fdcan_handle_map[instance] == NULL || fdcan_base_map[instance] == NULL) {
    return false;
  }
  return true;
}

/**
 * @brief  启用FDCAN时钟
 */
static void enable_fdcan_clock(hal_fdcan_instance_t instance) {
  switch (instance) {
    case HAL_FDCAN_INSTANCE_1:
      __HAL_RCC_FDCAN_CLK_ENABLE();
      break;
#if defined(FDCAN2)
    case HAL_FDCAN_INSTANCE_2:
      __HAL_RCC_FDCAN_CLK_ENABLE();  // FDCAN1和FDCAN2共享时钟
      break;
#endif
#if defined(FDCAN3)
    case HAL_FDCAN_INSTANCE_3:
      __HAL_RCC_FDCAN_CLK_ENABLE();
      break;
#endif
    default:
      break;
  }
}

/**
 * @brief  配置FDCAN中断优先级和使能
 * @param instance FDCAN实例
 * @return 操作结果错误码
 */
static void configure_fdcan_interrupts(const hal_fdcan_instance_t instance) {
  switch (instance) {
    case HAL_FDCAN_INSTANCE_1:
      HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
      HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
      break;
#if defined(FDCAN2)
    case HAL_FDCAN_INSTANCE_2:
      HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
      HAL_NVIC_SetPriority(FDCAN2_IT1_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN2_IT1_IRQn);
      break;
#endif
#if defined(FDCAN3)
    case HAL_FDCAN_INSTANCE_3:
      HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
      HAL_NVIC_SetPriority(FDCAN3_IT1_IRQn, 3, 0);
      HAL_NVIC_EnableIRQ(FDCAN3_IT1_IRQn);
      break;
#endif
    default:
      break;
  }
}

/**
 * @brief  初始化FDCAN
 * @param ctx FDCAN 上下文指针
 * @param config FDCAN配置结构体指针
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_init(hal_fdcan_context_t *ctx,
                                            const hal_fdcan_config_t *config) {
  (void)ctx;

  if (config == NULL || !validate_fdcan_instance(config->instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[config->instance];
  hfdcan->Instance = fdcan_base_map[config->instance];

  /* 启用FDCAN时钟 */
  enable_fdcan_clock(config->instance);

  /* 配置FDCAN */
  hfdcan->Init.FrameFormat = FDCAN_FRAME_FD_BRS;
  hfdcan->Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan->Init.AutoRetransmission = DISABLE;
  hfdcan->Init.TransmitPause = DISABLE;
  hfdcan->Init.ProtocolException = DISABLE;

  /* 配置时钟分频器 */
  hfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan->Init.NominalPrescaler = config->baudrate.prescaler;
  hfdcan->Init.NominalSyncJumpWidth = config->baudrate.sync_jump_width;
  hfdcan->Init.NominalTimeSeg1 = config->baudrate.time_seg1;
  hfdcan->Init.NominalTimeSeg2 = config->baudrate.time_seg2;

  /* 配置数据波特率 */
  hfdcan->Init.DataPrescaler = config->baudrate.prescaler;
  hfdcan->Init.DataSyncJumpWidth = config->baudrate.sync_jump_width;
  hfdcan->Init.DataTimeSeg1 = config->baudrate.time_seg1;
  hfdcan->Init.DataTimeSeg2 = config->baudrate.time_seg2;

  /* 配置FIFO */
  hfdcan->Init.StdFiltersNbr = config->filter_count;
  hfdcan->Init.ExtFiltersNbr = config->filter_count;

  hfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

  /* 根据配置设置模式 */
  switch (config->mode) {
    case HAL_FDCAN_MODE_NORMAL:
      hfdcan->Init.Mode = FDCAN_MODE_NORMAL;
      break;
    case HAL_FDCAN_MODE_LOOPBACK:
      hfdcan->Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
      break;
    case HAL_FDCAN_MODE_SILENT:
      hfdcan->Init.Mode = FDCAN_MODE_BUS_MONITORING;
      break;
    case HAL_FDCAN_MODE_SILENT_LOOPBACK:
      hfdcan->Init.Mode =
          FDCAN_MODE_INTERNAL_LOOPBACK | FDCAN_MODE_BUS_MONITORING;
      break;
    default:
      hfdcan->Init.Mode = FDCAN_MODE_NORMAL;
      break;
  }

  if (HAL_FDCAN_Init(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  /* 配置过滤器 */
  for (uint8_t i = 0; i < config->filter_count; i++) {
    FDCAN_FilterTypeDef bsp_filter = {
        .IdType = config->filters[i].is_extended ? FDCAN_EXTENDED_ID
                                                 : FDCAN_STANDARD_ID,
        .FilterIndex = config->filters[i].filter_index,
        .FilterType = FDCAN_FILTER_MASK,
        .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
        .FilterID1 = config->filters[i].id,
        .FilterID2 = config->filters[i].mask};

    if (HAL_FDCAN_ConfigFilter(hfdcan, &bsp_filter) != HAL_OK) {
      return HAL_FDCAN_ERROR_HARDWARE;
    }
  }

  /* 启动FDCAN */
  if (HAL_FDCAN_Start(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  /* 激活通知 */
  HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

  /* 配置中断 */
  configure_fdcan_interrupts(config->instance);

  return HAL_FDCAN_OK;
}

/**
 * @brief  反初始化FDCAN
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_deinit(hal_fdcan_context_t *ctx,
                                              hal_fdcan_instance_t instance) {
  (void)ctx;

  if (!validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];

  if (HAL_FDCAN_Stop(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  if (HAL_FDCAN_DeInit(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  return HAL_FDCAN_OK;
}

/**
 * @brief   把输入规整到最近的更大或相等的标准值
 * @param x 输入值
 * @return 规整后的值
 */
static uint8_t round_up_special(const uint8_t x) {
  static const uint8_t tbl[] = {8, 12, 16, 20, 24, 32, 48, 64};
  const size_t n = sizeof(tbl) / sizeof(tbl[0]);
  if (x < 8) return x;
  for (size_t i = 0; i < n; ++i)
    if (x <= tbl[i]) return tbl[i];
  return 64;
}

/**
 * @brief   发送FDCAN消息
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 要发送的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_send(hal_fdcan_context_t *ctx,
                                            hal_fdcan_instance_t instance,
                                            const hal_fdcan_message_t *message) {
  (void)ctx;

  if (message == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  const uint8_t tx_data_len = round_up_special(message->data_length);
  uint32_t len_code;

  switch (tx_data_len) {
    case 12:
      len_code = FDCAN_DLC_BYTES_12;
      break;
    case 16:
      len_code = FDCAN_DLC_BYTES_16;
      break;
    case 20:
      len_code = FDCAN_DLC_BYTES_20;
      break;
    case 24:
      len_code = FDCAN_DLC_BYTES_24;
      break;
    case 32:
      len_code = FDCAN_DLC_BYTES_32;
      break;
    case 48:
      len_code = FDCAN_DLC_BYTES_48;
      break;
    case 64:
      len_code = FDCAN_DLC_BYTES_64;
      break;
    default:
      len_code = tx_data_len;
      break;
  }

  FDCAN_TxHeaderTypeDef tx_header;
  tx_header.Identifier = message->id;
  tx_header.IdType =
      message->is_extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
  tx_header.TxFrameType =
      message->is_remote ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
  tx_header.DataLength = len_code;
  tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  tx_header.BitRateSwitch = FDCAN_BRS_ON;
  tx_header.FDFormat = len_code > 8 ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
  tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  tx_header.MessageMarker = 0;

  if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx_header, message->data) !=
      HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  return HAL_FDCAN_OK;
}

/**
 * @brief   接收FDCAN消息
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static uint8_t get_can_rx_length(const uint32_t rx_DL) {
  switch (rx_DL) {
    case FDCAN_DLC_BYTES_12:
      return 12;
    case FDCAN_DLC_BYTES_16:
      return 16;
    case FDCAN_DLC_BYTES_20:
      return 20;
    case FDCAN_DLC_BYTES_24:
      return 24;
    case FDCAN_DLC_BYTES_32:
      return 32;
    case FDCAN_DLC_BYTES_48:
      return 48;
    case FDCAN_DLC_BYTES_64:
      return 64;
    default:
      return (uint8_t)rx_DL;
  }
}

/**
 * @brief   获取接收FIFO0的剩余空间
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_receive(hal_fdcan_context_t *ctx,
                                               hal_fdcan_instance_t instance,
                                               hal_fdcan_message_t *message) {
  (void)ctx;

  if (message == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  FDCAN_RxHeaderTypeDef rx_header;

  if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header,
                             message->data) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  message->id = rx_header.Identifier;
  message->is_extended = (rx_header.IdType == FDCAN_EXTENDED_ID);
  message->is_remote = (rx_header.RxFrameType == FDCAN_REMOTE_FRAME);
  message->data_length = get_can_rx_length(rx_header.DataLength);

  return HAL_FDCAN_OK;
}

/**
 * @brief   获取接收FIFO0的剩余空间
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_receive_level(hal_fdcan_context_t *ctx,
                                                    hal_fdcan_instance_t instance,
                                                    uint8_t fifo_address,
                                                    uint32_t *level) {
  (void)ctx;

  if (level == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  const FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  if (fifo_address) {
    *level = HAL_FDCAN_GetRxFifoFillLevel(
        hfdcan, FDCAN_RX_FIFO1);  // FIFO不为空,有可能在其他中断时有多帧数据进入
  } else {
    *level = HAL_FDCAN_GetRxFifoFillLevel(
        hfdcan, FDCAN_RX_FIFO0);  // FIFO不为空,有可能在其他中断时有多帧数据进入
  }

  return HAL_FDCAN_OK;
}

/**
 * @brief   获取发送FIFO的剩余空间
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_get_send_level(hal_fdcan_context_t *ctx,
                                                     hal_fdcan_instance_t instance,
                                                     uint32_t *level) {
  (void)ctx;

  if (level == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  const FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  *level = HAL_FDCAN_GetTxFifoFreeLevel(hfdcan);

  return HAL_FDCAN_OK;
}

/**
 * @brief   设置过滤器
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_set_filter(hal_fdcan_context_t *ctx,
                                                  hal_fdcan_instance_t instance,
                                                  const hal_fdcan_filter_t *filter) {
  (void)ctx;

  if (filter == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  FDCAN_FilterTypeDef bsp_filter = {
      .IdType = filter->is_extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID,
      .FilterIndex = filter->filter_index,
      .FilterType = FDCAN_FILTER_MASK,
      .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
      .FilterID1 = filter->id,
      .FilterID2 = filter->mask};

  if (HAL_FDCAN_ConfigFilter(hfdcan, &bsp_filter) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }
  return HAL_FDCAN_OK;
}

/**
 * @brief   设置FDCAN模式
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_set_mode(hal_fdcan_context_t *ctx,
                                                hal_fdcan_instance_t instance,
                                                hal_fdcan_mode_t mode) {
  (void)ctx;

  if (!validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  uint32_t fdcan_mode;

  switch (mode) {
    case HAL_FDCAN_MODE_NORMAL:
      fdcan_mode = FDCAN_MODE_NORMAL;
      break;
    case HAL_FDCAN_MODE_LOOPBACK:
      fdcan_mode = FDCAN_MODE_INTERNAL_LOOPBACK;
      break;
    case HAL_FDCAN_MODE_SILENT:
      fdcan_mode = FDCAN_MODE_BUS_MONITORING;
      break;
    case HAL_FDCAN_MODE_SILENT_LOOPBACK:
      fdcan_mode = FDCAN_MODE_INTERNAL_LOOPBACK | FDCAN_MODE_BUS_MONITORING;
      break;
    default:
      return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  // 停止FDCAN以更改模式
  if (HAL_FDCAN_Stop(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  // 更改模式
  hfdcan->Init.Mode = fdcan_mode;

  // 重新初始化FDCAN
  if (HAL_FDCAN_Init(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  // 重新启动FDCAN
  if (HAL_FDCAN_Start(hfdcan) != HAL_OK) {
    return HAL_FDCAN_ERROR_HARDWARE;
  }

  return HAL_FDCAN_OK;
}

/**
 * @brief   获取错误计数
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到的消息
 * @return 操作结果错误码
 */
static hal_fdcan_error_t stm32_fdcan_get_error_count(hal_fdcan_context_t *ctx,
                                                       hal_fdcan_instance_t instance,
                                                       uint32_t *error_count) {
  (void)ctx;

  if (error_count == NULL || !validate_fdcan_instance(instance)) {
    return HAL_FDCAN_ERROR_INVALID_PARAM;
  }

  const FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  FDCAN_ErrorCountersTypeDef error_counters;
  HAL_FDCAN_GetErrorCounters(hfdcan, &error_counters);
  *error_count = error_counters.RxErrorCnt + error_counters.TxErrorCnt;

  return HAL_FDCAN_OK;
}

/**
 * @brief   FDCAN中断处理函数
 * @param ctx FDCAN 上下文指针
 * @param instance FDCAN实例
 * @param message 接收到消息
 */ 
static void stm32_fdcan_irq_handler(hal_fdcan_context_t *ctx, hal_fdcan_instance_t instance) {
  (void)ctx;

  if (!validate_fdcan_instance(instance)) {
    return;
  }

  FDCAN_HandleTypeDef *hfdcan = fdcan_handle_map[instance];
  HAL_FDCAN_IRQHandler(hfdcan);
}

// /* FDCAN中断处理函数 */
// void FDCAN1_IT0_IRQHandler(void)
// {
//     hal_fdcan_irq_handler(HAL_FDCAN_INSTANCE_1);
// }
//
// void FDCAN1_IT1_IRQHandler(void)
// {
//     hal_fdcan_irq_handler(HAL_FDCAN_INSTANCE_1);
// }

#if defined(FDCAN2)
void FDCAN2_IT0_IRQHandler(void) {
  hal_fdcan_irq_handler(NULL, HAL_FDCAN_INSTANCE_2);
}

void FDCAN2_IT1_IRQHandler(void) {
  hal_fdcan_irq_handler(NULL, HAL_FDCAN_INSTANCE_2);
}
#endif

#if defined(FDCAN3)
void FDCAN3_IT0_IRQHandler(void) {
  hal_fdcan_irq_handler(NULL, HAL_FDCAN_INSTANCE_3);
}

void FDCAN3_IT1_IRQHandler(void) {
  hal_fdcan_irq_handler(NULL, HAL_FDCAN_INSTANCE_3);
}
#endif
