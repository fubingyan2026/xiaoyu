# 通用有限状态机框架

轻量级、可复用的有限状态机（FSM）框架，从FOC电机控制系统提取。专为嵌入式系统和通用应用设计。

## 功能特性

✅ **灵活的架构**
- 用户自定义状态和转换
- 状态处理器函数
- 可选的转换条件
- 用户上下文数据支持

✅ **回调系统**
- 全局进入/退出回调
- 每个状态的初始化/清理

✅ **验证与安全性**
- 转换验证
- 无效状态检测
- 错误处理

✅ **调试支持**
- 状态名称映射
- 状态变化跟踪
- 可配置的日志

✅ **内存效率**
- 可配置的状态/转换限制
- 可禁用可选功能
- 无动态内存分配

## 快速开始

### 1. 包含框架

```c
#include "fsm.h"
```

### 2. 定义状态

```c
typedef enum {
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_STOPPED,
    STATE_COUNT
} my_state_e;
```

### 3. 创建上下文和用户数据

```c
fsm_context_t fsm;

typedef struct {
    int counter;
    bool enabled;
} my_data_t;

my_data_t user_data = {0};
```

### 4. 实现状态处理器

```c
static fsm_state_t handler_idle(fsm_context_t *ctx)
{
    my_data_t *data = (my_data_t *)fsm_get_user_data(ctx);
    
    if (data->enabled) {
        return STATE_RUNNING;
    }
    return STATE_IDLE;
}

static fsm_state_t handler_running(fsm_context_t *ctx)
{
    my_data_t *data = (my_data_t *)fsm_get_user_data(ctx);
    
    data->counter++;
    
    if (!data->enabled) {
        return STATE_STOPPED;
    }
    return STATE_RUNNING;
}
```

### 5. 初始化和配置

```c
/* 初始化FSM */
fsm_init(&fsm, STATE_IDLE, &user_data);

/* 注册状态处理器 */
fsm_register_handler(&fsm, STATE_IDLE, handler_idle);
fsm_register_handler(&fsm, STATE_RUNNING, handler_running);

/* 添加允许的转换 */
fsm_add_transition(&fsm, STATE_IDLE, STATE_RUNNING, NULL);
fsm_add_transition(&fsm, STATE_RUNNING, STATE_STOPPED, NULL);
fsm_add_transition(&fsm, STATE_STOPPED, STATE_IDLE, NULL);

/* 设置回调（可选） */
fsm_set_callbacks(&fsm, on_entry_callback, on_exit_callback);

/* 设置调试用状态名称（可选） */
const char *state_names[] = {"IDLE", "RUNNING", "STOPPED"};
fsm_set_state_names(&fsm, state_names, STATE_COUNT);
```

### 6. 运行状态机

```c
while (1) {
    /* 执行一个FSM步骤 */
    fsm_step(&fsm);
    
    /* 其他代码... */
}
```

## API参考

### 初始化

#### `fsm_init()`
```c
fsm_ret_t fsm_init(fsm_context_t *ctx, fsm_state_t initial_state, void *user_data);
```
使用初始状态和可选的用户数据初始化状态机。

### 配置

#### `fsm_register_handler()`
```c
fsm_ret_t fsm_register_handler(fsm_context_t *ctx, fsm_state_t state, fsm_handler_t handler);
```
为特定状态注册处理器函数。

#### `fsm_add_transition()`
```c
fsm_ret_t fsm_add_transition(fsm_context_t *ctx, fsm_state_t from_state, 
                              fsm_state_t to_state, fsm_condition_t condition);
```
添加有效的状态转换。可选的条件函数可以控制转换的触发。

#### `fsm_set_callbacks()`
```c
fsm_ret_t fsm_set_callbacks(fsm_context_t *ctx, fsm_on_entry_t on_entry, fsm_on_exit_t on_exit);
```
设置全局进入/退出回调函数。

#### `fsm_set_state_names()`
```c
fsm_ret_t fsm_set_state_names(fsm_context_t *ctx, const char **state_names, uint8_t state_count);
```
设置用于调试的状态名称字符串。

### 运行时

#### `fsm_step()`
```c
fsm_ret_t fsm_step(fsm_context_t *ctx);
```
执行一个状态机周期。定期调用（例如在主循环或定时器中断中）。

#### `fsm_request_transition()`
```c
fsm_ret_t fsm_request_transition(fsm_context_t *ctx, fsm_state_t target_state);
```
请求立即进行状态转换。在应用前验证转换的有效性。

### 查询

#### `fsm_get_current_state()`
```c
fsm_state_t fsm_get_current_state(const fsm_context_t *ctx);
```
获取当前状态ID。

#### `fsm_get_last_state()`
```c
fsm_state_t fsm_get_last_state(const fsm_context_t *ctx);
```
获取前一个状态ID。

