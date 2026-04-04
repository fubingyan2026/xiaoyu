# LED 模块重构计划（修订版）

## 概述

按照 `hal_drivers/HAL_DRIVERS_STYLE_GUIDE.md` 规范重构 `led/` 目录下的代码，参考 `hal_gpio.h`、`hal_gpio.c` 的**风格和架构**，但保持 led 模块自己的命名特色，不使用 HAL/hal 前缀。

---

## 重构原则

1. **参考风格，不照搬前缀**：学习 hal_gpio 的代码组织、注释风格、类型定义方式，但保持 `led_` 前缀
2. **保持功能不变**：所有重构只涉及命名和风格，不改变任何业务逻辑
3. **保持框架不变**：不改变现有的分层结构
4. **中文注释**：所有公共 API 和复杂逻辑使用中文注释

---

## 重构范围

### 需要修改的文件

1. **led/led.h** - 头文件重构
2. **led/led.c** - 实现文件重构
3. **applications/key_menu.c** - 调用方更新
4. **applications/warning_task.c** - 调用方更新
5. **applications/app.c** - 调用方更新

---

## 详细重构步骤

### 第一阶段：头文件重构 (led.h)

#### 1.1 文件头注释规范化

**当前格式**：
```c
/**
 * @file led.h
 * @author fubingyan qq:3245784484
 * @brief LED控制模块头文件 (大厂重构版)
 * ...
 */
```

**目标格式**（参考 hal_gpio.h 风格）：
```c
//
// Created by fubingyan on 25-9-20.
//

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
...
```

#### 1.2 类型命名规范（保持 led_ 前缀）

| 类型 | 当前命名 | 目标命名 | 说明 |
|-----|---------|---------|------|
| 错误码枚举 | `led_error_t` | `led_error_t` | 保持不变 |
| LED状态枚举 | `led_state_t` | `led_state_t` | 保持不变 |
| 闪烁阶段枚举 | `blink_code_phase_t` | `led_blink_phase_t` | 添加 led_ 前缀保持一致性 |
| 配置结构体 | `led_config_t` | `led_config_t` | 保持不变 |
| 句柄结构体 | `led_handle_t` | `led_handle_t` | 保持不变 |
| 命令结构体 | `led_cmd_t` | `led_cmd_t` | 保持不变 |
| PWM配置结构体 | `led_pwm_config_t` | `led_pwm_config_t` | 保持不变 |
| 时间获取函数指针 | `led_get_time_func` | `led_get_time_func_t` | 添加 _t 后缀 |
| 状态变化回调 | `led_state_change_callback_t` | `led_state_change_callback_t` | 保持不变 |
| 闪烁阶段回调 | `led_blink_phase_callback_t` | `led_blink_phase_callback_t` | 保持不变 |
| GPIO边沿回调 | `led_gpio_edge_callback_t` | `led_gpio_edge_callback_t` | 保持不变 |

#### 1.3 枚举值命名规范

**原则**：使用大写蛇形命名，带有 `LED_` 前缀

| 当前命名 | 目标命名 | 说明 |
|---------|---------|------|
| `LED_OK` | `LED_OK` | 保持不变 |
| `LED_OK_EXISTED` | `LED_OK_EXISTED` | 保持不变 |
| `LED_ERR_INVALID_PARAM` | `LED_ERROR_INVALID_PARAM` | ERR → ERROR 保持一致性 |
| `LED_ERR_NO_MEMORY` | `LED_ERROR_NO_MEMORY` | ERR → ERROR |
| `LED_ERR_NOT_FOUND` | `LED_ERROR_NOT_FOUND` | ERR → ERROR |
| `LED_ERR_ALREADY_EXIST` | `LED_ERROR_ALREADY_EXIST` | ERR → ERROR |
| `LED_ERR_INTERNAL` | `LED_ERROR_INTERNAL` | ERR → ERROR |
| `LED_STATE_NONE` | `LED_STATE_NONE` | 保持不变 |
| `LED_STATE_OFF` | `LED_STATE_OFF` | 保持不变 |
| `LED_STATE_ON` | `LED_STATE_ON` | 保持不变 |
| `LED_STATE_BLINK_CODE` | `LED_STATE_BLINK_CODE` | 保持不变 |
| `LED_STATE_BREATHING` | `LED_STATE_BREATHING` | 保持不变 |
| `LED_STATE_MAX` | `LED_STATE_MAX` | 保持不变 |
| `BLINK_CODE_PHASE_BLINKING` | `LED_BLINK_PHASE_BLINKING` | 添加 LED_ 前缀 |
| `BLINK_CODE_PHASE_INTERVAL` | `LED_BLINK_PHASE_INTERVAL` | 添加 LED_ 前缀 |

