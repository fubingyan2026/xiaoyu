//
// Created by fubingyan on 25-11-28.
//
/**
 * @file can_nm.c
 * @brief CAN网络管理模块实现文件
 * @details 提供CAN NM功能的状态机、消息处理和节点管理
 * @author fubingyan
 * @email qq:3245784484
 * @version V1.0.0
 * @date 2025-09-23
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "can_nm.h"

#include "bsp_delay.h"
#include "debug/debug.h"

/* ==================== Private Variables ==================== */

#define CAN_NM_PRINTF(...)  // CAN_NM_PRINTF(...)
/* NM管理器实例 */
nm_manager_t g_nm_manager = {0};
nm_statistics_t g_nm_stats = {0};

can_comm_rx_t* can_nm_rx;
can_comm_tx_t* can_nm_tx;

/* ==================== Private Functions ==================== */

/**
 * @brief 发送NM消息
 * @param nm_type NM消息类型
 */
static void send_nm_message(uint8_t nm_type) {
  ((nm_message_t*)(CANTxBindData(GetCanNMTxInstance())))->nm_type = nm_type;
  ((nm_message_t*)(CANTxBindData(GetCanNMTxInstance())))->source_node_id =
      g_nm_manager.local_node_id;
  ((nm_message_t*)(CANTxBindData(GetCanNMTxInstance())))->active_nodes =
      g_nm_manager.active_node_count;
  ((nm_message_t*)(CANTxBindData(GetCanNMTxInstance())))->nm_data = 0;
  /* 使用标准函数发送 */
  g_nm_stats.messages_sent++;

  CAN_NM_PRINTF("NM message sent: type=0x%02X, node=%d, active=%d\n", nm_type,
                g_nm_manager.local_node_id, g_nm_manager.active_node_count);
}

/**
 * @brief 处理唤醒消息
 */
static void handle_nm_wakeup_message(const nm_message_t* msg) {
  if (g_nm_manager.current_state == NM_STATE_BUS_SLEEP) {
    CAN_NM_PRINTF("NM: Wakeup message received from node %d\n",
                  msg->source_node_id);
    g_nm_manager.current_state = NM_STATE_NORMAL_OPERATION;
    g_nm_stats.wakeup_events++;
    g_nm_stats.state_changes++;

    /* 调用唤醒回调 */
    if (g_nm_manager.wakeup_callback != NULL) {
      g_nm_manager.wakeup_callback();
    }
  }
}

/**
 * @brief 处理活跃消息
 */
static void handle_nm_alive_message(const nm_message_t* msg) {
  /* 更新活跃节点信息 */
  if (msg->source_node_id < CAN_NM_MAX_NODES &&
      msg->source_node_id != g_nm_manager.local_node_id) {
    uint8_t was_active = g_nm_manager.nodes[msg->source_node_id].is_active;

    g_nm_manager.nodes[msg->source_node_id].is_active = 1;
    g_nm_manager.nodes[msg->source_node_id].last_seen_time = millis();
    g_nm_manager.nodes[msg->source_node_id].sleep_ready = 0;
    g_nm_manager.nodes[msg->source_node_id].nm_data = msg->nm_data;

    /* 如果节点从离线变为在线，调用回调 */
    if (!was_active && g_nm_manager.node_status_callback != NULL) {
      g_nm_manager.node_status_callback(msg->source_node_id, 1);
    }

    CAN_NM_PRINTF("NM: Alive message from node %d\n", msg->source_node_id);
  }
}

/**
 * @brief 处理睡眠指示消息
 */
static void handle_nm_sleep_ind_message(const nm_message_t* msg) {
  // CAN_NM_PRINTF("NM: Sleep indication from node %d\n", msg->source_node_id);

  /* 标记节点准备睡眠 */
  if (msg->source_node_id < CAN_NM_MAX_NODES) {
    g_nm_manager.nodes[msg->source_node_id].sleep_ready = 1;
  }

  if (g_nm_manager.current_state == NM_STATE_NORMAL_OPERATION) {
    /* 如果本节点也准备睡眠，发送应答 */
    if (g_nm_manager.sleep_condition_callback != NULL &&
        g_nm_manager.sleep_condition_callback()) {
      send_nm_message(CAN_NM_SLEEP_ACK_MSG);
      g_nm_manager.current_state = NM_STATE_PREPARE_SLEEP;
      g_nm_manager.prepare_start_time = millis();
      g_nm_stats.state_changes++;
    }
  }
}

