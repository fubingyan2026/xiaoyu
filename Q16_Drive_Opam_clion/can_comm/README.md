# CAN通信模块

基于 STM32 FDCAN 的通用 CAN 通信框架，支持多实例管理、协议解析、离线检测和统计监控。

## 功能特性

- **多实例管理**：支持注册多个接收/发送实例，通过链表管理
- **协议解析**：集成 `protocol_parser` 模块，支持自定义帧格式
- **离线检测**：集成 `daemon` 守护进程模块，自动检测通信超时
- **统计监控**：实时统计收发字节数、错误计数、总线负载等
- **序列号防重放**：支持序列号校验，防止数据重放攻击

## 目录结构

```
can_comm/
├── can_comm.h        # 模块头文件，定义接口和数据结构
├── can_comm.c        # 模块核心实现
├── can_comm_port.c   # 硬件抽象层，CAN收发接口
└── README.md         # 本文档
```

## 协议格式

```
| 帧头 (3字节) | 数据载荷 | 帧尾 (2字节) |
+-------------+---------+-------------+
| 'S' | len | seq | payload... | CRC8 | 'E' |
+-----+-----+-----+------------+------+-----+
  1B    1B    1B       N B        1B    1B
```

| 字段 | 大小 | 说明 |
|------|------|------|
| start | 1B | 起始字节，固定为 'S' (0x53) |
| data_length | 1B | 数据载荷长度 |
| sequence | 1B | 序列号，用于防重放 |
| payload | N B | 数据载荷 |
| crc | 1B | CRC8校验值 |
| end | 1B | 结束字节，固定为 'E' (0x45) |

## 快速开始

### 1. 初始化

```c
#include "can_comm.h"

// 在系统初始化时调用
daemon_init(millis);    // 初始化守护进程系统
can_comm_init();        // 初始化CAN通信模块
```

### 2. 注册接收实例

```c
// 定义离线回调函数
static void on_can_offline(void* rx) {
    can_comm_rx_t* instance = (can_comm_rx_t*)rx;
    if (!daemon_is_online(instance->daemon)) {
        printf("%s: offline!\n", instance->config.name);
    }
}

// 注册接收实例
can_comm_rx_config_t rx_config = {
    .name = "motor_controller",
    .instance = HAL_FDCAN_INSTANCE_1,
    .can_id = 0x100,
    .data_len = 32,
    .offline_ms = 250,
    .priority = 1,
    .offline_cb = on_can_offline,
    .rx_cb = NULL,  // 可选：接收完成回调
};

can_comm_rx_t* rx_instance = can_comm_rx_register(&rx_config);
```

### 3. 注册发送实例

```c
can_comm_tx_config_t tx_config = {
    .name = "gimbal_motor",
    .instance = HAL_FDCAN_INSTANCE_1,
    .can_id = 0x200,
    .data_len = 16,
    .priority = 2,
};

can_comm_tx_t* tx_instance = can_comm_tx_register(&tx_config);
```

### 4. 主循环处理

```c
void main_loop(void) {
    while (1) {
        // 处理接收数据
        can_comm_process_rx();
        
        // 打包发送数据
        can_comm_package_tx();
        
        // 刷新发送缓冲区
        can_comm_flush_tx();
        
        // 守护进程任务
        daemon_task();
    }
}
```

### 5. 数据绑定与访问

```c
// 绑定接收数据缓冲区
typedef struct {
    int16_t velocity[4];
    uint8_t status;
} motor_data_t;

motor_data_t* motor_data = (motor_data_t*)can_comm_rx_bind_data(rx_instance);

// 读取数据
if (daemon_is_online(rx_instance->daemon)) {
    int16_t velocity = motor_data->velocity[0];
}

// 绑定发送数据缓冲区
typedef struct {
    float velocity_rpm;
    uint8_t mode;
} motor_cmd_t;

motor_cmd_t* motor_cmd = (motor_cmd_t*)can_comm_tx_bind_data(tx_instance);

// 写入数据
motor_cmd->velocity_rpm = 1000.0f;
motor_cmd->mode = 1;
```

## API 参考

### 实例管理

