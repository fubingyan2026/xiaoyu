# daemon 守护进程模块

<p align='right'>重构版本: V2.0.0</p>

用于监测模块和应用运行情况的守护进程模块，类似于官方代码中的 detect task。当被监护的模块长时间未"喂狗"（调用 DaemonReload），则判定该模块离线，触发回调通知。

---

## 特性

- 支持 **静态分配** 和 **动态分配** 两种初始化方式，适用于各种嵌入式场景
- **时间获取函数解耦**：不依赖特定平台的 `millis()`，由用户在初始化时注入
- **时间溢出安全**：内置 32 位时间戳溢出处理（49.7 天回绕）
- **内存安全**：静态分配的实例在反初始化时不会被 `free()`
- **错误码设计**：通过 `owner_name` + 组合码机制实现模块标识，支持与业务层错误码无缝对接

---

## 初始化

**必须先调用初始化：**

```c
#include "daemon.h"

// 假设这是你平台获取毫秒的函数
uint32_t bsp_get_millis(void) {
    return millis(); // 或 HAL_GetTick() 等
}

// 在注册任何 daemon 前，首先初始化并注入时间函数
DaemonInit(bsp_get_millis);
```

---

## 配置结构体

```c
typedef struct
{
    uint16_t reload_time_out;  // 离线超时时间(ms), 0xFFFF或0表示永不超时
    uint16_t init_wait_time;  // 上线等待时间(ms)
    offline_callback callback; // 离线回调函数
    const char *owner_name;    // 拥有者名称（全局唯一，用于查找实例）
    void *owner_pointer;      // 指向拥有者的指针，回调时传递给用户
} daemon_init_config_t;
```

### 字段说明

| 字段 | 说明 |
|------|------|
| `reload_time_out` | 离线超时阈值（毫秒）。超时未喂狗则判定离线。0 或 0xFFFF 表示永不超时 |
| `init_wait_time` | 上线等待时间（毫秒）。注册后在此时间内不会触发离线判定 |
| `callback` | 离线回调函数，格式：`void callback(void *owner_pointer)` |
| `owner_name` | 实例名称，字符串，用于 `DaemonGetInstance()` 查找。**必须全局唯一** |
| `owner_pointer` | 用户自定义指针，回调时会原样传回，便于在回调中定位具体实例 |

---

## 注册与注销

### 静态初始化（推荐）

适用于不支持动态内存或希望精确控制内存的场景：

```c
// 1. 定义静态实例
static daemon_t motor_daemon;

// 2. 配置参数
daemon_init_config_t config = {
    .owner_name    = "motor_left",
    .reload_time_out = 1000,    // 1秒超时
    .init_wait_time = 500,       // 上线等待500ms
    .callback      = motor_offline_cb,
    .owner_pointer = &motor_left_instance,
};

// 3. 注册
daemon_error_e err = DaemonRegisterStatic(&config, &motor_daemon);
if (DAEMON_IS_ERR(err)) {
    // 错误处理
}
```

### 动态初始化（兼容旧 API）

```c
daemon_init_config_t config = {
    .owner_name    = "encoder_front",
    .reload_time_out = 500,
    .callback      = encoder_offline_cb,
    .owner_pointer = &encoder_front_instance,
};

daemon_t *daemon = DaemonRegister(&config);
if (daemon == NULL) {
    // 错误处理
}
```

### 注销

```c
DaemonUnregister("motor_left"); // 按名称注销
```

---

## 使用流程

```c
// ========== 初始化 ==========
DaemonInit(bsp_get_millis);

// ========== 注册守护进程 ==========
static daemon_t motor_daemon;
daemon_init_config_t motor_config = {
    .owner_name    = "motor_1",
    .reload_time_out = 1000,  // 1秒未喂狗则离线
    .init_wait_time = 500,    // 启动时等待500ms
    .callback      = motor_offline_handler,
    .owner_pointer = &motor_instance,
};
DaemonRegisterStatic(&motor_config, &motor_daemon);

// ========== 主循环中喂狗 ==========
void motor_task(void)
{
    if (receive_motor_data()) {
        DaemonReload(&motor_daemon); // 收到数据，喂狗
    }
}

// ========== 查询在线状态 ==========
if (DaemonIsOnline(&motor_daemon)) {
    // 模块在线
} else {
    // 模块离线
}
```

---

## 离线回调

当模块从在线变为离线（或反之）时，会触发回调：

```c
void motor_offline_handler(void *owner_pointer)
{
    // owner_pointer 就是注册时传入的 owner_pointer
    motor_t *motor = (motor_t *)owner_pointer;
    
    // 处理离线：如重启、清标志位等
    motor->online = 0;
    printf("Motor %s is offline!\n", DaemonGetName(&motor->daemon));
}
```

---

## API 参考

| 函数 | 说明 |
|------|------|
| `DaemonInit(get_time_cb)` | 初始化守护进程系统，**必须先调用** |
| `DaemonDeinit()` | 反初始化，释放动态实例 |
| `DaemonRegister(config)` | 动态注册，返回实例指针 |
| `DaemonRegisterStatic(config, instance)` | 静态注册，使用预分配内存 |
| `DaemonUnregister(name)` | 按名称注销实例 |
| `DaemonReload(daemon)` | 喂狗，更新心跳时间 |
| `DaemonIsOnline(daemon)` | 查询在线状态，返回 0/1 |
| `DaemonGetName(daemon)` | 获取实例名称 |
| `DaemonGetInstance(name)` | 按名称查找实例 |
| `DaemonTask()` | 主循环任务，需周期性调用 |
| `DaemonGetCount()` | 获取已注册实例数量 |

---

## 错误码

```c
typedef enum {
    DAEMON_OK = 0,
    DAEMON_OK_EXISTED = 1,
    DAEMON_ERR_INVALID_PARAM = -1,
    DAEMON_ERR_NO_MEMORY = -2,
    DAEMON_ERR_NOT_FOUND = -3,
    DAEMON_ERR_ALREADY_EXIST = -4,
    DAEMON_ERR_INTERNAL = -5,
} daemon_error_e;
```

辅助宏：
```c
DAEMON_IS_OK(err)  // 等于0或1时为真
DAEMON_IS_ERR(err) // 小于0时为真
```

---

## 实现细节

### 状态机

- **INIT_WAIT**：上线等待阶段，累计时间未超过 `init_wait_time` 前保持预上线状态
- **ONLINE**：正常在线，按固定间隔检查是否超时
- **OFFLINE**：超时离线，触发回调

### 时间溢出处理

使用安全时间差计算函数，防止 32 位毫秒计数器在约 49.7 天后溢出导致判定错误：

```c
static inline uint32_t daemon_time_diff(uint32_t new_time, uint32_t old_time)
{
    return (new_time >= old_time) ? (new_time - old_time) : (0xFFFFFFFF - old_time + new_time + 1);
}
```

### 稳定性检查

为避免任务调度频率波动导致的误判，稳定性检查使用动态阈值：

```c
uint32_t threshold = (diff_times > 0) ? (DAEMON_STABLE_TIMES_MS / diff_times) : DAEMON_STABLE_TIMES_MS;
if (threshold == 0) threshold = 1;
```