/**
 * @brief 处理睡眠应答消息
 */
static void handle_nm_sleep_ack_message(const nm_message_t* msg) {
  // CAN_NM_PRINTF("NM: Sleep acknowledgement from node %d\n",
  // msg->source_node_id);

  /* 标记节点确认睡眠 */
  if (msg->source_node_id < CAN_NM_MAX_NODES) {
    g_nm_manager.nodes[msg->source_node_id].sleep_ready = 1;
  }
}

/**
 * @brief 检查所有节点是否准备睡眠
 */
static uint8_t all_nodes_ready_for_sleep(void) {
  for (int i = 0; i < CAN_NM_MAX_NODES; i++) {
    /* 跳过本地节点和不活跃节点 */
    if (i == g_nm_manager.local_node_id || !g_nm_manager.nodes[i].is_active) {
      continue;
    }

    if (!g_nm_manager.nodes[i].sleep_ready) {
      return 0;
    }
  }

  return 1;
}

/**
 * @brief 检查所有节点是否确认睡眠
 */
static uint8_t all_nodes_sleep_acknowledged(void) {
  return all_nodes_ready_for_sleep();
}

/**
 * @brief 更新节点状态
 * @param current_time 当前时间
 */
static void update_node_status(uint32_t current_time) {
  uint8_t active_count = 0;

  for (int i = 0; i < CAN_NM_MAX_NODES; i++) {
    /* 跳过本地节点 */
    if (i == g_nm_manager.local_node_id) {
      active_count++;  // 本地节点始终活跃
      continue;
    }

    /* 检查节点超时 */
    if (g_nm_manager.nodes[i].is_active &&
        current_time - g_nm_manager.nodes[i].last_seen_time >
            CAN_NM_TIMEOUT_MS) {
      // CAN_NM_PRINTF("NM: Node %d timeout\n", i);
      g_nm_manager.nodes[i].is_active = 0;
      g_nm_manager.nodes[i].sleep_ready = 0;
      g_nm_stats.node_timeouts++;

      /* 通知应用层节点离线 */
      if (g_nm_manager.node_status_callback != NULL) {
        g_nm_manager.node_status_callback(i, 0);
      }
    }

    if (g_nm_manager.nodes[i].is_active) {
      active_count++;
    }
  }

  g_nm_manager.active_node_count = active_count;
}

/**
 * @brief 处理总线睡眠状态
 */
static void handle_nm_bus_sleep_state(uint32_t current_time) {
  /* 检查唤醒条件 */
  if (g_nm_manager.wakeup_condition_callback != NULL &&
      g_nm_manager.wakeup_condition_callback()) {
    // CAN_NM_PRINTF("NM: Wakeup condition detected, transitioning to normal
    // operation\n");
    g_nm_manager.current_state = NM_STATE_NORMAL_OPERATION;
    g_nm_stats.state_changes++;

    /* 发送唤醒消息 */
    send_nm_message(CAN_NM_WAKEUP_MSG);

    /* 调用唤醒回调 */
    if (g_nm_manager.wakeup_callback != NULL) {
      g_nm_manager.wakeup_callback();
    }
  }
}

/**
 * @brief 处理正常工作状态
 */
static void handle_nm_normal_state(uint32_t current_time) {
  /* 周期发送NM消息 */
  if (current_time - g_nm_manager.last_tx_time >= CAN_NM_MSG_INTERVAL_MS) {
    send_nm_message(CAN_NM_ALIVE_MSG);
    g_nm_manager.last_tx_time = current_time;
  }

  /* 检查睡眠条件 */
  if (g_nm_manager.sleep_condition_callback != NULL &&
      g_nm_manager.sleep_condition_callback()) {
    // CAN_NM_PRINTF("NM: Sleep condition detected, preparing for sleep\n");
    g_nm_manager.current_state = NM_STATE_PREPARE_SLEEP;
    g_nm_manager.prepare_start_time = current_time;
    g_nm_stats.state_changes++;
    send_nm_message(CAN_NM_SLEEP_IND_MSG);
  }

  /* 检查总线活动 */
  if (current_time - g_nm_manager.last_rx_time > CAN_NM_TIMEOUT_MS) {
    // CAN_NM_PRINTF("NM: Bus activity timeout, considering sleep\n");
    g_nm_stats.timeout_events++;
    g_nm_manager.current_state = NM_STATE_PREPARE_SLEEP;
    g_nm_manager.prepare_start_time = current_time;
    g_nm_stats.state_changes++;
  }
}

