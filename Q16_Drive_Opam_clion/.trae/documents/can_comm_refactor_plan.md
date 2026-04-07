# CAN通信模块重构计划

## 概述

参考 `MODULE_CODING_GUIDE.md` 规范，重构 `can_comm` 模块，将协议解析部分使用 `protocol_parser` 模块实现。

---

## 现状分析

### 当前 can_comm 模块结构

| 文件 | 功能 |
|------|------|
| `can_comm.h` | 头文件，定义协议结构、配置结构体、实例结构体 |
| `can_comm.c` | 核心实现，包含协议解析、发送打包、实例管理 |
| `can_comm_port.c` | 硬件抽象层，CAN收发接口 |

### 当前协议格式

```
| 帧头 (5字节) | 数据载荷 | 帧尾 (2字节) |
| 'S' | data_length | sequence | timestamp | payload... | CRC8 | 'E' |
```

### 存在的问题

1. **命名不规范**：函数命名使用 `CANXxx` 大驼峰，不符合模块前缀规范
2. **结构体设计不规范**：配置和运行时状态混合，未嵌套配置结构体
3. **协议解析耦合**：协议解析逻辑内嵌在 `CANCommGetDataTransmit_V2` 中
4. **错误处理不统一**：部分函数返回指针，部分返回整数，无统一错误码

---

## 重构目标

1. 遵循 `MODULE_CODING_GUIDE.md` 规范
2. 使用 `protocol_parser` 实现协议解析
3. 保持向后兼容的公共接口
4. 提高代码可维护性和可测试性

---

## 重构方案

### 1. 文件结构调整

```
can_comm/
├── can_comm.h              # 模块头文件（重构）
├── can_comm.c              # 模块核心实现（重构）
├── can_comm_port.c         # 硬件抽象层（保持不变）
└── CAN_COMM_STYLE_GUIDE.md # 模块规范文档（新增，可选）
```

### 2. 类型定义重构

#### 2.1 错误码枚举

```c
typedef enum {
  CAN_COMM_OK = 0,                    /**< 操作成功 */
  CAN_COMM_ERROR_NULL_PTR,            /**< 空指针错误 */
  CAN_COMM_ERROR_INVALID_PARAM,       /**< 无效参数 */
  CAN_COMM_ERROR_UNINITIALIZED,       /**< 未初始化 */
  CAN_COMM_ERROR_MEMORY_ALLOC,        /**< 内存分配失败 */
  CAN_COMM_ERROR_INSTANCE_EXISTS,     /**< 实例已存在 */
  CAN_COMM_ERROR_INSTANCE_NOT_FOUND,  /**< 实例未找到 */
  CAN_COMM_ERROR_BUFFER_OVERFLOW,     /**< 缓冲区溢出 */
  CAN_COMM_ERROR_PARSE_FAILED,        /**< 解析失败 */
  CAN_COMM_ERROR_CRC_FAILED,          /**< CRC校验失败 */
  CAN_COMM_ERROR_SEQUENCE_ERROR,      /**< 序列号错误 */
} can_comm_error_t;
```

#### 2.2 接收回调函数类型

```c
typedef void (*can_comm_rx_callback_t)(void* user_data, const uint8_t* data, uint16_t len);
```

#### 2.3 接收配置结构体

```c
typedef struct {
  const char* name;                   /**< 配置名称 */
  hal_fdcan_instance_t instance;      /**< CAN实例标识 */
  uint32_t can_id;                    /**< 接收消息ID */
  uint16_t data_len;                  /**< 接收数据长度 */
  uint16_t offline_ms;                /**< 离线超时时间(ms) */
  uint8_t priority;                   /**< 接收优先级 0-最高 */
  uint8_t daemon_error_code;          /**< 守护进程错误码 */
  daemon_offline_cb_t offline_cb;     /**< 离线回调函数 */
  can_comm_rx_callback_t rx_cb;       /**< 接收完成回调（新增） */
} can_comm_rx_config_t;
```

#### 2.4 发送配置结构体

```c
typedef struct {
  const char* name;                   /**< 配置名称 */
  hal_fdcan_instance_t instance;      /**< CAN实例标识 */
  uint32_t can_id;                    /**< 发送消息ID */
  uint16_t data_len;                  /**< 发送数据长度 */
  uint8_t priority;                   /**< 发送优先级 0-最高 */
} can_comm_tx_config_t;
```

#### 2.5 接收上下文结构体

```c
typedef struct can_comm_rx can_comm_rx_t;

struct can_comm_rx {
  can_comm_rx_config_t config;        /**< 配置参数（嵌套） */
  protocol_parser_context_t* parser;  /**< 协议解析器实例 */
  daemon_context_t* daemon;           /**< 守护进程实例 */
  uint8_t* output_buffer;             /**< 输出缓冲区 */
  void* user_data;                    /**< 用户数据绑定指针 */
  uint8_t last_sequence;              /**< 最后接收的序列号 */
  uint32_t sequence_errors;           /**< 序列号错误计数 */
  bool initialized;                   /**< 初始化标志 */
  can_comm_rx_t* next;                /**< 链表指针 */
};
```

#### 2.6 发送上下文结构体

```c
typedef struct can_comm_tx can_comm_tx_t;

struct can_comm_tx {
  can_comm_tx_config_t config;        /**< 配置参数（嵌套） */
  kfifo_t* kfifo;                     /**< 发送缓冲区 */
  uint8_t* tx_data;                   /**< 发送数据指针 */
  uint8_t sequence;                   /**< 序列号计数器 */
  bool tx_ready;                      /**< 发送就绪标志 */
  bool initialized;                   /**< 初始化标志 */
  can_comm_tx_t* next;                /**< 链表指针 */
};
```