#### `fsm_is_state_changed()`
```c
bool fsm_is_state_changed(const fsm_context_t *ctx);
```
检查上一次 `fsm_step()` 调用中状态是否发生变化。

#### `fsm_get_state_name()`
```c
const char *fsm_get_state_name(const fsm_context_t *ctx, fsm_state_t state);
```
获取状态的名称字符串（用于调试）。

## 配置

编辑 `fsm_config.h` 自定义框架：

### 内存配置

```c
#define FSM_MAX_STATES       (16U)   /* 最大状态数 */
#define FSM_MAX_TRANSITIONS  (32U)   /* 最大转换规则数 */
```

### 功能开关

```c
#define FSM_ENABLE_DEBUG      (1)    /* 启用调试功能 */
#define FSM_ENABLE_CALLBACKS  (1)    /* 启用回调 */
```

### 优化配置

使用预定义的配置适应常见场景：

```c
#define FSM_PROFILE_MINIMAL      /* 最小占用空间 */
#define FSM_PROFILE_EMBEDDED     /* 平衡配置 */
#define FSM_PROFILE_FULL         /* 完整功能 */
```

## 高级用法

### 转换条件

添加逻辑来控制转换：

```c
static bool can_transition_to_running(const fsm_context_t *ctx)
{
    my_data_t *data = (my_data_t *)fsm_get_user_data(ctx);
    return data->initialized && !data->error;
}

fsm_add_transition(&fsm, STATE_IDLE, STATE_RUNNING, can_transition_to_running);
```

### 进入/退出回调

在进入/退出状态时执行操作：

```c
static void on_entry(fsm_context_t *ctx, fsm_state_t state)
{
    printf("进入状态: %s\n", fsm_get_state_name(ctx, state));
    
    switch (state) {
        case STATE_RUNNING:
            start_motor();
            break;
        case STATE_STOPPED:
            stop_motor();
            break;
    }
}

fsm_set_callbacks(&fsm, on_entry, NULL);
```

## 示例

框架包含完整的示例：

1. **fsm_example.c** - 交通灯控制器
   - 基于定时器的简单状态机转换
   - 紧急停止处理
   - 演示基本FSM概念

2. **motor_fsm_example.c** - 电机控制
   - 具有对准、运行、制动功能的复杂状态机
   - 转换条件
   - 错误处理
   - 基于真实的FOC电机控制

### 构建示例

```bash
# 交通灯示例
gcc -o traffic_example fsm.c fsm_example.c -I.

# 电机控制示例
gcc -o motor_example fsm.c motor_fsm_example.c -I.
```

## 设计理念

本框架基于以下原则：

1. **简单性** - 易于理解和使用
2. **灵活性** - 适应不同的使用场景
3. **效率** - 最小开销，适合嵌入式系统
4. **安全性** - 验证转换，防止无效状态
5. **可调试性** - 内置状态跟踪和日志支持

## 使用场景

- 电机控制系统
- 协议状态机
- UI流程控制
- 设备操作模式
- 通信协议
- 游戏状态管理
- 机器人行为控制
- 流程自动化

## 内存占用

典型内存使用（32位ARM）：

| 组件 | 大小 |
|-----------|------|
| 上下文结构 | ~96字节（默认配置） |
| 状态处理器 | 4字节 × 状态数 |
| 转换表 | 6字节 × 转换数 |
| 调试字符串 | 4字节 + 字符串存储 |

**示例:** 具有8个状态、16个转换、启用调试的FSM ≈ 200字节RAM

## 线程安全性

框架默认**不是线程安全的**。如果在多线程环境中使用：

1. 使用互斥锁保护 `fsm_step()` 和 `fsm_request_transition()`
2. 或者仅从单个线程调用FSM函数
3. 或者在配置中启用 `FSM_ENABLE_THREAD_SAFETY`（实验性）

## 从原始FOC代码迁移

迁移现有的FOC代码：

1. 将 `foc_sm_context_t` 替换为 `fsm_context_t`
2. 将 `foc_sm_state_e` 替换为您的状态枚举
3. 更新处理器签名以匹配 `fsm_handler_t`
4. 将 `foc_sm_init()` 替换为 `fsm_init()`
5. 将 `foc_sm_step()` 替换为 `fsm_step()`
6. 更新转换验证以使用 `fsm_add_transition()`

## 许可证

本框架按原样提供，可用于任何项目。

## 贡献

欢迎贡献！请确保：
- 代码遵循现有风格
- 更改已记录文档
- 如果API更改请更新示例

## 支持

如有疑问或问题，请参考示例代码或在仓库中创建问题。

---

**版本:** 1.0.0  
**日期:** 2026-02-03  
**提取自:** FOC电机控制系统
