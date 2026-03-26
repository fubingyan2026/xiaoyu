# FOC驱动下位机串口通信协议设计方案

## 一、协议分析总结

### 1.1 上位机协议帧格式

| 字段 | 长度 | 说明 |
|------|------|------|
| 帧头 | 2 bytes | `0xA5 0x5A` |
| 命令码 | 1 byte | 命令类型 (0x01-0x83) |
| 数据长度 | 1 byte | Payload长度 (0-255) |
| 数据 | n bytes | 实际数据内容 |
| 校验和 | 1 byte | XOR校验 |
| 帧尾 | 2 bytes | `0x0D 0x0A` |

**校验和计算方法**:
```c
uint8_t checksum = cmd ^ len;
for (int i = 0; i < len; i++) {
    checksum ^= data[i];
}
```

### 1.2 命令列表

| 命令码 | 名称 | 方向 | 数据长度 |
|--------|------|------|----------|
| 0x01 | CMD_PING | PC→MCU | 0 |
| 0x02 | CMD_GET_VERSION | PC→MCU | 0 |
| 0x03 | CMD_RESET | PC→MCU | 0 |
| 0x10 | CMD_GET_STATUS | PC→MCU | 0 |
| 0x11 | CMD_GET_STATE | PC→MCU | 0 |
| 0x20 | CMD_SET_CURRENT | PC→MCU | 4 |
| 0x21 | CMD_SET_VELOCITY | PC→MCU | 2 |
| 0x22 | CMD_SET_POSITION | PC→MCU | 4 |
| 0x23 | CMD_STOP | PC→MCU | 0 |
| 0x30 | CMD_START_CALIB | PC→MCU | 0 |
| 0x31 | CMD_GET_CALIB_STATUS | PC→MCU | 0 |
| 0x32 | CMD_GET_CALIB_DATA | PC→MCU | 0 |
| 0x33 | CMD_SET_CALIB_DATA | PC→MCU | 变长 |
| 0x34 | CMD_CLEAR_CALIB | PC→MCU | 0 |
| 0x40 | CMD_GET_PARAMS | PC→MCU | 1 |
| 0x41 | CMD_SET_PARAMS | PC→MCU | 变长 |
| 0x50 | CMD_START_STREAM | PC→MCU | 1 |
| 0x51 | CMD_STOP_STREAM | PC→MCU | 0 |

### 1.3 响应命令

| 命令码 | 名称 | 方向 | 数据长度 |
|--------|------|------|----------|
| 0x80 | RESP_OK | MCU→PC | 0 |
| 0x81 | RESP_ERROR | MCU→PC | 1 |
| 0x82 | RESP_DATA | MCU→PC | 变长 |
| 0x83 | RESP_STREAM | MCU→PC | 16 |

---

## 二、下位机实现架构

### 2.1 模块结构

```
applications/
├── usart_protocol.c      # 协议核心处理
├── usart_protocol.h       # 协议头文件
├── usart_receive.c        # 已有: 串口接收初始化
└── CAN_Server.c           # 参考: CAN协议实现模式
```

### 2.2 设计模式

采用与 CAN_Server.c 相同的命令处理模式：
- **命令注册表**: 静态函数指针数组
- **命令分发**: switch-case 或函数指针查找
- **响应机制**: 主动发送响应帧

### 2.3 核心数据结构

```c
// 协议命令处理函数类型
typedef int (*protocol_cmd_handler_t)(uint8_t cmd, uint8_t *data, uint16_t len, uint8_t *response, uint16_t *resp_len);

// 命令注册表项
typedef struct {
    uint8_t cmd;                              // 命令码
    protocol_cmd_handler_t handler;           // 处理函数
    const char *name;                         // 命令名称(调试用)
} protocol_cmd_entry_t;

// 流数据配置
typedef struct {
    uint8_t enable;                           // 流使能
    uint8_t stream_type;                      // 流类型
    uint16_t interval_ms;                     // 流间隔(ms)
} protocol_stream_config_t;
```

---

## 三、命令处理函数设计

### 3.1 系统命令处理

| 命令 | 处理函数 | 说明 |
|------|----------|------|
| CMD_PING | cmd_ping_handler | 返回 RESP_OK |
| CMD_GET_VERSION | cmd_get_version_handler | 返回固件版本 |
| CMD_RESET | cmd_reset_handler | 系统复位 |

**版本响应格式** (RESP_DATA, 8 bytes):
```c
typedef struct {
    uint8_t protocol_ver;     // 协议版本
    uint8_t firmware_ver[3];  // 固件版本 (major.minor.patch)
    uint8_t hardware_ver;     // 硬件版本
    uint8_t reserve[3];       // 保留
} version_info_t;
```

