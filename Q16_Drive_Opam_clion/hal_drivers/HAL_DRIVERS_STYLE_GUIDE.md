# HAL Drivers 编程规范

## 概述

本文档定义 `hal_drivers/` 目录下所有代码的编程规范。**所有新增或修改的代码必须严格遵循本规范**。

---

## 核心参考标准

### 1. 参考代码文件

以下三个文件是本规范的完整实现示例，**新增代码必须严格参考这些文件的风格和架构**：

| 文件                                   | 说明               |
| -------------------------------------- | ------------------ |
| [hal_gpio.h](./hal_gpio.h)             | 通用层头文件示例   |
| [hal_gpio.c](./hal_gpio.c)             | 通用层实现示例     |
| [stm32_hal_gpio.c](./stm32_hal_gpio.c) | STM32 平台实现示例 |

### 2. 遵循的规范

- **Google 嵌入式规范**：遵循 [Google Embedded Style Guide](https://google.github.io/styleguide/embedded.html)
- **Google C++ 风格指南（C 语言部分）**：遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- **项目 AGENTS.md 规范**：遵循项目根目录下的 [AGENTS.md](../AGENTS.md)

---

## 目录结构规范

```
hal_drivers/
├── hal_*.h              # 硬件抽象层头文件（通用层）
├── hal_*.c              # 硬件抽象层实现（通用层）
├── stm32_hal_*.c        # STM32 平台特定实现
└── HAL_DRIVERS_STYLE_GUIDE.md  # 本规范文档
```

---

## 文件头规范

### 头文件 (.h)

```c
//
// Created by fubingyan on 25-9-20.
//

#ifndef __HAL_XXX_H
#define __HAL_XXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
}
#endif

#endif /* __HAL_XXX_H */
```

### 源文件 (.c)

```c
//
// Created by fubingyan on 25-9-20.
//

/**
 * @file    hal_xxx.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-09-20
 * @brief   硬件抽象层 - XXX 实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "hal_xxx.h"
```

---

## 代码组织顺序

所有文件必须按以下顺序组织代码：

```c
// 1. 文件头注释
// 2. 头文件包含 (/* Includes ------------------------------------------------------------------*/)
// 3. 私有变量 (/* Private variables ---------------------------------------------------------*/)
// 4. 私有函数原型 (/* Private function prototypes -----------------------------------------------*/)
// 5. 操作函数结构体（如适用）
// 6. 导出函数 (/* Exported functions --------------------------------------------------------*/)
// 7. 私有函数 (/* Private functions ---------------------------------------------------------*/)
```

---

## 命名规范

### 类型定义

| 类型     | 命名规范                    | 示例                 |
| -------- | --------------------------- | -------------------- |
| 枚举     | 前缀 + 蛇形命名 (结尾为 _t) | `hal_xxx_error_t`    |
| 结构体   | 前缀 + 蛇形命名 (结尾为 _t) | `hal_xxx_config_t`   |
| 函数指针 | 前缀 + 蛇形命名             | `hal_xxx_callback_t` |

### 枚举值

```c
typedef enum {
  HAL_XXX_OK = 0,              /**< 操作成功 */
  HAL_XXX_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_XXX_ERROR_UNINITIALIZED, /**< 未初始化 */
} hal_xxx_error_t;
```

**规则：**
- 使用大写蛇形命名
- 带有前缀（如 `HAL_XXX_`）
- 每个值都有详细的**中文**注释

### 函数命名

| 函数类型     | 命名规范                 | 示例                    |
| ------------ | ------------------------ | ----------------------- |
| 通用层函数   | `hal_` + 模块名 + 动词   | `hal_gpio_init()`       |
| 平台层函数   | `stm32_` + 模块名 + 动词 | `stm32_gpio_init()`     |
| 静态辅助函数 | 模块名_ + 动词           | `stm32_gpio_get_mode()` |

### 变量命名

| 变量类型      | 命名规范     | 示例                |
| ------------- | ------------ | ------------------- |
| 局部变量      | 小写蛇形命名 | `pin_index`         |
| 全局/静态变量 | 小写蛇形命名 | `exti_callbacks`    |
| 常量/宏       | 大写蛇形命名 | `HAL_GPIO_PIN_MASK` |

---

## 注释规范

### 中文注释要求

**所有公共 API 和复杂逻辑必须使用中文注释**（符合 AGENTS.md 要求）。

#### 枚举注释

```c
/**
 * @brief GPIO 操作错误码枚举
 */
typedef enum {
  HAL_GPIO_OK = 0,              /**< 操作成功 */
  HAL_GPIO_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_GPIO_ERROR_UNINITIALIZED, /**< 未初始化 */
} hal_gpio_error_t;
```

#### 函数注释

```c
/**
 * @brief  初始化 GPIO 引脚
 * @param  ctx GPIO 上下文指针
 * @param  config GPIO 配置结构体指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t *ctx,
                               const hal_gpio_config_t *config);
```

#### 行内注释

```c
// 检查参数有效性
if (ctx == NULL || config == NULL) {
  return HAL_GPIO_ERROR_INVALID_PARAM;
}

// 进入临界区，调用平台特定的初始化函数
HAL_GPIO_ENTER_CRITICAL();
hal_gpio_error_t result = ctx->ops->init(ctx, config);
HAL_GPIO_EXIT_CRITICAL();
```

---

## 类型安全规范

### 枚举类型

```c
typedef enum __attribute__((packed)) {
  HAL_GPIO_PIN_RESET = 0,
  HAL_GPIO_PIN_SET = 1,
} hal_gpio_pin_state_t;
```

**规则：**
- 使用 `__attribute__((packed))` 减小枚举大小
- 显式指定第一个值
- 保持类型安全

### 指针 const 正确性

```c
// 输入参数，不修改内容
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t *ctx,
                               const hal_gpio_config_t *config);

// 输出参数，用于返回结果
hal_gpio_error_t hal_gpio_read(hal_gpio_context_t *ctx,
                               uint8_t port,
                               uint8_t pin,
                               hal_gpio_pin_state_t *state);
```

---

## 错误处理规范

### 未使用参数

```c
static hal_gpio_error_t stm32_gpio_write(hal_gpio_context_t *ctx,
                                         uint8_t port,
                                         uint8_t pin,
                                         hal_gpio_pin_state_t state) {
  (void)ctx;  // 显式标记未使用的参数
  HAL_GPIO_WritePin(port_map[port], HAL_GPIO_PIN_MASK(pin), ...);
  return HAL_GPIO_OK;
}
```

**规则：**
- 使用 `(void)param` 显式标记未使用的参数
- 避免编译器警告 `-Wunused-parameter`

---

## 分层设计规范

### 通用层 (hal_*.h/c)

**职责：**
- 定义通用接口和类型
- 参数验证
- 错误处理
- 线程安全保护
- 调用平台特定实现

**不应该：**
- 直接操作硬件寄存器
- 包含平台特定的头文件

### 平台层 (stm32_hal_*.c)

**职责：**
- 实现平台特定的硬件操作
- 包含平台头文件
- 调用 STM32 HAL 库

**不应该：**
- 修改通用接口
- 包含业务逻辑

---

## 上下文设计规范

### 上下文结构体

```c
typedef struct hal_xxx_context hal_xxx_context_t;

struct hal_xxx_context {
  const struct hal_xxx_ops *ops;  /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized;    /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_xxx_callback_t callback;     /**< 中断回调函数指针 */
  void *user_data;                  /**< 用户自定义数据 */
};
```

**规则：**
- 使用前向声明 `typedef struct hal_xxx_context hal_xxx_context_t;`
- 所有函数第一个参数是上下文指针
- 支持多实例

---

## 代码格式规范

### 缩进
- 使用 2 空格缩进
- 不要使用 Tab

### 大括号位置

```c
// 函数定义
hal_gpio_error_t hal_gpio_init(...) {
  // ...
}

// 控制语句
if (condition) {
  // ...
} else {
  // ...
}
```

### 行长
- 单行不超过 100 字符

---

## 检查清单

创建或修改 HAL 模块时，请确认：

- [ ] 严格参考 hal_gpio.h、hal_gpio.c、stm32_hal_gpio.c 的风格
- [ ] 文件头注释完整
- [ ] 命名符合规范
- [ ] 所有公共 API 有中文注释
- [ ] 参数验证完整
- [ ] 错误码返回正确
- [ ] 未使用参数用 `(void)` 标记
- [ ] 分层设计清晰
- [ ] 代码格式符合要求
- [ ] 语法检查通过（无错误、无警告）

---

## 总结

**关键要点：**
1. **必须参考** hal_gpio.h、hal_gpio.c、stm32_hal_gpio.c 这三个文件
2. **必须遵循** Google 嵌入式规范
3. **必须使用** 中文注释（公共 API 和复杂逻辑）
4. **必须保持** 代码风格一致性
5. **未提及的** 参考google嵌入式规范

---

**遵循本规范，保持代码一致性和可维护性！**