| 函数 | 说明 |
|------|------|
| `can_comm_init()` | 初始化CAN通信模块 |
| `can_comm_rx_register()` | 注册接收实例 |
| `can_comm_tx_register()` | 注册发送实例 |
| `can_comm_rx_unregister()` | 注销接收实例 |
| `can_comm_tx_unregister()` | 注销发送实例 |
| `can_comm_unregister_all()` | 注销所有实例 |

### 实例查询

| 函数 | 说明 |
|------|------|
| `can_comm_get_rx_instance()` | 根据名称获取接收实例 |
| `can_comm_get_tx_instance()` | 根据名称获取发送实例 |
| `can_comm_get_rx_count()` | 获取接收实例数量 |
| `can_comm_get_tx_count()` | 获取发送实例数量 |

### 数据处理

| 函数 | 说明 |
|------|------|
| `can_comm_rx_bind_data()` | 绑定接收数据缓冲区 |
| `can_comm_tx_bind_data()` | 绑定发送数据缓冲区 |
| `can_comm_process_rx()` | 处理接收数据（主循环调用） |
| `can_comm_package_tx()` | 打包发送数据（主循环调用） |
| `can_comm_flush_tx()` | 刷新发送缓冲区（主循环调用） |

### 统计信息

| 函数 | 说明 |
|------|------|
| `can_comm_get_stats()` | 获取统计信息结构体 |
| `can_comm_reset_stats()` | 重置统计信息 |
| `can_comm_get_bus_load()` | 获取总线负载率(0-100) |
| `can_comm_get_sequence_errors()` | 获取序列号错误计数 |

## 数据结构

### 接收配置结构体

```c
typedef struct {
    const char* name;               // 配置名称
    hal_fdcan_instance_t instance;  // CAN实例标识
    uint32_t can_id;                // 接收消息ID
    uint16_t data_len;              // 接收数据长度
    uint16_t offline_ms;            // 离线超时时间(ms)
    uint8_t priority;               // 接收优先级 0-最高
    uint8_t daemon_error_code;      // 守护进程错误码
    daemon_offline_cb_t offline_cb; // 离线回调函数
    can_comm_rx_callback_t rx_cb;   // 接收完成回调
} can_comm_rx_config_t;
```

### 发送配置结构体

```c
typedef struct {
    const char* name;              // 配置名称
    hal_fdcan_instance_t instance; // CAN实例标识
    uint32_t can_id;               // 发送消息ID
    uint16_t data_len;             // 发送数据长度
    uint8_t priority;              // 发送优先级 0-最高
} can_comm_tx_config_t;
```

### 统计信息结构体

```c
typedef struct {
    uint32_t total_rx_bytes;   // 总接收字节数
    uint32_t total_tx_bytes;   // 总发送字节数
    uint32_t rx_errors;        // 接收错误计数
    uint32_t tx_errors;        // 发送错误计数
    uint32_t crc_errors;       // CRC校验错误计数
    uint32_t timeout_errors;   // 超时错误计数
    uint32_t protocol_errors;  // 协议错误计数
    uint32_t total_rx_frames;  // 总接收帧数
    uint32_t total_tx_frames;  // 总发送帧数
} can_comm_stats_t;
```

## 模块依赖

| 模块 | 说明 |
|------|------|
| `protocol_parser` | 协议解析器，用于帧解析 |
| `daemon` | 守护进程，用于离线检测 |
| `kfifo` | FIFO缓冲区，用于数据缓存 |
| `hal_fdcan` | FDCAN硬件抽象层 |
| `memory_pool` | 内存池，用于动态分配 |
| `crc` | CRC校验模块 |

## 注意事项

1. **初始化顺序**：必须先调用 `daemon_init()`，再调用 `can_comm_init()`
2. **主循环调用**：`can_comm_process_rx()`、`can_comm_package_tx()`、`can_comm_flush_tx()` 需要在主循环中周期性调用
3. **离线检测**：通过 `daemon_is_online(instance->daemon)` 检查通信是否在线
4. **内存管理**：模块内部使用 `memory_pool` 进行动态内存分配，确保内存池已初始化

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| V2.0.0 | 2025-04-05 | 重构：集成 protocol_parser，规范命名，分离配置与状态 |
| V1.0.0 | 2024-xx-xx | 初始版本 |
