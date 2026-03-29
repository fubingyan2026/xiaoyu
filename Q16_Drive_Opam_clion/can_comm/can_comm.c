/**
 * @file can_comm.c
 * @brief CAN通信模块实现文件
 * @details 提供CAN通信的初始化、数据处理和发送接收功能
 * @author fubingyan
 * @email qq:3245784484
 * @version V1.0.0
 *          V1.0.1 更新了协议的序列化，加上快速配置和诊断服务
 *          V1.0.2 增加网络调度功能
 *          V1.0.3 优化发送的调度
 *          V1.0.4 去掉队列通信增强实时性
 * @date 2025-09-23
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "can_comm.h"

#include "bsp_delay.h"
#include "can_nm.h"
#include "crc/crc.h"
#include "debug/debug.h"
#include "hal_fdcan.h"
#include "memory_pool/memory_pool.h"

#define CAN_COMM_PRINTF(...)  // DEBUG_INFO(__VA_ARGS__)

/* ==================== Public ==================== */
can_comm_rx_t *g_can_comm_rx_list = NULL; /**< CAN接收实例链表头指针 */
can_comm_tx_t *g_can_comm_tx_list = NULL; /**< CAN发送实例链表头指针 */
can_comm_stats_t g_can_stats = {0};       /**< CAN通信统计信息 */
static const uint8_t protocol_len =
    sizeof(can_comm_protocol_head_t) + sizeof(can_comm_protocol_end_t);
static const uint8_t protocol_head_len = sizeof(can_comm_protocol_head_t);

/* ==================== Private ==================== */
static uint8_t validate_can_rx_config(const can_rx_config_t *config);
static uint8_t validate_can_tx_config(const can_tx_config_t *config);
static uint8_t floor_to_std(uint8_t input_value);

/**
 * @brief 验证CAN接收配置的有效性
 * @param config CAN接收配置指针
 * @return 有效返回1，无效返回0
 * @note 检查配置指针、名称和数据长度的有效性
 */
static uint8_t validate_can_rx_config(const can_rx_config_t *config) {
  /* 检查配置指针是否为空 */
  if (config == NULL) {
    CAN_COMM_PRINTF("CAN RX config is NULL\n");
    return 0;
  }
  /* 检查配置名称是否为空 */
  if (config->name == NULL) {
    CAN_COMM_PRINTF("CAN RX config name is NULL\n");
    return 0;
  }
  /* 检查数据长度是否在有效范围内 */
  if (config->rx_data_len == 0 || config->rx_data_len > CAN_MAX_DATA_LEN) {
    CAN_COMM_PRINTF("CAN RX config data length invalid: %d\n",
                    config->rx_data_len);
    return 0;
  }
  /* 检查离线超时时间是否过小 */
  if (config->offline_ms < CAN_MIN_OFFLINE_MS) {
    CAN_COMM_PRINTF("CAN RX offline timeout too small: %d\n",
                    config->offline_ms);
  }
  return 1;
}

/**
 * @brief 验证CAN发送配置的有效性
 * @param config CAN发送配置指针
 * @return 有效返回1，无效返回0
 * @note 检查配置指针、名称和数据长度的有效性
 */
static uint8_t validate_can_tx_config(const can_tx_config_t *config) {
  /* 检查配置指针是否为空 */
  if (config == NULL) {
    CAN_COMM_PRINTF("CAN TX config is NULL\n");
    return 0;
  }
  /* 检查配置名称是否为空 */
  if (config->name == NULL) {
    CAN_COMM_PRINTF("CAN TX config name is NULL\n");
    return 0;
  }
  /* 检查数据长度是否在有效范围内 */
  if (config->tx_data_len == 0 || config->tx_data_len > CAN_MAX_DATA_LEN) {
    CAN_COMM_PRINTF("CAN TX config data length invalid: %d\n",
                    config->tx_data_len);
    return 0;
  }
  return 1;
}

/**
 * @brief 向下取整到最近标准值
 * @param x 输入数据长度
 * @return 标准数据长度 (0-8/12/16/20/24/32/48/64)
 * @note 用于将数据长度规范化为CAN标准帧长度
 */