### 3.2 状态查询命令处理

| 命令 | 处理函数 | 说明 |
|------|----------|------|
| CMD_GET_STATUS | cmd_get_status_handler | 获取运行状态 |
| CMD_GET_STATE | cmd_get_state_handler | 获取FOC状态机状态 |

**状态响应格式** (RESP_DATA, 15 bytes):
```c
typedef struct {
    uint8_t state;            // FOC状态机状态
    uint8_t error_code;       // 错误代码
    int16_t id_current;       // D轴电流 (mA)
    int16_t iq_current;       // Q轴电流 (mA)
    int16_t velocity;         // 速度 (rad/s × 1000)
    int32_t position;         // 位置 (rad × 10000)
    uint16_t voltage;         // 母线电压 (V × 100)
    int8_t temperature;       // 温度 (°C + 40)
} __attribute__((packed)) status_data_t;
```

### 3.3 控制命令处理

| 命令 | 处理函数 | 数据格式 |
|------|----------|----------|
| CMD_SET_CURRENT | cmd_set_current_handler | Id(int16), Iq(int16), 单位: 0.001A |
| CMD_SET_VELOCITY | cmd_set_velocity_handler | velocity(int16), 单位: 0.001 rad/s |
| CMD_SET_POSITION | cmd_set_position_handler | position(int32), 单位: 0.0001 rad |
| CMD_STOP | cmd_stop_handler | 无数据 |

**控制流程**:
1. 解析数据并验证范围
2. 调用 FOC 控制接口设置目标值
3. 返回 RESP_OK 或 RESP_ERROR

### 3.4 校准命令处理

| 命令 | 处理函数 | 说明 |
|------|----------|------|
| CMD_START_CALIB | cmd_start_calib_handler | 启动编码器校准 |
| CMD_GET_CALIB_STATUS | cmd_get_calib_status_handler | 获取校准进度 |
| CMD_GET_CALIB_DATA | cmd_get_calib_data_handler | 获取校准数据 |
| CMD_SET_CALIB_DATA | cmd_set_calib_data_handler | 写入校准数据 |
| CMD_CLEAR_CALIB | cmd_clear_calib_handler | 清除校准数据 |

**校准状态响应** (RESP_DATA, 3 bytes):
```c
typedef struct {
    uint8_t status;           // 校准状态 (0=IDLE, 1=RUNNING, 2=FORWARD, 3=REVERSE, 4=COMPLETE, 5=FAILED)
    uint8_t progress;         // 进度 (0-100)
    int8_t direction;          // 方向 (-1=反向, 1=正向)
} __attribute__((packed)) calib_status_t;
```

### 3.5 参数命令处理

| 命令 | 处理函数 | 说明 |
|------|----------|------|
| CMD_GET_PARAMS | cmd_get_params_handler | 获取PID/电机参数 |
| CMD_SET_PARAMS | cmd_set_params_handler | 设置PID/电机参数 |

**参数类型** (data[0]):
- 0x01: PID_CURRENT - 电流环PID
- 0x02: PID_VELOCITY - 速度环PID
- 0x03: PID_POSITION - 位置环PID
- 0x04: MOTOR_PARAMS - 电机参数
- 0x05: OBSERVER_PARAMS - 观测器参数

**PID参数格式** (16 bytes):
```c
typedef struct {
    float kp;                  // 比例增益
    float ki;                  // 积分增益
    float kd;                  // 微分增益
    float output_limit;        // 输出限制
} __attribute__((packed)) pid_param_t;
```

### 3.6 数据流命令处理

| 命令 | 处理函数 | 说明 |
|------|----------|------|
| CMD_START_STREAM | cmd_start_stream_handler | 启动数据流 |
| CMD_STOP_STREAM | cmd_stop_stream_handler | 停止数据流 |

**流数据格式** (RESP_STREAM, 16 bytes):
```c
typedef struct {
    uint32_t timestamp;        // 时间戳 (us)
    int16_t id_current;        // D轴电流 (mA)
    int16_t iq_current;        // Q轴电流 (mA)
    int16_t velocity;          // 速度 (rad/s × 1000)
    int32_t position;          // 位置 (rad × 10000)
    uint16_t elec_angle;       // 电角度 (0-65535 → 0-2π)
} __attribute__((packed)) stream_data_t;
```

---

## 四、响应帧构建

### 4.1 响应帧格式

使用与请求帧相同的格式，命令码替换为响应码：

```c
typedef struct {
    uint8_t head[2];           // 0xA5, 0x5A
    uint8_t cmd;               // 响应命令码
    uint8_t len;               // 数据长度
    uint8_t data[256];         // 响应数据
    uint8_t checksum;          // XOR校验
    uint8_t tail[2];           // 0x0D, 0x0A
} __attribute__((packed)) protocol_frame_t;
```