/**
 * @brief 处理准备睡眠状态
 */
static void handle_nm_prepare_sleep_state(uint32_t current_time) {
  /* 检查是否所有节点都准备就绪 */
  if (all_nodes_ready_for_sleep()) {
    // CAN_NM_PRINTF("NM: All nodes ready for sleep\n");
    g_nm_manager.current_state = NM_STATE_READY_SLEEP;
    g_nm_stats.state_changes++;
    send_nm_message(CAN_NM_SLEEP_ACK_MSG);
    g_nm_manager.prepare_start_time = 0;
  } else if (current_time - g_nm_manager.prepare_start_time >
             CAN_NM_PREPARE_TIMEOUT_MS) {
    CAN_NM_PRINTF("NM: Sleep preparation timeout, returning to normal\n");
    g_nm_manager.current_state = NM_STATE_NORMAL_OPERATION;
    g_nm_stats.state_changes++;
    g_nm_manager.prepare_start_time = 0;
  }
}

/**
 * @brief 处理准备就绪睡眠状态
 */
static void handle_nm_ready_sleep_state(uint32_t current_time) {
  /* 检查是否所有节点都确认睡眠 */
  if (all_nodes_sleep_acknowledged()) {
    // CAN_NM_PRINTF("NM: Entering bus sleep\n");
    g_nm_manager.current_state = NM_STATE_BUS_SLEEP;
    g_nm_stats.state_changes++;
    g_nm_stats.sleep_entries++;

    /* 调用睡眠回调 */
    if (g_nm_manager.sleep_callback != NULL) {
      g_nm_manager.sleep_callback();
    }
  }
}

/* ==================== Public Functions ==================== */

/**
 * @brief 初始化网络管理
 * @param node_id 本地节点ID
 * @param sleep_cb 睡眠回调函数
 * @param wakeup_cb 唤醒回调函数
 * @param sleep_cond_cb 睡眠条件检查回调
 * @param wakeup_cond_cb 唤醒条件检查回调
 * @param node_status_cb 节点状态变化回调
 */
void CANNmInit(uint8_t node_id, void (*sleep_cb)(void), void (*wakeup_cb)(void),
               uint8_t (*sleep_cond_cb)(void), uint8_t (*wakeup_cond_cb)(void),
               void (*node_status_cb)(uint8_t, uint8_t)) {
  if (node_id >= CAN_NM_MAX_NODES) {
    CAN_NM_PRINTF("NM: Invalid node ID %d (max %d)\n", node_id,
                  CAN_NM_MAX_NODES - 1);
    return;
  }

  __memset(&g_nm_manager, 0, sizeof(nm_manager_t));
  __memset(&g_nm_stats, 0, sizeof(nm_statistics_t));

  g_nm_manager.local_node_id = node_id;
  g_nm_manager.current_state = NM_STATE_BUS_SLEEP;
  g_nm_manager.last_tx_time = millis();
  g_nm_manager.last_rx_time = millis();
  g_nm_manager.is_initialized = 1;

  /* 设置回调函数 */
  g_nm_manager.sleep_callback = sleep_cb;
  g_nm_manager.wakeup_callback = wakeup_cb;
  g_nm_manager.sleep_condition_callback = sleep_cond_cb;
  g_nm_manager.wakeup_condition_callback = wakeup_cond_cb;
  g_nm_manager.node_status_callback = node_status_cb;

  /* 初始化节点列表 */
  for (int i = 0; i < CAN_NM_MAX_NODES; i++) {
    g_nm_manager.nodes[i].node_id = i;
    g_nm_manager.nodes[i].is_active = 0;
    g_nm_manager.nodes[i].sleep_ready = 0;
    g_nm_manager.nodes[i].last_seen_time = 0;
    g_nm_manager.nodes[i].nm_data = 0;
  }

  /* 注册NM通信实例 */
  const can_rx_config_t nm_rx_config = {.instance = HAL_FDCAN_INSTANCE_1,
                                        .can_rx_identify = CAN_NM_ID_BASE,
                                        .callback = NULL,
                                        .offline_ms = CAN_NM_TIMEOUT_MS,
                                        .name = "CAN_NM",
                                        .rx_data_len = sizeof(nm_message_t),
                                        .priority = 0};

  const can_tx_config_t nm_tx_config = {.instance = HAL_FDCAN_INSTANCE_1,
                                        .can_tx_identify = CAN_NM_ID_BASE,
                                        .tx_data_len = sizeof(nm_message_t),
                                        .name = "CAN_NM",
                                        .priority = 0};

  can_nm_rx = CANRxRegister(&nm_rx_config);
  can_nm_tx = CANTxRegister(&nm_tx_config);

  ASSERT(can_nm_rx);
  ASSERT(can_nm_tx);
}