static uint8_t floor_to_std(const uint8_t input_value) {
  if (input_value <= 8) return input_value;
  static const uint8_t STANDARD_VALUES[] = {8, 12, 16, 20, 24, 32, 48, 64};
  for (int i = sizeof(STANDARD_VALUES) / sizeof(STANDARD_VALUES[0]) - 1; i >= 0;
       --i) {
    if (input_value >= STANDARD_VALUES[i]) {
      return STANDARD_VALUES[i];
    }
  }
  return STANDARD_VALUES[0];  // 默认返回8
}

/**
 * @brief 注册CAN接收实例
 * @param config CAN接收配置指针
 * @return 成功返回CAN接收实例指针，失败返回NULL
 * @note 创建新地接收实例并初始化所有相关资源
 */
can_comm_rx_t *CANRxRegister(const can_rx_config_t *config) {
  /* 验证配置有效性 */
  if (!validate_can_rx_config(config)) return NULL;
  /* 检查是否已存在同名实例 */
  can_comm_rx_t *existing = CANGetRxInstance(config->name);
  if (existing != NULL) {
    CAN_COMM_PRINTF("CAN RX instance %s already exists\n", config->name);
    return existing; /* 返回已存在的实例 */
  }
  /* 创建新实例内存 */
  can_comm_rx_t *new_instance = __malloc(sizeof(can_comm_rx_t));
  if (new_instance == NULL) {
    CAN_COMM_PRINTF("Failed to allocate memory for CAN RX instance %s\n",
                    config->name);
    return NULL;
  }
  /* 初始化内存为零 */
  __memset(new_instance, 0, sizeof(can_comm_rx_t));
  new_instance->last_sequence = 0xFF;  // 表示未接收过任何数据
  /* 复制配置信息 */
  __memcpy(&new_instance->config, config, sizeof(can_rx_config_t));
  /* 初始化环形缓冲区 */
  new_instance->kfifo_ptr = kfifo_alloc(
      (config->rx_data_len + protocol_len) * CAN_FIFO_MULTIPLIER, NULL);
  if (new_instance->kfifo_ptr == NULL) {
    CAN_COMM_PRINTF("Failed to allocate kfifo for CAN RX instance %s\n",
                    config->name);
    __free(new_instance);
    return NULL;
  }
  /* 初始化守护进程 */
  const daemon_init_config_t daemon_config = {
      .callback = config->callback,
      .owner_pointer = new_instance,
      .owner_name = config->name,
      .reload_time_out =
          config->offline_ms > 0 ? config->offline_ms : CAN_DEFAULT_OFFLINE_MS,
      .init_wait_time = 200,
  };
  new_instance->daemon_can_rx_ptr = DaemonRegister(&daemon_config);
  if (new_instance->daemon_can_rx_ptr == NULL) {
    CAN_COMM_PRINTF("Failed to register daemon for CAN RX instance %s\n",
                    config->name);
    kfifo_free(new_instance->kfifo_ptr);
    __free(new_instance);
    return NULL;
  }

  new_instance->direct_binding_ptr = __malloc(config->rx_data_len);
  if (new_instance->direct_binding_ptr == NULL) {
    CAN_COMM_PRINTF(
        "Failed to allocate direct_binding_ptr for CAN RX instance %s\n",
        config->name);
    MessageCenterUnregister(config->name);
    if (new_instance->daemon_can_rx_ptr != NULL) {
      DaemonUnregister(new_instance->daemon_can_rx_ptr->config.owner_name);
    }
    if (new_instance->kfifo_ptr != NULL) kfifo_free(new_instance->kfifo_ptr);
    __free(new_instance);
    return NULL;
  }

  /* 添加到链表头部 */
  new_instance->next = g_can_comm_rx_list;
  g_can_comm_rx_list = new_instance;
  CAN_COMM_PRINTF("CAN RX instance %s registered successfully\n", config->name);
  return new_instance;
}