### 4.2 响应构建函数

```c
// 构建响应帧
static uint16_t protocol_build_response(uint8_t cmd, uint8_t *data, uint16_t len, uint8_t *output)
{
    uint8_t checksum = cmd ^ len;
    for (uint16_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    
    output[0] = 0xA5;
    output[1] = 0x5A;
    output[2] = cmd;
    output[3] = len;
    memcpy(&output[4], data, len);
    output[4 + len] = checksum;
    output[5 + len] = 0x0D;
    output[6 + len] = 0x0A;
    
    return 7 + len;  // 总帧长度
}

// 发送响应
static void protocol_send_response(uint8_t cmd, uint8_t *data, uint16_t len)
{
    static uint8_t resp_buffer[280];
    uint16_t frame_len = protocol_build_response(cmd, data, len, resp_buffer);
    // 通过UART发送
    hal_uart_send_dma(HAL_UART_INSTANCE_1, resp_buffer, frame_len);
}
```

---

## 五、错误处理

### 5.1 错误代码定义

| 错误码 | 名称 | 说明 |
|--------|------|------|
| 0 | ERR_NONE | 无错误 |
| 1 | ERR_OVERCURRENT | 过流保护 |
| 2 | ERR_OVERTEMP | 过温保护 |
| 3 | ERR_OVERVOLTAGE | 过压保护 |
| 4 | ERR_UNDERVOLTAGE | 欠压保护 |
| 5 | ERR_ENCODER | 编码器错误 |
| 6 | ERR_CALIB_FAILED | 校准失败 |
| 7 | ERR_INVALID_PARAM | 无效参数 |
| 8 | ERR_TIMEOUT | 超时 |

### 5.2 错误响应

```c
// 发送错误响应
static void protocol_send_error(uint8_t error_code)
{
    protocol_send_response(RESP_ERROR, &error_code, 1);
}
```

---

## 六、集成方案

### 6.1 文件结构

```
applications/
├── usart_protocol.c      # 协议处理实现
├── usart_protocol.h      # 协议头文件
│
├── usart_receive.c       # 已有: 串口接收 + init_a5_protocol()
│   - init_a5_protocol() # 初始化A5协议解析器
│   - Uart_Process_Task() # 处理接收数据
│
└── app.c                 # 主程序
    - AppInit()          # 初始化时调用协议初始化
```

### 6.2 初始化流程

```c
// usart_protocol.h
extern void usart_protocol_init(void);
extern void usart_protocol_process(uint8_t *data, uint16_t len);
extern void usart_protocol_stream_task(void);  // 流数据定时任务

// usart_protocol.c
void usart_protocol_init(void)
{
    // 初始化流配置
    stream_config.enable = 0;
    stream_config.stream_type = 0;
    stream_config.interval_ms = 10;  // 默认10    // 注册命令处理ms
    
函数
    protocol_register_commands();
}

// 修改 usart_receive.c 的 Uart_Process_Task
void Uart_Process_Task(void)
{
    // ... 现有代码 ...
    
    p = b_frame_check_get(&test_frame, &flen);
    if (p) {
        // 新增: 协议处理
        usart_protocol_process(p, flen);
        
        DEBUG_INFO("%s:", test_frame.frame_init.pname);
        for (uint16_t i = 0; i < flen; i++) {
            BSP_Printf(" 0x%02x", p[i]);
        }
        BSP_Printf("\r\n");
    }
    b_frame_idie_timer(&test_frame);
}
```

### 6.3 流数据任务

在定时器中断或任务中添加流数据发送：

```c
// 定时调用 (如10ms)
void usart_protocol_stream_task(void)
{
    if (!stream_config.enable) {
        return;
    }
    
    static uint32_t last_tick = 0;
    uint32_t current_tick = get_system_ticks();
    
    if ((current_tick - last_tick) < stream_config.interval_ms) {
        return;
    }
    last_tick = current_tick;
    
    // 构建流数据
    stream_data_t stream;
    stream.timestamp = get_timestamp_us();
    stream.id_current = foc_get_id_current();
    stream.iq_current = foc_get_iq_current();
    stream.velocity = foc_get_velocity();
    stream.position = foc_get_position();
    stream.elec_angle = foc_get_electrical_angle();
    
    // 发送流数据
    protocol_send_response(RESP_STREAM, (uint8_t *)&stream, sizeof(stream));
}
```

---

## 七、协议命令表

### 7.1 命令处理函数注册

