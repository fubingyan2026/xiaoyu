# usart_protocol 模块重构计划

## 概述

按照 `protocol_parser/MODULE_CODING_GUIDE.md` 规范，重构 `usart_protocol.h` 和 `usart_protocol.c` 文件。

---

## 现状分析

### 当前代码问题

| 问题类型 | 现状 | 规范要求 |
|---------|------|---------|
| 文件头注释 | 使用 `/** */` 格式 | 使用 `//` 格式 |
| 守卫宏 | `USART_PROTOCOL_H` | `__USART_PROTOCOL_H` |
| 错误码枚举 | `ProtocolError_e` 后缀 | 应为 `usart_protocol_error_t` |
| 枚举值命名 | `ERR_NONE` 无模块前缀 | `USART_PROTOCOL_OK` 带模块前缀 |
| 配置结构体 | 无 | 需要添加 `usart_protocol_config_t` |
| 上下文结构体 | 无 | 需要添加 `usart_protocol_context_t` |
| 静态变量 | 使用全局静态变量 | 应封装到上下文结构体 |
| 初始化函数 | 无参数，无返回值 | 应接受 config 和 ctx 参数，返回错误码 |
| 代码组织 | 基本符合 | 需要按规范顺序调整 |
| 注释语言 | 已使用中文 | 符合规范 |

---

## 重构步骤

### 第一步：重构头文件 (usart_protocol.h)

#### 1.1 修改文件头注释格式
- 将 `/** */` 格式改为 `//` 格式
- 添加创建者和日期信息

#### 1.2 修改守卫宏
- 将 `#ifndef USART_PROTOCOL_H` 改为 `#ifndef __USART_PROTOCOL_H`
- 将 `#define USART_PROTOCOL_H` 改为 `#define __USART_PROTOCOL_H`

#### 1.3 重构错误码枚举
```c
// 原代码
typedef enum {
  ERR_NONE = 0x00U,
  ERR_GENERAL = 0x01U,
  ...
} ProtocolError_e;

// 重构后
typedef enum {
  USART_PROTOCOL_OK = 0,                   /**< 操作成功 */
  USART_PROTOCOL_ERROR_GENERAL,            /**< 通用错误 */
  USART_PROTOCOL_ERROR_INVALID_CMD,        /**< 无效命令 */
  USART_PROTOCOL_ERROR_INVALID_PARAM,      /**< 无效参数 */
  ...
} usart_protocol_error_t;
```

#### 1.4 重构其他枚举类型
- `ProtocolErrorClass_e` → `usart_protocol_error_class_t`
- `CalibStatus_e` → `usart_protocol_calib_status_t`
- `ParamType_e` → `usart_protocol_param_type_t`
- `StreamType_e` → `usart_protocol_stream_type_t`

#### 1.5 重构结构体类型命名
- `VersionInfo_t` → `usart_protocol_version_info_t`
- `StatusData_t` → `usart_protocol_status_data_t`
- `CalibStatusData_t` → `usart_protocol_calib_status_data_t`
- `PidParam_t` → `usart_protocol_pid_param_t`
- `StreamData_t` → `usart_protocol_stream_data_t`
- `StreamConfig_t` → `usart_protocol_stream_config_t`

#### 1.6 添加配置结构体
```c
/**
 * @brief 串口协议模块配置结构体
 */
typedef struct {
  const char* name;                           /**< 模块名称 */
  uint8_t* output_buffer;                     /**< 输出缓冲区 */
  uint16_t output_buffer_len;                 /**< 输出缓冲区大小 */
  void* uart_ctx;                             /**< UART上下文指针 */
  uint8_t uart_instance;                      /**< UART实例号 */
} usart_protocol_config_t;
```

#### 1.7 添加上下文结构体
```c
typedef struct usart_protocol_context usart_protocol_context_t;

struct usart_protocol_context {
  usart_protocol_config_t config;             /**< 配置参数 */
  usart_protocol_stream_config_t stream_cfg;  /**< 流配置 */
  uint32_t stream_last_tick;                  /**< 流上次发送时间 */
  bool initialized;                           /**< 初始化标志 */
};
```