/**
 * @brief NM状态机处理
 */
void CANNmStateMachine(void) {
  if (!g_nm_manager.is_initialized) {
    return;
  }
  const uint32_t current_time = millis();

  /* 状态机处理 */
  switch (g_nm_manager.current_state) {
    case NM_STATE_BUS_SLEEP:
      handle_nm_bus_sleep_state(current_time);
      break;

    case NM_STATE_NORMAL_OPERATION:
      handle_nm_normal_state(current_time);
      break;

    case NM_STATE_PREPARE_SLEEP:
      handle_nm_prepare_sleep_state(current_time);
      break;

    case NM_STATE_READY_SLEEP:
      handle_nm_ready_sleep_state(current_time);
      break;

    default:
      g_nm_manager.current_state = NM_STATE_BUS_SLEEP;
      break;
  }

  /* 更新节点状态 */
  update_node_status(current_time);
}

/**
 * @brief 处理接收到的NM消息
 * @param data 数据指针
 * @param len 数据长度
 */
void CANNmProcessMessage(const uint8_t* data, uint16_t len) {
  if (!g_nm_manager.is_initialized || data == NULL ||
      len < sizeof(nm_message_t)) {
    return;
  }

  nm_message_t* nm_msg = (nm_message_t*)data;
  const uint32_t current_time = millis();

  g_nm_manager.last_rx_time = current_time;
  g_nm_stats.messages_received++;

  /* 更新节点状态 */
  if (nm_msg->source_node_id < CAN_NM_MAX_NODES &&
      nm_msg->source_node_id != g_nm_manager.local_node_id) {
    g_nm_manager.nodes[nm_msg->source_node_id].last_seen_time = current_time;
  }

  switch (nm_msg->nm_type) {
    case CAN_NM_WAKEUP_MSG:
      handle_nm_wakeup_message(nm_msg);
      break;

    case CAN_NM_ALIVE_MSG:
      handle_nm_alive_message(nm_msg);
      break;

    case CAN_NM_SLEEP_IND_MSG:
      handle_nm_sleep_ind_message(nm_msg);
      break;

    case CAN_NM_SLEEP_ACK_MSG:
      handle_nm_sleep_ack_message(nm_msg);
      break;

    default:
      CAN_NM_PRINTF("NM: Unknown message type: 0x%02X\n", nm_msg->nm_type);
      break;
  }
}

/**
 * @brief 请求网络睡眠
 * @note 应用层调用此函数请求进入睡眠
 */
void CANNmRequestSleep(void) {
  if (!g_nm_manager.is_initialized) {
    return;
  }

  if (g_nm_manager.current_state == NM_STATE_NORMAL_OPERATION) {
    // CAN_NM_PRINTF("NM: Sleep requested by application\n");
    g_nm_manager.current_state = NM_STATE_PREPARE_SLEEP;
    g_nm_manager.prepare_start_time = millis();
    g_nm_stats.state_changes++;
    send_nm_message(CAN_NM_SLEEP_IND_MSG);
  }
}

/**
 * @brief 请求网络唤醒
 * @note 应用层调用此函数请求唤醒网络
 */
