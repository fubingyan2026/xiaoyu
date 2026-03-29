//
// Created by fubingyan on 25-11-28.
//

#ifndef CAN_NM_H
#define CAN_NM_H

#include "can_comm.h"
#include "stdint.h"

/* CAN NM配置宏定义 */
#define CAN_NM_ID_BASE 0x500           /**< NM消息ID基址 */
#define CAN_NM_MSG_INTERVAL_MS 100     /**< NM消息发送间隔 */
#define CAN_NM_TIMEOUT_MS 0xFFFF       /**< NM超时时间 */
#define CAN_NM_PREPARE_TIMEOUT_MS 1000 /**< 准备睡眠超时时间 */
#define CAN_NM_MAX_NODES 16            /**< 最大节点数 */

/* NM消息类型定义 */
#define CAN_NM_WAKEUP_MSG 0x01    /**< 唤醒消息 */
#define CAN_NM_ALIVE_MSG 0x02     /**< 活跃消息 */
#define CAN_NM_SLEEP_IND_MSG 0x03 /**< 睡眠指示 */
#define CAN_NM_SLEEP_ACK_MSG 0x04 /**< 睡眠应答 */

/**
 * @brief NM状态枚举
 */
typedef enum
{
    NM_STATE_BUS_SLEEP = 0,    /**< 总线睡眠 */
    NM_STATE_NORMAL_OPERATION, /**< 正常工作 */
    NM_STATE_PREPARE_SLEEP,    /**< 准备睡眠 */
    NM_STATE_READY_SLEEP       /**< 准备就绪睡眠 */
} nm_state_e;

/**
 * @brief 节点状态信息
 */
typedef struct
{
    uint8_t node_id;         /**< 节点ID */
    uint32_t last_seen_time; /**< 最后出现时间 */
    uint8_t is_active;       /**< 是否活跃 */
    uint8_t sleep_ready;     /**< 睡眠就绪 */
    uint8_t nm_data;         /**< NM数据 */
} nm_node_info_t;

/**
 * @brief NM消息结构
 * @note 使用标准协议格式，数据部分包含NM特定内容
 */
#pragma pack(1)
typedef struct
{
    uint8_t nm_type;        /**< NM消息类型 */
    uint8_t source_node_id; /**< 源节点ID */
    uint8_t active_nodes;   /**< 活跃节点数 */
    uint8_t nm_data;        /**< NM数据 */
} nm_message_t;
#pragma pack()

/**
 * @brief NM管理器结构
 */
typedef struct
{
    nm_state_e current_state;    /**< 当前状态 */
    uint8_t local_node_id;       /**< 本地节点ID */
    uint32_t last_tx_time;       /**< 最后发送时间 */
    uint32_t last_rx_time;       /**< 最后接收时间 */
    uint32_t prepare_start_time; /**< 准备睡眠开始时间 */
    uint8_t active_node_count;   /**< 活跃节点数 */
    uint8_t is_initialized;      /**< 初始化标志 */

    /* 回调函数指针 */
    void (*sleep_callback)(void);                                     /**< 睡眠回调函数 */
    void (*wakeup_callback)(void);                                    /**< 唤醒回调函数 */
    uint8_t (*sleep_condition_callback)(void);                        /**< 睡眠条件检查回调 */
    uint8_t (*wakeup_condition_callback)(void);                       /**< 唤醒条件检查回调 */
    void (*node_status_callback)(uint8_t node_id, uint8_t is_online); /**< 节点状态变化回调 */

    nm_node_info_t nodes[CAN_NM_MAX_NODES]; /**< 节点列表 */
} nm_manager_t;

/**
 * @brief NM统计信息
 */
typedef struct
{
    uint32_t state_changes;     /**< 状态切换次数 */
    uint32_t messages_sent;     /**< 发送消息数 */
    uint32_t messages_received; /**< 接收消息数 */
    uint32_t sleep_entries;     /**< 进入睡眠次数 */
    uint32_t wakeup_events;     /**< 唤醒事件数 */
    uint32_t timeout_events;    /**< 超时事件数 */
    uint32_t node_timeouts;     /**< 节点超时次数 */
} nm_statistics_t;

/* 全局变量声明 */
extern nm_manager_t g_nm_manager;
extern nm_statistics_t g_nm_stats;

/* ==================== Public Functions ==================== */

void CANNmInit(uint8_t node_id, void (*sleep_cb)(void), void (*wakeup_cb)(void), uint8_t (*sleep_cond_cb)(void),
               uint8_t (*wakeup_cond_cb)(void), void (*node_status_cb)(uint8_t, uint8_t));

void CANNmStateMachine(void);

void CANNmProcessMessage(const uint8_t *data, uint16_t len);

void CANNmRequestSleep(void);

void CANNmRequestWakeup(void);

nm_state_e CANNmGetState(void);

uint8_t CANNmGetActiveNodeCount(void);

const nm_node_info_t *CANNmGetNodeStatus(uint8_t node_id);

uint8_t CANNmIsNodeOnline(uint8_t node_id);

nm_statistics_t *CANNmGetStatistics(void);

void CANNmResetStatistics(void);

void CANNmSetStateForced(nm_state_e state);

can_comm_rx_t *GetCanNMRxInstance(void);

can_comm_tx_t *GetCanNMTxInstance(void);

void can_nm_task(void);

#endif /* CAN_NM_H */