/**
 * @brief 注册CAN发送实例
 * @param config CAN发送配置指针
 * @return 成功返回CAN发送实例指针，失败返回NULL
 * @note 创建新地发送实例并初始化所有相关资源
 */
can_comm_tx_t *CANTxRegister(const can_tx_config_t *config) {
  /* 验证配置有效性 */
  if (!validate_can_tx_config(config)) return NULL;
  /* 检查是否已存在同名实例 */
  can_comm_tx_t *existing = CANGetTxInstance(config->name);
  if (existing != NULL) {
    CAN_COMM_PRINTF("CAN TX instance %s already exists\n", config->name);
    return existing; /* 返回已存在的实例 */
  }
  /* 创建新实例内存 */
  can_comm_tx_t *new_instance =
      (can_comm_tx_t *)__malloc(sizeof(can_comm_tx_t));
  if (new_instance == NULL) {
    CAN_COMM_PRINTF("Failed to allocate memory for CAN TX instance %s\n",
                    config->name);
    return NULL;
  }
  /* 初始化内存为零 */
  __memset(new_instance, 0, sizeof(can_comm_tx_t));
  /* 复制配置信息 */
  __memcpy(&new_instance->config, config, sizeof(can_tx_config_t));
  /* 初始化环形缓冲区 */
  new_instance->kfifo_ptr = kfifo_alloc(
      (config->tx_data_len + protocol_len) * CAN_FIFO_MULTIPLIER, NULL);
  if (new_instance->kfifo_ptr == NULL) {
    CAN_COMM_PRINTF("Failed to allocate kfifo for CAN TX instance %s\n",
                    config->name);
    __free(new_instance);
    return NULL;
  }
  new_instance->direct_tx_data_ptr = __malloc(config->tx_data_len);
  if (new_instance->direct_tx_data_ptr == NULL) {
    CAN_COMM_PRINTF(
        "Failed to allocate direct_tx_data_ptr for CAN TX instance %s\n",
        config->name);
    if (new_instance->kfifo_ptr != NULL) kfifo_free(new_instance->kfifo_ptr);
    __free(new_instance);
    return NULL;
  }
  /* 添加到链表头部 */
  new_instance->next = g_can_comm_tx_list;
  g_can_comm_tx_list = new_instance;

  CAN_COMM_PRINTF("CAN TX instance %s registered successfully\n", config->name);
  return new_instance;
}

/**
 * @brief 注销CAN接收实例
 * @param instance 要注销的CAN接收实例指针
 * @note 释放实例占用的所有资源并从链表中移除
 */
void CANCommUnregisterRx(can_comm_rx_t *instance) {
  if (instance == NULL) {
    return;
  }
  can_comm_rx_t **current = &g_can_comm_rx_list;
  while (*current != NULL) {
    if (*current == instance) {
      *current = instance->next;
      /* 释放环形缓冲区 */
      if (instance->kfifo_ptr != NULL) kfifo_free(instance->kfifo_ptr);
      /* 注销守护进程 */
      if (instance->daemon_can_rx_ptr != NULL)
        DaemonUnregister(instance->daemon_can_rx_ptr->config.owner_name);
      /* 注销消息中心 */
      if (instance->direct_binding_ptr != NULL)
        __free(instance->direct_binding_ptr);
      /* 释放实例内存 */
      __free(instance);
      CAN_COMM_PRINTF("CAN RX instance unregistered successfully\n");
      return;
    }
    current = &(*current)->next;
  }
}

/**
 * @brief 注销CAN发送实例
 * @param instance 要注销的CAN发送实例指针
 * @note 释放实例占用的所有资源并从链表中移除
 */
void CANCommUnregisterTx(can_comm_tx_t *instance) {
  if (instance == NULL) {
    return;
  }
  can_comm_tx_t **current = &g_can_comm_tx_list;
  while (*current != NULL) {
    if (*current == instance) {
      *current = instance->next;
      /* 释放环形缓冲区 */
      if (instance->kfifo_ptr != NULL) kfifo_free(instance->kfifo_ptr);
      /* 释放直接数据指针 */
      if (instance->direct_tx_data_ptr != NULL)
        __free(instance->direct_tx_data_ptr);
      /* 释放实例内存 */
      __free(instance);
      CAN_COMM_PRINTF("CAN TX instance unregistered successfully\n");
      return;
    }
    current = &(*current)->next;
  }
}