#### 1.4 函数命名规范

**原则**：使用小写蛇形命名，`led_` 前缀 + 动词

| 当前命名 | 目标命名 | 说明 |
|---------|---------|------|
| `LedInit` | `led_init` | 驼峰 → 蛇形 |
| `LedDeinit` | `led_deinit` | 驼峰 → 蛇形 |
| `LedRegister` | `led_register` | 驼峰 → 蛇形 |
| `LedRegisterStatic` | `led_register_static` | 驼峰 → 蛇形 |
| `LedUnregister` | `led_unregister` | 驼峰 → 蛇形 |
| `LedSetState` | `led_set_state` | 驼峰 → 蛇形 |
| `LedSetBlinkInterval` | `led_set_blink_interval` | 驼峰 → 蛇形 |
| `LedGetInstance` | `led_get_instance` | 驼峰 → 蛇形 |
| `LedGetBlinkPhase` | `led_get_blink_phase` | 驼峰 → 蛇形 |
| `LedSetStateChangeCallback` | `led_set_state_change_callback` | 驼峰 → 蛇形 |
| `LedTaskRefresh` | `led_task_refresh` | 驼峰 → 蛇形 |

#### 1.5 宏定义规范

| 当前命名 | 目标命名 | 说明 |
|---------|---------|------|
| `LED_IS_OK` | `LED_IS_OK` | 保持不变 |
| `LED_IS_ERR` | `LED_IS_ERR` | 保持不变 |

#### 1.6 代码组织顺序

按照规范重新组织头文件：

```c
//
// Created by fubingyan on 25-9-20.
//

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
// 枚举定义
// 结构体定义
// 函数指针类型定义

/* Exported constants --------------------------------------------------------*/
// 宏常量定义

/* Exported macro ------------------------------------------------------------*/
// 宏函数定义

/* Exported functions prototypes ---------------------------------------------*/
// 函数声明

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
```

#### 1.7 注释规范化

**枚举注释示例**（参考 hal_gpio.h）：
```c
/**
 * @brief LED 操作错误码枚举
 */
typedef enum {
  LED_OK = 0,                   /**< 操作成功 */
  LED_OK_EXISTED = 1,           /**< 已存在 */
  LED_ERROR_INVALID_PARAM = -1, /**< 无效参数 */
  LED_ERROR_NO_MEMORY = -2,     /**< 内存不足 */
  LED_ERROR_NOT_FOUND = -3,     /**< 未找到 */
  LED_ERROR_ALREADY_EXIST = -4, /**< 已存在 */
  LED_ERROR_INTERNAL = -5,      /**< 内部错误 */
} led_error_t;
```

**函数注释示例**：
```c
/**
 * @brief  初始化 LED 子系统
 * @param  get_time_cb 毫秒级时间获取回调函数
 * @return 操作结果错误码
 */
led_error_t led_init(led_get_time_func_t get_time_cb);
```

---

### 第二阶段：实现文件重构 (led.c)

#### 2.1 文件头注释规范化

**目标格式**（参考 hal_gpio.c）：
```c
//
// Created by fubingyan on 25-9-20.
//
/**
 * @file    led.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2025-09-20
 * @brief   LED 控制模块实现
 * @attention
 *
 * Copyright (c) 2025 fubingyan.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */
```

#### 2.2 代码组织顺序调整

按照规范重新组织代码：

```c
/* Includes ------------------------------------------------------------------*/
#include "led.h"
// ... 其他头文件

/* Private macros ------------------------------------------------------------*/
// 私有宏定义

/* Private variables ---------------------------------------------------------*/
// 静态变量定义

/* Private function prototypes -----------------------------------------------*/
// 静态函数声明

/* Exported functions --------------------------------------------------------*/
// 公共函数实现

/* Private functions ---------------------------------------------------------*/
// 私有函数实现
```

#### 2.3 内部函数命名

保持现有的内部函数命名，仅调整不符合规范的：

| 当前命名 | 目标命名 | 说明 |
|---------|---------|------|
| `led_time_diff` | `led_time_diff` | 保持不变 |
| `get_times` | `led_get_time_now` | 添加 led_ 前缀 |
| `led_phys_write` | `led_phys_write` | 保持不变 |
| `led_phys_read` | `led_phys_read` | 保持不变 |
| `led_fsm_none_handler` | `led_fsm_none_handler` | 保持不变 |
| `process_led_cmds` | `led_process_cmds` | 添加 led_ 前缀 |
| `check_blink_phase_change` | `led_check_blink_phase_change` | 添加 led_ 前缀 |

#### 2.4 注释规范化