void CANNmRequestWakeup(void) {
  if (!g_nm_manager.is_initialized) {
    return;
  }

  if (g_nm_manager.current_state == NM_STATE_BUS_SLEEP) {
    // CAN_NM_PRINTF("NM: Wakeup requested by application\n");
    g_nm_manager.current_state = NM_STATE_NORMAL_OPERATION;
    g_nm_stats.state_changes++;
    g_nm_stats.wakeup_events++;
    send_nm_message(CAN_NM_WAKEUP_MSG);

    if (g_nm_manager.wakeup_callback != NULL) {
      g_nm_manager.wakeup_callback();
    }
  }
}

/**
 * @brief 获取当前NM状态
 * @return 当前NM状态
 */
nm_state_e CANNmGetState(void) { return g_nm_manager.current_state; }

/**
 * @brief 获取活跃节点数量
 * @return 活跃节点数
 */
uint8_t CANNmGetActiveNodeCount(void) { return g_nm_manager.active_node_count; }

/**
 * @brief 获取节点状态
 * @param node_id 节点ID
 * @return 节点状态信息指针，未找到返回NULL
 */
const nm_node_info_t* CANNmGetNodeStatus(uint8_t node_id) {
  if (node_id < CAN_NM_MAX_NODES) {
    return &g_nm_manager.nodes[node_id];
  }
  return NULL;
}

/**
 * @brief 检查节点是否在线
 * @param node_id 节点ID
 * @return 在线返回1，离线返回0
 */
uint8_t CANNmIsNodeOnline(uint8_t node_id) {
  if (node_id < CAN_NM_MAX_NODES) {
    return g_nm_manager.nodes[node_id].is_active;
  }
  return 0;
}

/**
 * @brief 获取NM统计信息
 * @return 统计信息指针
 */
nm_statistics_t* CANNmGetStatistics(void) { return &g_nm_stats; }

/**
 * @brief 重置NM统计信息
 */
void CANNmResetStatistics(void) {
  __memset(&g_nm_stats, 0, sizeof(nm_statistics_t));
  // CAN_NM_PRINTF("NM statistics reset\n");
}

/**
 * @brief 强制设置NM状态（调试用）
 * @param state 目标状态
 */
void CANNmSetStateForced(const nm_state_e state) {
  if (state > NM_STATE_READY_SLEEP) {
    return;
  }

  CAN_NM_PRINTF("NM: State forced to %d\n", state);
  g_nm_manager.current_state = state;
  g_nm_stats.state_changes++;
}

/**
 * @brief 获取CAN NM接收实例
 * @return
 * 返回指向can_comm_rx_t类型的指针，如果实例存在则返回该实例的指针，否则返回NULL
 */
can_comm_rx_t* GetCanNMRxInstance(void) {
  if (can_nm_rx) return can_nm_rx;
  return NULL;
}

/**
 * @brief 获取CAN NM发送实例
 * @return 返回指向CAN NM发送实例的指针，如果实例不存在则返回NULL
 */
can_comm_tx_t* GetCanNMTxInstance(void) {
  if (can_nm_tx) return can_nm_tx;
  return NULL;
}

/** examples **/
/**
 * @brief 应用层任务
 */
void application_task(void) {
  /* 应用层业务逻辑 */

  /* 示例：在特定条件下请求睡眠 */
  // if (should_enter_sleep_mode())
  {
    CANNmRequestSleep();
  }

  /* 示例：处理节点状态变化 */
  if (!CANNmIsNodeOnline(2))  // 检查节点2是否离线
  {
    // handle_node2_offline();
  }
}

/**
 * @brief 执行CAN网络管理任务
 * @details
 * 该函数定期检查并显示当前的网络状态，包括总线状态、活跃节点数和总线负载。
 *          如果总线处于睡眠状态，可以采取进一步的低功耗措施。
 */
void can_nm_task(void) {
  /* 显示网络状态（可选） */
  static uint32_t last_status_time = 0;
  if (millis() - last_status_time > 2000) {
    nm_state_e current_state = CANNmGetState();
    uint8_t active_nodes = CANNmGetActiveNodeCount();
    uint8_t bus_load = 0;

    CAN_NM_PRINTF("NM Status: state=%d, active_nodes=%d, bus_load=%d%%\n",
                  current_state, active_nodes, bus_load);

    last_status_time = millis();
  }

  /* 低功耗处理 */
  if (CANNmGetState() == NM_STATE_BUS_SLEEP) {
    /* 在睡眠状态下可以进入更深度的低功耗模式 */
    // enter_deep_sleep_if_possible();
  }
}