/**
 * @brief 注销所有CAN实例
 * @note 清理所有CAN接收和发送实例
 */
void CANCommUnregisterAll(void) {
  /* 注销所有接收实例 */
  while (g_can_comm_rx_list != NULL) {
    CANCommUnregisterRx(g_can_comm_rx_list);
  }

  /* 注销所有发送实例 */
  while (g_can_comm_tx_list != NULL) {
    CANCommUnregisterTx(g_can_comm_tx_list);
  }

  CAN_COMM_PRINTF("All CAN instances unregistered\n");
}

/**
 * @brief 优化后的接收处理函数
 * @note 1. 移除状态机，直接按包长处理
 *       2. 使用栈内存缓冲，减少堆访问
 *       3. 如果绑定了指针，直接拷贝，跳过 PubSub
 */
void CANCommGetDataTransmit_V2(void) {
  can_comm_rx_t *current = g_can_comm_rx_list;

  // 在栈上开辟一个临时缓冲区，大小为最大包长 (64 + 7 协议头尾)
  // STM32 栈访问速度极快
  uint8_t temp_buffer[CAN_MAX_DATA_LEN + 10];

  while (current != NULL) {
    // 1. 计算该节点期望的完整包长度 (协议头 + 数据 + 协议尾)
    const uint16_t total_packet_len = current->config.rx_data_len +
                                      sizeof(can_comm_protocol_head_t) +
                                      sizeof(can_comm_protocol_end_t);
    const uint16_t header_len = sizeof(can_comm_protocol_head_t);

    // 2. 循环处理 FIFO 中的所有数据
    while (1) {
      // [优化点1] 一次性获取长度，减少锁开销
      uint32_t fifo_len = kfifo_len(current->kfifo_ptr);

      // 如果数据不够一个包，直接跳出，处理下一个电机
      if (fifo_len < total_packet_len) break;

      // [优化点2] 预读取(Peek)完整的一帧数据到栈缓冲区
      // 注意：这里只是看，不移动 FIFO 指针
      kfifo_peek(current->kfifo_ptr, temp_buffer, total_packet_len, 0);

      // 3. 快速校验帧头 (Magic Word 'S' 和 长度)
      can_comm_protocol_head_t *p_head =
          (can_comm_protocol_head_t *)temp_buffer;

      if (p_head->start != 'S' ||
          p_head->data_length != current->config.rx_data_len) {
        // [滑动窗口] 帧头不对，丢弃 1 个字节，继续寻找下一字节
        kfifo_skip(current->kfifo_ptr, 1);
        g_can_stats.protocol_errors++;
        continue;
      }

      // 4. 校验帧尾 (Magic Word 'E')
      // 访问数组下标比结构体指针更安全防止对齐问题
      if (temp_buffer[total_packet_len - 1] != 'E') {
        kfifo_skip(current->kfifo_ptr,
                   1);  // 尾部不对，说明可能只是数据里恰好有个 'S'
        continue;
      }

      // 5. 校验 CRC (这是最耗时的步骤，放在最后做)
      uint8_t calculated_crc = get_CRC8_check_sum(
          temp_buffer + header_len, current->config.rx_data_len, 0);
      if (calculated_crc != temp_buffer[total_packet_len - 2]) {
        kfifo_skip(current->kfifo_ptr, 1);  // CRC 挂了
        g_can_stats.crc_errors++;
        continue;
      }

      // --- 至此，数据包完全合法 ---

      // [优化点3] 确认有效后，直接从 FIFO 中移除这一整包数据
      // 使用 skip 而不是 get，因为数据已经在 temp_buffer 里了，不需要再拷贝一次
      kfifo_skip(current->kfifo_ptr, total_packet_len);

      // 更新统计信息

      current->last_sequence = p_head->sequence;
      // 喂狗
      DaemonReload(current->daemon_can_rx_ptr);

      // [优化点4] 极速分发：如果绑定了直接指针，直接 Memcpy
      if (current->direct_binding_ptr != NULL) {
        // 指针运算：跳过头部，只拷贝 Payload
        __memcpy((void *)current->direct_binding_ptr, temp_buffer + header_len,
                 current->config.rx_data_len);
      }
    }
    if (current == GetCanNMRxInstance()) {
      /* NM消息 - 调用NM模块处理 */
      CANNmProcessMessage((uint8_t *)CANRxBindData(current),
                          current->config.rx_data_len);
      current = current->next;
      continue;
    }
    current = current->next;
  }
}

