//
// Created by fubingyan on 25-11-27.
//

#include "can_comm.h"
#include "debug/debug.h"
#include "fdcan.h"

#define CAN_SERVER_INFO DEBUG_ERROR

static hal_fdcan_context_t fdcan1_ctx;
static uint8_t fdcan_initialized = 0;

static void ensure_fdcan_initialized(void) {
  if (!fdcan_initialized) {
    stm32_fdcan_init_context(&fdcan1_ctx);
    fdcan_initialized = 1;
  }
}

uint32_t can_comm_get_receive_level(hal_fdcan_instance_t instance,
                                    uint8_t fifo_address) {
  ensure_fdcan_initialized();
  uint32_t level = 0;
  hal_fdcan_get_receive_level(&fdcan1_ctx, instance, fifo_address, &level);
  return level;
}

uint32_t can_comm_get_send_level(hal_fdcan_instance_t instance) {
  ensure_fdcan_initialized();
  uint32_t level = 0;
  hal_fdcan_get_send_fifo_level(&fdcan1_ctx, instance, &level);
  return level;
}

bool can_comm_receive(hal_fdcan_instance_t instance,
                      hal_fdcan_message_t *message) {
  ensure_fdcan_initialized();
  hal_fdcan_error_t ret = hal_fdcan_receive(&fdcan1_ctx, instance, message);
  return (ret == HAL_FDCAN_OK);
}

bool can_comm_send(hal_fdcan_instance_t instance,
                   const hal_fdcan_message_t *message) {
  ensure_fdcan_initialized();
  hal_fdcan_error_t ret = hal_fdcan_send(&fdcan1_ctx, instance, message);
  return (ret == HAL_FDCAN_OK);
}

/* ==================== interrupt function ==================== */

/**
 * @brief HAL库CAN回调函数,接收电机数据,并将数据解包后存入相应数组
 * @param hfdcan CAN句柄指针
 * @param RxFifo0ITs 接收FIFO中断标志
 * @note 在CAN接收中断中调用，处理接收到的数据
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               const uint32_t RxFifo0ITs) {
  static hal_fdcan_message_t rx_msg;

  /* 检查是否有新消息 */
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
    /* 从RX FIFO0检索接收消息 */
    while (can_comm_get_receive_level(HAL_FDCAN_INSTANCE_1, 0)) {
      if (can_comm_receive(HAL_FDCAN_INSTANCE_1, &rx_msg)) {
        g_can_stats.total_rx_bytes += rx_msg.data_length;
        g_can_stats.total_rx_frames++;  // 记得在结构体里加上这个成员
        /* 遍历接收实例链表，匹配消息ID */
        can_comm_rx_t *current = g_can_comm_rx_list;
        while (current != NULL) {
          if (current->config.instance == HAL_FDCAN_INSTANCE_1 &&
              current->config.can_rx_identify == rx_msg.id) {
            /* 将数据存入对应的环形缓冲区 */
            kfifo_put(current->kfifo_ptr, rx_msg.data, rx_msg.data_length);
            break;
          }
          current = current->next;
        }
      }
    }
  }
}

/**
 * @brief FIFO1回调函数，分担FIFO0的处理压力
 * @param hfdcan CAN句柄指针
 * @param RxFifo1ITs 接收FIFO1中断标志
 * @note 用于处理高优先级消息，提高系统响应速度
 */
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo1ITs) {
  if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET) {
    static hal_fdcan_message_t rx_msg;

    /* 从FIFO1读取，分担FIFO0的压力 */
    while (can_comm_get_receive_level(HAL_FDCAN_INSTANCE_1, 1)) {
      if (can_comm_receive(HAL_FDCAN_INSTANCE_1, &rx_msg)) {
        /* 高优先级消息处理 */
        g_can_stats.total_rx_bytes += rx_msg.data_length;
        g_can_stats.total_rx_frames++;  // 记得在结构体里加上这个成员
        can_comm_rx_t *current = g_can_comm_rx_list;
        while (current != NULL) {
          if (current->config.instance == HAL_FDCAN_INSTANCE_1 &&
              current->config.can_rx_identify == rx_msg.id) {
            kfifo_put(current->kfifo_ptr, rx_msg.data, rx_msg.data_length);
            // 新增
            g_can_stats.total_rx_frames++;
            g_can_stats.total_rx_bytes += rx_msg.data_length;
            break;
          }
          current = current->next;
        }
      }
    }
  }
}

void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan) {}

/**
 * @brief  FDCAN 错误状态回调，所有错误中断都会先进这里
 * @param  hfdcan          : 外设句柄
 * @param  ErrorStatusITs  : HAL 汇总来的错误标志位集合
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan,
                                   uint32_t ErrorStatusITs) {
  /* 1. 读当前错误计数器 --------------------------------------------------*/
  const uint32_t ecr = hfdcan->Instance->ECR;
  const uint32_t rx_err_cnt = (ecr >> 8) & 0xFFU;
  const uint32_t tx_err_cnt = (ecr >> 0) & 0xFFU;

  /* 3. 逐个解析并处理 -----------------------------------------------------*/
  if (ErrorStatusITs & FDCAN_IT_BUS_OFF) {
    CAN_SERVER_INFO("==> Bus-Off detected!\n");

    /* 一键退出 Bus-Off：先进入 INIT，再清 INIT，硬件自动复位错误计数 */
    HAL_FDCAN_Stop(hfdcan);                          // 置 INIT + CCE
    hfdcan->Instance->CCCR &= ~FDCAN_CCCR_INIT_Msk;  // 清 INIT，退出 Bus-Off
    HAL_FDCAN_Start(hfdcan);                         // 重新参与总线
    CAN_SERVER_INFO("==> Bus-Off recovered\n");
  }

  if (ErrorStatusITs & FDCAN_IT_ERROR_WARNING) {
    CAN_SERVER_INFO("==> Error Warning (RxCnt=%lu TxCnt=%lu)\n", rx_err_cnt,
                    tx_err_cnt);
  }

  if (ErrorStatusITs & FDCAN_IT_ERROR_PASSIVE) {
    __HAL_FDCAN_CLEAR_FLAG(hfdcan, FDCAN_FLAG_ERROR_PASSIVE);
    CAN_SERVER_INFO("==> Error Passive\n");
  }

  if (ErrorStatusITs & FDCAN_IT_ARB_PROTOCOL_ERROR) {
    CAN_SERVER_INFO("==> Arbitration Phase Protocol Error\n");
  }

  if (ErrorStatusITs & FDCAN_IT_DATA_PROTOCOL_ERROR) {
    CAN_SERVER_INFO("==> Data Phase Protocol Error\n");
  }

  if (ErrorStatusITs & FDCAN_IT_TX_FIFO_EMPTY) {
    /* 只是示例，可删 */
    /* 用户想干的事 */
    CAN_SERVER_INFO("->FDCAN_IT_TX_FIFO_EMPTY\n");
  }

  if (ErrorStatusITs & FDCAN_IT_RX_FIFO0_FULL) {
  }
  if (ErrorStatusITs & FDCAN_IT_RX_FIFO1_FULL) {
  }
  // 定期清除错误计数器
  static uint32_t last_clear = 0;
  if (HAL_GetTick() - last_clear > 1000) {
    hfdcan->Instance->ECR = 0;  // 清除错误状态
    last_clear = HAL_GetTick();
  }
}