#### 1.8 重构函数声明
```c
// 原代码
void usart_protocol_init(void);
void usart_protocol_process_frame(uint8_t* data, uint16_t len);

// 重构后
usart_protocol_error_t usart_protocol_init(
    usart_protocol_context_t* ctx,
    const usart_protocol_config_t* config);

void usart_protocol_deinit(usart_protocol_context_t* ctx);

bool usart_protocol_is_initialized(const usart_protocol_context_t* ctx);

usart_protocol_error_t usart_protocol_process_frame(
    usart_protocol_context_t* ctx,
    uint8_t* data,
    uint16_t len);
```

#### 1.9 调整代码组织顺序
按照规范顺序重新组织：
1. 文件头注释
2. 头文件包含 (`/* Includes ------------------------------------------------------------------*/`)
3. 导出类型 (`/* Exported types ------------------------------------------------------------*/`)
4. 导出常量 (`/* Exported constants --------------------------------------------------------*/`)
5. 导出宏 (`/* Exported macro ------------------------------------------------------------*/`)
6. 导出函数原型 (`/* Exported functions prototypes ---------------------------------------------*/`)

---

### 第二步：重构源文件 (usart_protocol.c)

#### 2.1 修改文件头注释格式
- 使用规范模板格式

#### 2.2 调整代码组织顺序
按照规范顺序重新组织：
1. 文件头注释
2. 头文件包含 (`/* Includes ------------------------------------------------------------------*/`)
3. 私有常量 (`/* Private constants ---------------------------------------------------------*/`)
4. 私有变量 (`/* Private variables ---------------------------------------------------------*/`)
5. 私有函数原型 (`/* Private function prototypes -----------------------------------------------*/`)
6. 导出函数 (`/* Exported functions --------------------------------------------------------*/`)
7. 私有函数 (`/* Private functions ---------------------------------------------------------*/`)

#### 2.3 移除全局静态变量
将以下静态变量移到上下文结构体：
- `s_stream_config`
- `s_stream_last_tick`

#### 2.4 重构初始化函数
```c
usart_protocol_error_t usart_protocol_init(
    usart_protocol_context_t* ctx,
    const usart_protocol_config_t* config) {
  // 1. 检查参数有效性
  if (!ctx || !config) {
    return USART_PROTOCOL_ERROR_NULL_PTR;
  }

  // 2. 检查配置有效性
  if (!config->output_buffer) {
    return USART_PROTOCOL_ERROR_NULL_PTR;
  }

  // 3. 如果已初始化，先反初始化
  if (ctx->initialized) {
    usart_protocol_deinit(ctx);
  }

  // 4. 保存配置
  ctx->config = *config;

  // 5. 初始化运行时状态
  ctx->stream_cfg.enable = 0U;
  ctx->stream_cfg.stream_type = 0U;
  ctx->stream_cfg.interval_ms = 10U;
  ctx->stream_last_tick = 0U;
  ctx->initialized = true;

  return USART_PROTOCOL_OK;
}
```

#### 2.5 添加反初始化函数
```c
void usart_protocol_deinit(usart_protocol_context_t* ctx) {
  if (!ctx) {
    return;
  }

  ctx->stream_cfg.enable = 0U;
  ctx->initialized = false;
}
```

#### 2.6 添加状态检查函数
```c
bool usart_protocol_is_initialized(const usart_protocol_context_t* ctx) {
  return (ctx && ctx->initialized);
}
```

#### 2.7 重构命令处理函数
- 将函数签名中的返回值类型改为 `usart_protocol_error_t`
- 更新错误码使用

#### 2.8 重构帧处理函数
- 添加上下文参数
- 使用上下文中的配置

#### 2.9 重构流任务函数
- 添加上下文参数
- 使用上下文中的流配置

---

### 第三步：更新调用方代码

需要更新以下文件中对 `usart_protocol` 的调用：
- `applications/app.c`
- `applications/usart_receive.c`

主要变更：
1. 创建上下文实例
2. 创建配置实例
3. 调用新的初始化函数
4. 传递上下文参数

---

## 详细变更清单

### usart_protocol.h 变更