/**
 * @brief 优化后的发送打包函数
 * @note 1. 移除 PubSub，支持直接指针读取
 *       2. 统一时间戳，减少 HAL_GetTick 调用
 *       3. 增加 FIFO 空间预检查
 *       4. 使用栈内存，速度更快
 */
void CANCommSendDataPackage_V2(void) {
  can_comm_tx_t *current = g_can_comm_tx_list;

  // 1. [优化] 统一获取时间戳，保证本次循环所有电机时间同步，且减少函数调用
  uint16_t current_timestamp = (uint16_t)(millis() & 0xFFFF);
  // 2. [优化] 使用栈上的局部缓冲区 (STM32G4 访问栈极快)
  uint8_t temp_buffer[CAN_MAX_DATA_LEN];
  while (current != NULL) {
    uint8_t data_len = current->config.tx_data_len;
    uint8_t package_total_len = data_len + sizeof(can_comm_protocol_head_t) +
                                sizeof(can_comm_protocol_end_t);

    // 3. [优化] 预检查：如果 kfifo 没空间，根本不要开始打包，直接跳过
    // 这样避免了打包产生的 CPU 浪费
    if (kfifo_status(current->kfifo_ptr) == KFIFO_FULL ||
        (current->kfifo_ptr->size - kfifo_len(current->kfifo_ptr)) <
            package_total_len) {
      g_can_stats.tx_errors++;  // 记录因缓冲满而丢弃
      current = current->next;
      continue;
    }

    uint8_t has_new_data = 0;

    // 4. [核心优化] 分支处理：优先使用直接指针
    if (current->direct_tx_data_ptr != NULL && current->direct_tx_ready) {
      current->direct_tx_ready = 0;
      // 方案A：直接指针模式 (Zero Copy 变种)
      // 填充数据段
      __memcpy(temp_buffer + sizeof(can_comm_protocol_head_t),
               current->direct_tx_data_ptr, data_len);
      has_new_data = 1;
    }

    if (has_new_data) {
      // 5. 填充协议头 (使用指针转换更直观)
      can_comm_protocol_head_t *p_head =
          (can_comm_protocol_head_t *)temp_buffer;
      p_head->start = 'S';
      p_head->data_length = data_len;
      p_head->sequence = current->sequence_counter++;
      p_head->timestamp = current_timestamp;  // 使用统一时间戳

      // 6. 填充 CRC 和 协议尾
      // 注意：这里继续使用 temp_buffer 索引
      uint8_t *p_crc =
          &temp_buffer[sizeof(can_comm_protocol_head_t) + data_len];
      *p_crc = get_CRC8_check_sum(
          &temp_buffer[sizeof(can_comm_protocol_head_t)], data_len, 0);

      *(p_crc + 1) = 'E';  // End byte

      // 7. 写入 kfifo
      // 因为前面已经做过空间检查，这里理论上不会失败
      kfifo_put(current->kfifo_ptr, temp_buffer, package_total_len);
    }

    current = current->next;
  }
}

/**
 * @brief 发送CAN数据
 * @note 遍历所有发送实例，发送其FIFO中的数据
 *  不要试图在一个周期内清空 kfifo，而是只要填满硬件 TX FIFO 就停止
 *  让硬件在后台自己发送，CPU 去做别的事
 */