### 3. 公共接口重构

| 原接口 | 新接口 | 说明 |
|--------|--------|------|
| `CANRxRegister` | `can_comm_rx_register` | 注册接收实例 |
| `CANTxRegister` | `can_comm_tx_register` | 注册发送实例 |
| `CANCommUnregisterRx` | `can_comm_rx_unregister` | 注销接收实例 |
| `CANCommUnregisterTx` | `can_comm_tx_unregister` | 注销发送实例 |
| `CANCommUnregisterAll` | `can_comm_unregister_all` | 注销所有实例 |
| `CANCommInit` | `can_comm_init` | 初始化模块 |
| `CANGetTxInstance` | `can_comm_get_tx_instance` | 获取发送实例 |
| `CANGetRxInstance` | `can_comm_get_rx_instance` | 获取接收实例 |
| `CANRxBindData` | `can_comm_rx_bind_data` | 绑定接收数据 |
| `CANTxBindData` | `can_comm_tx_bind_data` | 绑定发送数据 |
| `CANCommGetDataTransmit_V2` | `can_comm_process_rx` | 处理接收数据 |
| `CANCommSendDataPackage_V2` | `can_comm_package_tx` | 打包发送数据 |
| `CANCommSendFlush` | `can_comm_flush_tx` | 刷新发送缓冲 |
| `CANCommGetStats` | `can_comm_get_stats` | 获取统计信息 |
| `CANCommResetStats` | `can_comm_reset_stats` | 重置统计信息 |
| `CANCommGetBusLoad` | `can_comm_get_bus_load` | 获取总线负载 |
| `CANCommGetRxCount` | `can_comm_get_rx_count` | 获取接收实例数 |
| `CANCommGetTxCount` | `can_comm_get_tx_count` | 获取发送实例数 |
| `CANCommGetSequenceErrors` | `can_comm_get_sequence_errors` | 获取序列号错误 |

### 4. 协议解析集成

#### 4.1 帧长度计算回调

```c
static uint16_t can_comm_get_frame_len(uint8_t* buffer, uint16_t len) {
  if (len < sizeof(can_comm_protocol_head_t)) {
    return 0;
  }
  can_comm_protocol_head_t* head = (can_comm_protocol_head_t*)buffer;
  if (head->start != 'S') {
    return 0;
  }
  return sizeof(can_comm_protocol_head_t) + head->data_length + sizeof(can_comm_protocol_end_t);
}
```

#### 4.2 帧校验回调

```c
static protocol_parser_error_t can_comm_check_frame(uint8_t* buffer, uint16_t len) {
  can_comm_protocol_head_t* head = (can_comm_protocol_head_t*)buffer;
  uint16_t payload_len = head->data_length;
  uint8_t* crc_ptr = buffer + sizeof(can_comm_protocol_head_t) + payload_len;
  
  uint8_t calc_crc = get_CRC8_check_sum(
      buffer + sizeof(can_comm_protocol_head_t), payload_len, 0);
  
  if (calc_crc != *crc_ptr) {
    return PROTOCOL_PARSER_ERROR_CHECKSUM;
  }
  return PROTOCOL_PARSER_OK;
}
```

---

## 实施步骤

### 步骤 1：重构头文件 `can_comm.h`

1. 添加错误码枚举 `can_comm_error_t`
2. 重构配置结构体，确保配置参数集中
3. 重构上下文结构体，嵌套配置结构体
4. 添加 `protocol_parser.h` 依赖
5. 更新函数声明，使用规范命名

### 步骤 2：重构源文件 `can_comm.c`

1. 实现协议解析回调函数
2. 修改 `can_comm_rx_register`，创建 `protocol_parser` 实例
3. 修改 `can_comm_rx_unregister`，销毁 `protocol_parser` 实例
4. 重写 `can_comm_process_rx`，使用 `protocol_parser_parse`
5. 重写 `can_comm_package_tx`，保持发送逻辑
6. 更新所有函数命名

### 步骤 3：更新 `can_comm_port.c`

1. 更新中断回调函数中的函数调用
2. 保持硬件抽象层不变

### 步骤 4：兼容性处理

1. 添加宏定义兼容旧接口名称
2. 确保现有调用代码无需修改

```c
/* 兼容性宏定义 */
#define CANRxRegister can_comm_rx_register
#define CANTxRegister can_comm_tx_register
// ... 其他兼容宏
```

### 步骤 5：验证测试

1. 编译检查无错误无警告
2. 功能测试：接收、发送、离线检测
3. 性能测试：确保实时性不受影响

---

## 风险评估

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 接口变更影响现有代码 | 高 | 提供兼容性宏定义 |
| 性能下降 | 中 | 优化回调函数，避免额外开销 |
| 内存占用增加 | 低 | 复用现有缓冲区 |

---

## 预期收益

1. **代码规范统一**：符合大厂编码规范
2. **模块解耦**：协议解析独立，便于测试和复用
3. **可维护性提升**：清晰的配置/上下文分离
4. **可扩展性增强**：易于支持新协议格式

---

## 时间估计

- 头文件重构：1小时
- 源文件重构：2小时
- 兼容性处理：0.5小时
- 测试验证：1小时

**总计：约4.5小时**
