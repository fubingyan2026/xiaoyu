//
// Created by fubingyan on 25-11-27.
//

/**
 * @file    can_comm_port.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2025-04-05
 * @brief   CAN通信硬件抽象层实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "can_comm.h"
#include "public.h"
#include "fdcan.h"

/* Private constants ---------------------------------------------------------*/

#define CAN_SERVER_INFO(fmt, ...) DEBUG_LOGI("can", fmt, ##__VA_ARGS__)

/* Private variables ---------------------------------------------------------*/

static hal_fdcan_context_t fdcan1_ctx;
static uint8_t fdcan_initialized = 0;

/* Private function prototypes -----------------------------------------------*/

static void ensure_fdcan_initialized(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 确保FDCAN已初始化
 */
static void ensure_fdcan_initialized(void) {
  if (!fdcan_initialized) {
    stm32_fdcan_init_context(&fdcan1_ctx);
    fdcan_initialized = 1;
  }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 获取接收FIFO数据量
 * @param instance CAN实例
 * @param fifo_address FIFO地址
 * @return 数据量
 */
uint32_t can_comm_get_receive_level(hal_fdcan_instance_t instance,
                                    uint8_t fifo_address) {
  ensure_fdcan_initialized();
  uint32_t level = 0;
  hal_fdcan_get_receive_level(&fdcan1_ctx, instance, fifo_address, &level);
  return level;
}

/**
 * @brief 获取发送FIFO空闲量
 * @param instance CAN实例
 * @return 空闲量
 */
uint32_t can_comm_get_send_level(hal_fdcan_instance_t instance) {
  ensure_fdcan_initialized();
  uint32_t level = 0;
  hal_fdcan_get_send_fifo_level(&fdcan1_ctx, instance, &level);
  return level;
}

/**
 * @brief 接收CAN消息
 * @param instance CAN实例
 * @param message 消息结构体指针
 * @return 成功返回true，失败返回false
 */
bool can_comm_receive(hal_fdcan_instance_t instance,
                      hal_fdcan_message_t* message) {
  ensure_fdcan_initialized();
  hal_fdcan_error_t ret = hal_fdcan_receive(&fdcan1_ctx, instance, message);
  return (ret == HAL_FDCAN_OK);
}

/**
 * @brief 发送CAN消息
 * @param instance CAN实例
 * @param message 消息结构体指针
 * @return 成功返回true，失败返回false
 */
bool can_comm_send(hal_fdcan_instance_t instance,
                   const hal_fdcan_message_t* message) {
  ensure_fdcan_initialized();
  hal_fdcan_error_t ret = hal_fdcan_send(&fdcan1_ctx, instance, message);
  return (ret == HAL_FDCAN_OK);
}

/* ==================== interrupt function ==================== */

/**
 * @brief HAL库CAN回调函数，接收电机数据
 * @param hfdcan CAN句柄指针
 * @param RxFifo0ITs 接收FIFO中断标志
 * @note 在CAN接收中断中调用，处理接收到的数据
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan,
                               const uint32_t RxFifo0ITs) {
  static hal_fdcan_message_t rx_msg;

  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
    while (can_comm_get_receive_level(HAL_FDCAN_INSTANCE_1, 0)) {
      if (can_comm_receive(HAL_FDCAN_INSTANCE_1, &rx_msg)) {
        g_can_stats.total_rx_bytes += rx_msg.data_length;
        g_can_stats.total_rx_frames++;

        can_comm_rx_t* current = g_can_comm_rx_list;
        while (current != NULL) {
          if (current->config.instance == HAL_FDCAN_INSTANCE_1 &&
              current->config.can_id == rx_msg.id) {
            if (current->parser != NULL && current->initialized) {
              protocol_parser_feed(current->parser, rx_msg.data,
                                   rx_msg.data_length);
            }
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
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef* hfdcan,
                               uint32_t RxFifo1ITs) {
  if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET) {
    static hal_fdcan_message_t rx_msg;

    while (can_comm_get_receive_level(HAL_FDCAN_INSTANCE_1, 1)) {
      if (can_comm_receive(HAL_FDCAN_INSTANCE_1, &rx_msg)) {
        g_can_stats.total_rx_bytes += rx_msg.data_length;
        g_can_stats.total_rx_frames++;

        can_comm_rx_t* current = g_can_comm_rx_list;
        while (current != NULL) {
          if (current->config.instance == HAL_FDCAN_INSTANCE_1 &&
              current->config.can_id == rx_msg.id) {
            if (current->parser != NULL && current->initialized) {
              protocol_parser_feed(current->parser, rx_msg.data,
                                   rx_msg.data_length);
            }
            break;
          }
          current = current->next;
        }
      }
    }
  }
}

/**
 * @brief 发送FIFO空中断回调
 * @param hfdcan CAN句柄指针
 */
void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef* hfdcan) {
  (void)hfdcan;
}

/**
 * @brief FDCAN错误状态回调
 * @param hfdcan 外设句柄
 * @param ErrorStatusITs HAL汇总来的错误标志位集合
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan,
                                   uint32_t ErrorStatusITs) {
  const uint32_t ecr = hfdcan->Instance->ECR;
  const uint32_t rx_err_cnt = (ecr >> 8) & 0xFFU;
  const uint32_t tx_err_cnt = (ecr >> 0) & 0xFFU;

  if (ErrorStatusITs & FDCAN_IT_BUS_OFF) {
    CAN_SERVER_INFO("==> Bus-Off detected!\n");
    HAL_FDCAN_Stop(hfdcan);
    hfdcan->Instance->CCCR &= ~FDCAN_CCCR_INIT_Msk;
    HAL_FDCAN_Start(hfdcan);
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
    CAN_SERVER_INFO("->FDCAN_IT_TX_FIFO_EMPTY\n");
  }

  if (ErrorStatusITs & FDCAN_IT_RX_FIFO0_FULL) {
    (void)0;
  }
  if (ErrorStatusITs & FDCAN_IT_RX_FIFO1_FULL) {
    (void)0;
  }

  static uint32_t last_clear = 0;
  if (HAL_GetTick() - last_clear > 1000) {
    hfdcan->Instance->ECR = 0;
    last_clear = HAL_GetTick();
  }
}