- 所有公共 API 函数添加完整的中文 Doxygen 注释
- 复杂逻辑添加中文行内注释
- 参数验证处添加注释说明

**示例**：
```c
/**
 * @brief  初始化 LED 子系统
 * @param  get_time_cb 毫秒级时间获取回调函数
 * @return 操作结果错误码
 */
led_error_t led_init(led_get_time_func_t get_time_cb) {
  // 检查参数有效性
  if (get_time_cb == NULL) {
    return LED_ERROR_INVALID_PARAM;
  }
  
  // 检查是否已初始化
  if (led_system_initialized) {
    return LED_OK_EXISTED;
  }
  
  // ... 其他实现
}
```

---

### 第三阶段：调用方更新

#### 3.1 applications/key_menu.c

需要更新的内容：

**类型名称**：
- `blink_code_phase_t` → `led_blink_phase_t`
- 枚举值 `BLINK_CODE_PHASE_*` → `LED_BLINK_PHASE_*`

**函数调用**：
- `LedInit(millis)` → `led_init(millis)`
- `LedRegister(&s_led_default_config)` → `led_register(&s_led_default_config)`
- `LedSetStateChangeCallback(...)` → `led_set_state_change_callback(...)`
- `LedSetBlinkInterval(...)` → `led_set_blink_interval(...)`
- `LedSetState(...)` → `led_set_state(...)`

#### 3.2 applications/warning_task.c

需要更新的内容：

**函数调用**：
- `LedGetInstance(LED_NAME_TAG)` → `led_get_instance(LED_NAME_TAG)`

#### 3.3 applications/app.c

需要更新的内容：

**函数调用**：
- `LedTaskRefresh()` → `led_task_refresh()`

---

## 命名对照表

### 枚举值对照表

| 旧命名 | 新命名 |
|-------|-------|
| `LED_ERR_INVALID_PARAM` | `LED_ERROR_INVALID_PARAM` |
| `LED_ERR_NO_MEMORY` | `LED_ERROR_NO_MEMORY` |
| `LED_ERR_NOT_FOUND` | `LED_ERROR_NOT_FOUND` |
| `LED_ERR_ALREADY_EXIST` | `LED_ERROR_ALREADY_EXIST` |
| `LED_ERR_INTERNAL` | `LED_ERROR_INTERNAL` |
| `BLINK_CODE_PHASE_BLINKING` | `LED_BLINK_PHASE_BLINKING` |
| `BLINK_CODE_PHASE_INTERVAL` | `LED_BLINK_PHASE_INTERVAL` |

### 函数对照表

| 旧命名 | 新命名 |
|-------|-------|
| `LedInit` | `led_init` |
| `LedDeinit` | `led_deinit` |
| `LedRegister` | `led_register` |
| `LedRegisterStatic` | `led_register_static` |
| `LedUnregister` | `led_unregister` |
| `LedSetState` | `led_set_state` |
| `LedSetBlinkInterval` | `led_set_blink_interval` |
| `LedGetInstance` | `led_get_instance` |
| `LedGetBlinkPhase` | `led_get_blink_phase` |
| `LedSetStateChangeCallback` | `led_set_state_change_callback` |
| `LedTaskRefresh` | `led_task_refresh` |

### 类型对照表

| 旧命名 | 新命名 |
|-------|-------|
| `blink_code_phase_t` | `led_blink_phase_t` |
| `led_get_time_func` | `led_get_time_func_t` |

---

## 验证步骤

重构完成后需要验证：

1. **编译验证**：确保代码能够正常编译，无错误无警告
2. **功能验证**：确保 LED 控制功能正常工作
   - LED 开/关功能
   - LED 闪烁功能
   - LED 呼吸灯功能
   - 按键菜单交互功能
3. **调用方验证**：确保所有调用方正确更新

---

## 执行顺序

1. 重构 led.h 头文件
2. 重构 led.c 实现文件
3. 更新 applications/key_menu.c
4. 更新 applications/warning_task.c
5. 更新 applications/app.c
6. 编译验证

---

## 重构要点总结

1. **文件头注释**：参考 hal_gpio.h/c 的简洁风格
2. **代码组织**：按照规范的顺序组织代码（Includes → Private macros → Private variables → Private function prototypes → Exported functions → Private functions）
3. **命名风格**：
   - 函数：小写蛇形，`led_` 前缀（如 `led_init`）
   - 枚举值：大写蛇形，`LED_` 前缀（如 `LED_OK`）
   - 类型：小写蛇形，`_t` 后缀（如 `led_error_t`）
4. **注释**：所有公共 API 和复杂逻辑使用中文注释
5. **错误码**：统一使用 `ERROR` 而非 `ERR`
6. **类型后缀**：函数指针类型添加 `_t` 后缀
