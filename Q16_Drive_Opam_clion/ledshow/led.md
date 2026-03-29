# LED 控制模块 (ledshow)

<p align='right'>版本: 2.0.0 | 作者: fubingyan</p>

一个工业级的 LED 控制模块，支持 GPIO 常亮/闪烁、编码闪烁、呼吸灯模式。采用**异步命令队列**与**事件驱动**架构，能够平滑处理高频状态切换请求。

---

## 🚀 核心特性

- **异步命令架构** — 内置 `kfifo` 命令队列，所有的状态切换和参数修改均为异步执行，不会阻塞调用者。
- **共享队列集成** — 状态变更时自动向 `message_center` 发布事件，解耦 LED 表现与业务逻辑。
- **时间获取解耦** — 不依赖特定 BSP 函数，初始化时注入时间回调，内置 32 位时间溢出保护。
- **内存安全** — 完美支持**静态分配**与**动态分配**，反初始化时自动识别并安全回收资源。
- **硬件通用性** — 彻底消除硬编码，PWM 定时器实例与通道均可通过配置结构体动态指定。

---

## 🛠️ 配置结构体

### 1. LED 基础配置 (`led_config_t`)

| 字段 | 类型 | 说明 |
| :--- | :--- | :--- |
| `led_name` | `const char*` | LED 唯一名称，用于查找和消息中心话题名 |
| `port` / `pin` | `uint8_t` | GPIO 端口与引脚号 |
| `active_level` | `hal_gpio_pin_state_e` | 有效电平（`HAL_GPIO_PIN_SET` 或 `RESET`） |
| `init_state` | `led_state_t` | 注册后的初始状态（常亮/关闭/闪烁/呼吸） |
| `blink_interval_ms` | `uint16_t` | 闪烁模式下的翻转间隔 (ms) |
| `blink_counts` | `uint16_t` | 编码闪烁模式下，每一轮闪烁的次数 |
| `pwm` | `led_pwm_config_t` | 呼吸灯模式所需的定时器实例与通道 |

---

## 📖 使用指南

### 1. 初始化系统
在使用任何 LED 之前，必须先注入时间函数：

```c
#include "led.h"

// 注入系统毫秒计数函数
LedInit(HAL_GetTick);
```

### 2. 注册 LED 实例

**静态注册 (推荐):**
```c
static led_handle_t heart_led_h;
led_config_t cfg = {
    .led_name = "heartbeat",
    .port = HAL_GPIO_PORT_B,
    .pin = HAL_GPIO_PIN_0,
    .active_level = HAL_GPIO_PIN_RESET,
    .init_state = LED_STATE_BLINK_CODE,
    .blink_interval_ms = 500
};
LedRegisterStatic(&cfg, &heart_led_h);
```

**动态注册:**
```c
led_handle_t *led = LedRegister(&cfg);
```

### 3. 控制 LED

**普通状态切换:**
```c
LedSetState(&heart_led_h, LED_STATE_ON);  // 开启
LedSetState(&heart_led_h, LED_STATE_OFF); // 关闭
```

**触发编码闪烁事件:**
```c
// 闪烁 3 次，每次间隔 100ms，每轮间隔 1000ms
LedSetBlinkInterval(&heart_led_h, 100, 1000, 3);
```

---

## 🔔 消息中心集成

当 LED 状态发生变更时，模块会自动向 `message_center` 发布话题。其他模块可以订阅此话题来感知 LED 状态（如 UI 刷新或系统日志）。

- **话题名格式**: `LED_EVT_[led_name]`
- **消息内容**: `led_state_t` (4字节枚举)

**订阅示例:**
```c
Subscriber_t *led_sub = SubRegister("LED_EVT_heartbeat", sizeof(led_state_t));

led_state_t current_state;
if (SubGetMessage(led_sub, &current_state)) {
    // 处理 LED 状态变更事件
}
```

---

## 📊 API 参考

| 函数 | 功能说明 |
| :--- | :--- |
| `LedInit(cb)` | 初始化 LED 子系统，注入时间回调 |
| `LedRegisterStatic(cfg, h)` | 静态注册 LED，使用预分配内存 |
| `LedRegister(cfg)` | 动态注册 LED |
| `LedUnregister(name)` | 注销并释放指定名称的 LED 资源 |
| `LedSetState(h, state)` | 异步设置 LED 运行状态 |
| `LedSetBlinkInterval(...)` | 异步配置并触发编码闪烁事件 |
| `LedTaskRefresh()` | **核心任务**，需在主循环中高频调用 |
| `LedGetInstance(name)` | 根据名称获取 LED 句柄指针 |

---

## ⚠️ 注意事项

1. **刷新频率**: `LedTaskRefresh()` 的调用频率决定了闪烁和呼吸灯的精度，建议调用周期 $\le 10ms$。
2. **PWM 初始化**: 呼吸灯模式依赖外部已初始化好的 PWM 硬件，请确保在 `LedRegister` 前已调用相关的硬件驱动。
3. **FIFO 深度**: 默认命令队列深度为 4，若在极短时间内发送超过 4 条控制指令，旧指令将被覆盖。
