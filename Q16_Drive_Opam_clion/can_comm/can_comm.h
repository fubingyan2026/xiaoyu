/**
 * @file can_comm.h
 * @brief CAN通信模块头文件
 * @details 提供CAN通信的配置、数据结构和函数声明
 * @author fubingyan
 * @email qq:3245784484
 * @version V2.0.0
 * @date 2025-09-23
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#ifndef CAN_COMM_H
#define CAN_COMM_H

#include "daemon.h"
#include "hal_fdcan.h"
#include "kfifo/kfifo.h"
#include "stdint.h"

/* CAN通信配置宏定义 */
#define CAN_ONE_FRAME_SEND_LEN 24  /**< define一帧率发送的长度*/
#define CAN_MAX_DATA_LEN (128)     /**< 最大数据长度 */
#define CAN_FIFO_MULTIPLIER 2      /**< FIFO缓冲区倍数 */
#define CAN_DEFAULT_OFFLINE_MS 100 /**< 默认离线超时时间(ms) */
#define CAN_MIN_OFFLINE_MS 5       /**< 最小离线超时时间(ms) */
#define CAN_MAX_TX_BATCH_FRAMES 4  /**< 最大批量发送帧数 */
#define CAN_MAX_RETRY_COUNT 1      /**< 最大重试次数 */
#define CAN_RETRY_DELAY_MS 2       /**< 重试延迟(ms) */

/**
 * @brief CAN通信协议格式
 * @details 定义CAN通信的数据包格式
 */
#pragma pack(1)
typedef struct {
  char start;          /**< 'S' - 起始字节 */
  uint8_t data_length; /**< 数据长度 */
  uint8_t sequence;    /**< 新增：序列号，防重放 */
  uint16_t timestamp;  /**< 新增：时间戳(低16位) */
} can_comm_protocol_head_t;
typedef struct {
  uint8_t crc; /**< CRC8校验 */
  char end;    /**< 'E' - 结束字节 */
} can_comm_protocol_end_t;
#pragma pack()

/**
 * @brief CAN通信统计信息结构体
 * @details 记录CAN通信的各项统计数据
 */
typedef struct {
  uint32_t total_rx_bytes;  /**< 总接收字节数 */
  uint32_t total_tx_bytes;  /**< 总发送字节数 */
  uint32_t rx_errors;       /**< 接收错误计数 */
  uint32_t tx_errors;       /**< 发送错误计数 */
  uint32_t crc_errors;      /**< CRC校验错误计数 */
  uint32_t timeout_errors;  /**< 超时错误计数 */
  uint32_t protocol_errors; /**< 协议错误计数 */

  /* 新增帧数统计 */
  uint32_t total_rx_frames;
  uint32_t total_tx_frames;
} can_comm_stats_t;

/**
 * @brief CAN接收配置结构体
 * @details 配置CAN接收实例的各项参数
 */
typedef struct {
  hal_fdcan_instance_t instance; /**< CAN实例标识 */
  uint32_t can_rx_identify;      /**< 接收消息ID */
  daemon_offline_cb_t callback;  /**< 离线回调函数指针 */
  uint16_t offline_ms;           /**< 离线超时时间(ms) */
  uint8_t daemon_error_code;     /**< 守护进程错误码 */
  const char* name;              /**< 配置名称 */
  uint16_t rx_data_len;          /**< 接收数据长度 */
  uint8_t priority;              /**< 接收优先级 0-最高, 255-最低 */
} can_rx_config_t;

/**
 * @brief CAN发送配置结构体
 * @details 配置CAN发送实例的各项参数
 */
typedef struct {
  hal_fdcan_instance_t instance; /**< CAN实例标识 */
  uint32_t can_tx_identify;      /**< 发送消息ID */
  uint16_t tx_data_len;          /**< 发送数据长度 */
  const char* name;              /**< 配置名称 */
  uint8_t priority;              /**< 发送优先级 0-最高, 255-最低 */
} can_tx_config_t;

/**
 * @brief CAN接收实例结构体
 * @details 管理CAN接收实例的所有状态和数据
 */
typedef struct can_comm_rx {
  can_rx_config_t config;              /**< 接收配置信息 */
  kfifo_t* kfifo_ptr;                  /**< 环形缓冲区指针 */
  daemon_context_t* daemon_can_rx_ptr; /**< 守护进程指针 */
  uint8_t last_sequence;               /**< 最后接收的序列号 */
  uint32_t sequence_errors;            /**< 序列号错误计数 */
  void* direct_binding_ptr;            // 新增：直接数据绑定指针
  struct can_comm_rx* next;            /**< 下一个接收实例指针 */
} can_comm_rx_t;

/**
 * @brief CAN发送实例结构体
 * @details 管理CAN发送实例的所有状态和数据
 */
typedef struct can_comm_tx {
  can_tx_config_t config;    /**< 发送配置信息 */
  kfifo_t* kfifo_ptr;        /**< 环形缓冲区指针 */
  uint8_t sequence_counter;  /**< 实例私有序列号 */
  void* direct_tx_data_ptr;  // 新增：指向应用层发送数据的指针
  uint8_t direct_tx_ready;
  struct can_comm_tx* next; /**< 下一个发送实例指针 */
} can_comm_tx_t;

/* 全局变量声明 */
extern can_comm_rx_t* g_can_comm_rx_list; /**< CAN接收实例链表头指针 */
extern can_comm_tx_t* g_can_comm_tx_list; /**< CAN发送实例链表头指针 */
extern can_comm_stats_t g_can_stats;      /**< CAN通信统计信息 */

/** can_comm_public **/
can_comm_rx_t* CANRxRegister(const can_rx_config_t* config);
can_comm_tx_t* CANTxRegister(const can_tx_config_t* config);

void CANCommGetDataTransmit_V2(void);
void CANCommSendDataPackage_V2(void);
void CANCommSendFlush(void);

void* CANRxBindData(can_comm_rx_t* rx);
void* CANTxBindData(can_comm_tx_t* tx);

void CANCommUnregisterRx(can_comm_rx_t* instance);
void CANCommUnregisterTx(can_comm_tx_t* instance);
void CANCommUnregisterAll(void);
void CANCommInit(void);

can_comm_tx_t* CANGetTxInstance(const char* name);
can_comm_rx_t* CANGetRxInstance(const char* name);

uint32_t CANCommGetRxCount(void);
uint32_t CANCommGetTxCount(void);

/** can_comm_port **/
uint32_t can_comm_get_receive_level(hal_fdcan_instance_t instance,
                                    uint8_t fifo_address);
uint32_t can_comm_get_send_level(hal_fdcan_instance_t instance);

bool can_comm_receive(hal_fdcan_instance_t instance,
                      hal_fdcan_message_t* message);
bool can_comm_send(hal_fdcan_instance_t instance,
                   const hal_fdcan_message_t* message);

/** can_comm_state **/
can_comm_stats_t* CANCommGetStats(void);
void CANCommResetStats(void);
uint8_t CANCommGetBusLoad(void);
uint32_t CANCommGetSequenceErrors(const char* rx_name);

#endif /* CAN_COMM_H */