void CANCommSendFlush(void) {
  const can_comm_tx_t *current = g_can_comm_tx_list;
  const uint8_t send_one_frame_len = floor_to_std(CAN_ONE_FRAME_SEND_LEN);
  /* 遍历所有发送实例 */
  while (current != NULL) {
    const uint8_t one_packet_lmax_len =
        floor_to_std(current->config.tx_data_len + protocol_len);
    /* 检查是否有待发送数据 */
    uint8_t get_fifo_len = one_packet_lmax_len < send_one_frame_len
                               ? one_packet_lmax_len
                               : send_one_frame_len;

    /* 检查发送FIFO是否有空间 */
    while (kfifo_len(current->kfifo_ptr) > 0) {
      if (can_comm_get_send_level(current->config.instance) > 0) {
        static hal_fdcan_message_t message;
        const uint8_t get_len =
            kfifo_get(current->kfifo_ptr, message.data, get_fifo_len);
        message.id = current->config.can_tx_identify;
        message.data_length = get_len;

        // 直接发送，不要重试循环
        if (can_comm_send(current->config.instance, &message)) {
          g_can_stats.total_tx_bytes += get_len;
        }
        g_can_stats.total_tx_frames++;
      } else {
        break;
      }
    }

    // 如果硬件 FIFO 满，不做任何事，保留 kfifo 数据下次发，或者清空 kfifo
    // (根据实时性要求) 建议：如果是电机控制指令，直接丢弃 kfifo
    // 数据可能更好，保证发出去的是最新的
    current = current->next;
  }
}

/**
 * @brief 检查CAN总线负载
 * @return 估算的总线负载率(0-100)💯
 * @note 基于发送/接收字节数估算总线负载
 */
/* 定义波特率 (根据你的实际 CubeMX 配置修改 !!!) */
#define CAN_BAUD_NOMINAL 1000000.0f  // 1Mbps, 仲裁段
#define CAN_BAUD_DATA \
  1000000.0f  // 5Mbps, 数据段 (如果是普通CAN，这里也填 1000000)

/* CAN FD 协议开销估算 (bits) */
/* 仲裁段开销: SOF(1)+ID(11)+RRS(1)+IDE(1)+FDF(1)+Res(1)+BRS(1)+ESI(1)+DLC(4) ≈
 * 22 bits */
#define CAN_FD_OVERHEAD_ARB 22

/* 数据段开销: CRC(17/21)+CRC_DEL(1)+ACK(1)+ACK_DEL(1)+EOF(7)+IFS(3) ≈ 30 bits
 * (保守估计) */
/* 注意：CRC在FD中随长度变化，这里取平均值 */
#define CAN_FD_OVERHEAD_DATA 30

/* 位填充系数 (Bit Stuffing): 经验值通常增加 10%-15% 的长度 */
#define CAN_STUFFING_FACTOR 1.15f