```c
// 命令处理表
static const protocol_cmd_entry_t cmd_table[] = {
    {CMD_PING,             cmd_ping_handler,             "PING"},
    {CMD_GET_VERSION,      cmd_get_version_handler,      "GET_VERSION"},
    {CMD_RESET,            cmd_reset_handler,            "RESET"},
    {CMD_GET_STATUS,       cmd_get_status_handler,       "GET_STATUS"},
    {CMD_GET_STATE,        cmd_get_state_handler,        "GET_STATE"},
    {CMD_SET_CURRENT,      cmd_set_current_handler,      "SET_CURRENT"},
    {CMD_SET_VELOCITY,     cmd_set_velocity_handler,     "SET_VELOCITY"},
    {CMD_SET_POSITION,     cmd_set_position_handler,     "SET_POSITION"},
    {CMD_STOP,             cmd_stop_handler,             "STOP"},
    {CMD_START_CALIB,      cmd_start_calib_handler,      "START_CALIB"},
    {CMD_GET_CALIB_STATUS, cmd_get_calib_status_handler, "GET_CALIB_STATUS"},
    {CMD_GET_CALIB_DATA,   cmd_get_calib_data_handler,   "GET_CALIB_DATA"},
    {CMD_SET_CALIB_DATA,   cmd_set_calib_data_handler,   "SET_CALIB_DATA"},
    {CMD_CLEAR_CALIB,      cmd_clear_calib_handler,      "CLEAR_CALIB"},
    {CMD_GET_PARAMS,       cmd_get_params_handler,       "GET_PARAMS"},
    {CMD_SET_PARAMS,       cmd_set_params_handler,       "SET_PARAMS"},
    {CMD_START_STREAM,     cmd_start_stream_handler,     "START_STREAM"},
    {CMD_STOP_STREAM,      cmd_stop_stream_handler,      "STOP_STREAM"},
};

// 命令处理
static void usart_protocol_process(uint8_t *data, uint16_t len)
{
    if (len < 4) return;  // head(2) + cmd(1) + len(1)
    
    uint8_t cmd = data[2];
    uint8_t payload_len = data[3];
    uint8_t *payload = (len > 4) ? &data[4] : NULL;
    
    // 查找命令处理函数
    for (int i = 0; i < sizeof(cmd_table)/sizeof(cmd_table[0]); i++) {
        if (cmd_table[i].cmd == cmd) {
            uint8_t response[256];
            uint16_t resp_len = 0;
            
            int ret = cmd_table[i].handler(cmd, payload, payload_len, response, &resp_len);
            
            if (ret == 0) {
                protocol_send_response(RESP_OK, response, resp_len);
            } else {
                protocol_send_error((uint8_t)ret);
            }
            return;
        }
    }
    
    // 未找到命令
    protocol_send_error(ERR_INVALID_PARAM);
}
```

---

## 八、文件清单

### 8.1 新增文件

| 文件 | 说明 |
|------|------|
| `applications/usart_protocol.h` | 协议头文件 |
| `applications/usart_protocol.c` | 协议实现 |

### 8.2 修改文件

| 文件 | 修改内容 |
|------|----------|
| `applications/usart_receive.c` | 添加协议处理调用 |
| `applications/app.c` | 添加协议初始化 |

---

## 九、测试验证

### 9.1 通信测试用例

1. **PING测试**: 发送 `A5 5A 01 00 01 0D 0A`，应返回 `A5 5A 80 00 80 0D 0A`
2. **版本查询**: 发送 `A5 5A 02 00 02 0D 0A`，应返回版本信息
3. **状态查询**: 发送 `A5 5A 10 00 10 0D 0A`，应返回15字节状态数据
4. **设置电流**: 发送 `A5 5A 20 04 F4 01 E8 03 1F 0D 0A`，应返回成功响应

### 9.2 边界条件

- 空数据帧 (len=0)
- 最大数据帧 (len=255)
- 校验错误帧
- 帧头/帧尾错误帧

---

## 十、与大厂协议对比

| 特性 | 本协议 | 汇川 | 松下 | 大华 |
|------|--------|------|------|------|
| 帧头 | 0xA5 0x5A | 0x68 | 0x01 | 0xAA |
| 帧尾 | 0x0D 0x0A | 0x16 | 0x03 | 0x55AA |
| 校验 | XOR | CRC16 | BCC | CRC16 |
| 命令类型 | 1字节 | 1字节 | 1字节 | 2字节 |
| 响应机制 | 有 | 有 | 有 | 有 |
| 流数据 | 支持 | 支持 | 支持 | 支持 |

本协议参考业界标准，采用简洁的XOR校验（适合嵌入式资源受限场景）和标准帧格式，与主流PLC/伺服驱动协议兼容。
