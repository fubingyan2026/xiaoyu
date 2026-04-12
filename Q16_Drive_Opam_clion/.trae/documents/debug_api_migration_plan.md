# Debug 模块 API 迁移计划

## 概述

将工程中使用的旧 `debug/debug.h` API 替换为新重构的 ESP32 风格 debug 模块 API。

## API 映射表

| 旧 API | 新 API | 说明 |
|--------|--------|------|
| `BSP_Printf(fmt, ...)` | `DEBUG_LOGX(tag, fmt, ...)` | 需要添加 tag 参数 |
| `DEBUG_ERROR(...)` | `DEBUG_LOGE(tag, ...)` | 需要添加 tag 参数 |
| `DEBUG_WARN(...)` | `DEBUG_LOGW(tag, ...)` | 需要添加 tag 参数 |
| `DEBUG_INFO(...)` | `DEBUG_LOGI(tag, ...)` | 需要添加 tag 参数 |
| `DEBUG_DEBUG(...)` | `DEBUG_LOGD(tag, ...)` | 需要添加 tag 参数 |
| `DEBUG_TRACE(...)` | `DEBUG_LOGT(tag, ...)` | 需要添加 tag 参数 |
| `ASSERT(expr)` | `DEBUG_ASSERT(expr)` | 直接替换 |
| `FUNCTION_ENTER()` | `DEBUG_ENTER()` | 直接替换 |
| `FUNCTION_EXIT()` | `DEBUG_EXIT()` | 直接替换 |
| `FUNCTION_EXIT_VAL(val)` | `DEBUG_EXIT_VAL(val)` | 直接替换 |
| `debug_enable_color(enable)` | `debug_set_color_enable(enable)` | 函数名变更 |

## 需要修改的文件

### 1. 应用层文件 (applications/)
- `app.c` - 主应用，需要添加 debug 初始化
- `CAN_Server.c`
- `flash_task.c`
- `hall_adjustment.c`
- `key_menu.c`
- `MT6816.c`
- `WS2812_SPI.c`

### 2. 模块文件
- `led/led.c`
- `protocol_parser/protocol_parser.c`
- `can_comm/can_comm.c`
- `can_comm/can_comm_port.c`

### 3. FOC 核心文件 (Q16_FOC/)
- `foc_sm.c`
- `foc_ctrl_q16.c`

## 实施步骤

### 步骤 1: 修改头文件引用
将 `#include "debug/debug.h"` 改为 `#include "debug.h"`（本地模块）

### 步骤 2: 在 app.c 中添加 debug 模块初始化
```c
// 定义静态缓冲区
static uint8_t s_debug_tx_buffer[1024];

// 初始化配置
debug_config_t debug_config = {
  .name = "app",
  .tx_buffer = s_debug_tx_buffer,
  .tx_buffer_size = sizeof(s_debug_tx_buffer),
  .get_timestamp_cb = HAL_GetTick,
  .format_buffer_size = DEBUG_DEFAULT_FORMAT_BUFFER_SIZE,
  .default_level = DEBUG_LEVEL_INFO,
  .enable_color = true,
  .enable_timestamp = true,
};
debug_init(&debug_config);
```

### 步骤 3: 替换各文件中的 API 调用

#### 3.1 app.c
- `DEBUG_DEBUG("mycmd_fn called...")` -> `DEBUG_LOGD("app", "mycmd_fn called...")`
- `DEBUG_WARN("memory used...")` -> `DEBUG_LOGW("app", "memory used...")`

#### 3.2 foc_sm.c
- `DEBUG_DEBUG("状态机进入...")` -> `DEBUG_LOGD("foc_sm", "状态机进入...")`
- `DEBUG_INFO("正向校准...")` -> `DEBUG_LOGI("foc_sm", "正向校准...")`
- `DEBUG_ERROR("状态机设置...")` -> `DEBUG_LOGE("foc_sm", "状态机设置...")`

#### 3.3 其他文件
按照相同模式替换，tag 使用模块名或功能名

### 步骤 4: 处理串口发送逻辑
在串口发送中断或轮询中调用 `debug_tx_get()` 获取数据并发送

### 步骤 5: 编译验证
确保所有修改编译通过

## 注意事项

1. 新 API 需要 tag 参数，建议使用模块名作为 tag
2. 新 API 输出格式为 ESP32 风格：`I (1234) tag: message`
3. 不再需要 `\r\n` 后缀，新 API 自动添加
4. 颜色输出由模块自动管理，无需手动添加颜色代码