uint8_t CANCommGetBusLoad(void) {
  static uint32_t last_time = 0;
  /* 上一次的快照 */
  static uint32_t last_rx_bytes = 0;
  static uint32_t last_tx_bytes = 0;
  static uint32_t last_rx_frames = 0;
  static uint32_t last_tx_frames = 0;
  static uint8_t load = 0;
  const uint32_t current_time = millis();
  /* 每 100ms 计算一次，太快会抖动，太慢不实时 */
  if (current_time > last_time && (current_time - last_time) >= 100) {
    /* 1. 计算时间间隔 (us) */
    float interval_us = (current_time - last_time) * 1000.0f;
    /* 2. 获取增量 */
    uint32_t delta_rx_bytes = g_can_stats.total_rx_bytes - last_rx_bytes;
    uint32_t delta_tx_bytes = g_can_stats.total_tx_bytes - last_tx_bytes;
    uint32_t delta_rx_frames = g_can_stats.total_rx_frames - last_rx_frames;
    uint32_t delta_tx_frames = g_can_stats.total_tx_frames - last_tx_frames;

    uint32_t delta_total_frames = delta_rx_frames + delta_tx_frames;
    uint32_t delta_total_bytes = delta_rx_bytes + delta_tx_bytes;

    /* 3. 计算物理占用时间 (核心优化) */
    // A. 仲裁段耗时 (运行在 1Mbps)
    // 每一帧都有固定的仲裁头
    float time_arb_us = (float)delta_total_frames * CAN_FD_OVERHEAD_ARB *
                        (1000000.0f / CAN_BAUD_NOMINAL);

    // B. 数据段耗时 (运行在 5Mbps)
    // 包含：Payload数据(bytes*8) + 数据段的CRC/ACK等开销
    float bits_data_phase =
        (delta_total_bytes * 8) + (delta_total_frames * CAN_FD_OVERHEAD_DATA);
    float time_data_us = bits_data_phase * (1000000.0f / CAN_BAUD_DATA);

    // C. 总线占用总时间 (加上位填充系数)
    float total_bus_time_us =
        (time_arb_us + time_data_us) * CAN_STUFFING_FACTOR;

    /* 4. 计算百分比 */
    float load_percent = (total_bus_time_us / interval_us) * 100.0f;

    /* 5. 限幅与更新 */
    if (load_percent > 100.0f) load_percent = 100.0f;
    load = (uint8_t)load_percent;

    /* 更新快照 */
    last_time = current_time;
    last_rx_bytes = g_can_stats.total_rx_bytes;
    last_tx_bytes = g_can_stats.total_tx_bytes;
    last_rx_frames = g_can_stats.total_rx_frames;
    last_tx_frames = g_can_stats.total_tx_frames;
  }

  return load;
}

void *CANRxBindData(can_comm_rx_t *rx) {
  if (rx->direct_binding_ptr) {
    return rx->direct_binding_ptr;
  }
  return NULL;
}

void *CANTxBindData(can_comm_tx_t *tx) {
  if (tx->direct_tx_data_ptr) {
    tx->direct_tx_ready = 1;
    return tx->direct_tx_data_ptr;
  }
  return NULL;
}

/**
 * @brief 根据名称获取CAN发送实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_tx_t *CANGetTxInstance(const char *name) {
  if (name == NULL) return NULL;
  can_comm_tx_t *current = g_can_comm_tx_list;
  while (current != NULL) {
    if (__strcmp(current->config.name, name) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief 根据名称获取CAN接收实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_rx_t *CANGetRxInstance(const char *name) {
  if (name == NULL) return NULL;
  can_comm_rx_t *current = g_can_comm_rx_list;
  while (current != NULL) {
    if (__strcmp(current->config.name, name) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief 获取CAN接收实例数量
 * @return 接收实例数量
 */
uint32_t CANCommGetRxCount(void) {
  uint32_t count = 0;
  const can_comm_rx_t *current = g_can_comm_rx_list;
  while (current != NULL) {
    count++;
    current = current->next;
  }
  return count;
}

/**
 * @brief 获取CAN发送实例数量
 * @return 发送实例数量
 */
uint32_t CANCommGetTxCount(void) {
  uint32_t count = 0;
  const can_comm_tx_t *current = g_can_comm_tx_list;
  while (current != NULL) {
    count++;
    current = current->next;
  }
  return count;
}

/**
 * @brief 获取CAN通信统计信息
 * @return 统计信息结构体指针
 */
can_comm_stats_t *CANCommGetStats(void) {
  return &g_can_stats;
}

/**
 * @brief 重置统计信息
 */
void CANCommResetStats(void) {
  __memset(&g_can_stats, 0, sizeof(can_comm_stats_t));
  CAN_COMM_PRINTF("CAN statistics reset\n");
}

/**
 * @brief 获取接收实例的序列号错误统计
 * @param rx_name 接收实例名称
 * @return 序列号错误计数
 */
uint32_t CANCommGetSequenceErrors(const char *rx_name) {
  const can_comm_rx_t *rx_instance = CANGetRxInstance(rx_name);
  if (rx_instance == NULL) return 0;
  return rx_instance->sequence_errors;
}

// end of the file!