| 行号 | 原代码 | 新代码 | 说明 |
|-----|-------|-------|------|
| 1-17 | `/** */` 格式注释 | `//` 格式注释 | 文件头格式 |
| 19 | `#ifndef USART_PROTOCOL_H` | `#ifndef __USART_PROTOCOL_H` | 守卫宏 |
| 20 | `#define USART_PROTOCOL_H` | `#define __USART_PROTOCOL_H` | 守卫宏 |
| 106-123 | `ProtocolError_e` | `usart_protocol_error_t` | 错误码枚举 |
| 128-134 | `ProtocolErrorClass_e` | `usart_protocol_error_class_t` | 错误类枚举 |
| 139-146 | `CalibStatus_e` | `usart_protocol_calib_status_t` | 校准状态枚举 |
| 151-159 | `ParamType_e` | `usart_protocol_param_type_t` | 参数类型枚举 |
| 164-170 | `StreamType_e` | `usart_protocol_stream_type_t` | 流类型枚举 |
| 180-187 | `VersionInfo_t` | `usart_protocol_version_info_t` | 版本信息结构体 |
| 193-203 | `StatusData_t` | `usart_protocol_status_data_t` | 状态数据结构体 |
| 209-213 | `CalibStatusData_t` | `usart_protocol_calib_status_data_t` | 校准状态数据结构体 |
| 220-225 | `PidParam_t` | `usart_protocol_pid_param_t` | PID参数结构体 |
| 231-239 | `StreamData_t` | `usart_protocol_stream_data_t` | 流数据结构体 |
| 245-249 | `StreamConfig_t` | `usart_protocol_stream_config_t` | 流配置结构体 |
| 新增 | - | `usart_protocol_config_t` | 配置结构体 |
| 新增 | - | `usart_protocol_context_t` | 上下文结构体 |
| 260 | `void usart_protocol_init(void)` | `usart_protocol_error_t usart_protocol_init(...)` | 初始化函数 |
| 新增 | - | `void usart_protocol_deinit(...)` | 反初始化函数 |
| 新增 | - | `bool usart_protocol_is_initialized(...)` | 状态检查函数 |
| 269 | `void usart_protocol_process_frame(...)` | `usart_protocol_error_t usart_protocol_process_frame(...)` | 帧处理函数 |

### usart_protocol.c 变更

| 行号 | 原代码 | 新代码 | 说明 |
|-----|-------|-------|------|
| 1-17 | `/** */` 格式注释 | `//` 格式注释 | 文件头格式 |
| 36-42 | `static StreamConfig_t s_stream_config` | 移到上下文结构体 | 静态变量封装 |
| 44 | `static uint32_t s_stream_last_tick` | 移到上下文结构体 | 静态变量封装 |
| 722-727 | `void usart_protocol_init(void)` | 重构为带参数版本 | 初始化函数 |
| 新增 | - | `usart_protocol_deinit()` | 反初始化函数 |
| 新增 | - | `usart_protocol_is_initialized()` | 状态检查函数 |
| 735-781 | `void usart_protocol_process_frame(...)` | 添加上下文参数 | 帧处理函数 |
| 787-811 | `void usart_protocol_stream_task(void)` | 添加上下文参数 | 流任务函数 |

---

## 执行顺序

1. **重构 usart_protocol.h**
   - 修改文件头注释
   - 修改守卫宏
   - 重构枚举类型命名
   - 重构结构体类型命名
   - 添加配置结构体
   - 添加上下文结构体
   - 重构函数声明
   - 调整代码组织顺序

2. **重构 usart_protocol.c**
   - 修改文件头注释
   - 调整代码组织顺序
   - 移除全局静态变量
   - 重构初始化函数
   - 添加反初始化函数
   - 添加状态检查函数
   - 重构帧处理函数
   - 重构流任务函数
   - 重构命令处理函数

3. **更新调用方代码**
   - 更新 app.c
   - 更新 usart_receive.c

---

## 风险评估

| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
| API 变更导致编译错误 | 高 | 同时更新所有调用方 |
| 运行时行为变化 | 中 | 保持核心逻辑不变 |
| 内存布局变化 | 低 | 结构体大小基本不变 |

---

## 验证方法

1. 编译检查：确保无编译错误和警告
2. 功能测试：验证协议通信正常
3. 代码审查：确保符合规范要求
