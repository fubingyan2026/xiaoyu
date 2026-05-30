# HAL Drivers 编程规范

## 概述

本文档定义 `hal_drivers/` 目录下所有代码的编程规范，所有规则均来源于实际代码的通用模式。**所有新增或修改的代码必须严格遵循本规范**。

---

## 核心参考标准

### 1. 参考代码文件

以下文件是本规范的完整实现示例，**新增代码必须严格参考这些文件的风格和架构**：

| 文件                                   | 说明               |
| -------------------------------------- | ------------------ |
| [hal_gpio.h](./hal_gpio.h)             | 通用层头文件示例   |
| [hal_gpio.c](./hal_gpio.c)             | 通用层实现示例     |
| [stm32_hal_gpio.c](./stm32_hal_gpio.c) | STM32 平台实现示例 |

### 2. 遵循的规范

- **WebKit 代码风格指南**：遵循 [WebKit Code Style Guidelines](https://webkit.org/code-style-guidelines/)，包括缩进、大括号位置、命名、注释等格式化规范

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

/* Exported types ------------------------------------------------------------*/

// 类型定义...

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

// 函数声明...

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

### 通用层 .h 文件顺序

```c
// 1. 文件头注释
// 2. 头文件包含 (/* Includes ------------------------------------------------------------------*/)
// 3. 导出类型 (/* Exported types ------------------------------------------------------------*/)
// 4. 导出常量 (/* Exported constants --------------------------------------------------------*/)
// 5. 导出宏 (/* Exported macro ------------------------------------------------------------*/)
// 6. 导出函数原型 (/* Exported functions prototypes ---------------------------------------------*/)
```

### 通用层 .c 文件顺序

```c
// 1. 文件头注释
// 2. 头文件包含 (/* Includes ------------------------------------------------------------------*/)
// 3. 私有函数原型 (/* Private function prototypes -----------------------------------------------*/)
// 4. 私有函数 - 小型内联函数 (/* Private functions ---------------------------------------------------------*/)
// 5. 私有变量 (/* Private variables ---------------------------------------------------------*/)
// 6. 私有函数 - 其余实现 (/* Private functions ---------------------------------------------------------*/)
// 7. 导出函数 (/* Exported functions --------------------------------------------------------*/)
```

**注意**：私有内联辅助函数（如 `is_valid_xxx`、`ensure_ops`）放在文件前部，调用它们的导出函数放在后部，避免额外的前向声明。

### 平台层 .c 文件顺序

```c
// 1. 文件头注释
// 2. 头文件包含 (/* Includes ------------------------------------------------------------------*/)
// 3. 私有变量 (/* Private variables ---------------------------------------------------------*/)
// 4. 私有函数原型 (/* Private function prototypes -----------------------------------------------*/)
// 5. 操作函数结构体定义（static const，使用 designated initializers）
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
- 第一个值为 `HAL_XXX_OK = 0`
- 错误码以 `HAL_XXX_ERROR_` 开头
- 每个值都有详细的**中文**注释（使用 `/**< */` 格式）
- 枚举体左大括号与 `typedef enum` 同行（K&R 风格）

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
| 平台 ops 指针 | `g_hal_xxx_platform_ops` | `g_hal_gpio_platform_ops` |
| 常量/宏       | 大写蛇形命名 | `HAL_GPIO_PIN_MASK` |

---

## 注释规范

### 中文注释要求

**所有公共 API 和复杂逻辑必须使用中文注释**。

### 注释格式

- **函数文档**：使用 `/** */` 多行格式（Doxygen 风格）
- **结构体/枚举成员**：使用 `/**< */` 行尾格式
- **行内注释**：使用 `//` 格式

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

#### 结构体成员注释

```c
/**
 * @brief GPIO 配置结构体
 */
typedef struct {
    hal_gpio_port_t port;          /**< GPIO 端口 */
    hal_gpio_pin_t pin;            /**< GPIO 引脚号 */
    hal_gpio_mode_t mode;          /**< GPIO 工作模式 */
    hal_gpio_pin_state_t default_state; /**< 默认引脚状态（仅输出模式有效） */
    hal_gpio_pull_t pull;          /**< 上下拉配置 */
    hal_gpio_speed_t speed;        /**< 输出速度 */
    hal_gpio_af_t alternate;       /**< 复用功能选择 */
} hal_gpio_config_t;
```

#### 函数注释

```c
/**
 * @brief  初始化 GPIO 引脚
 * @param  ctx GPIO 上下文指针
 * @param  config GPIO 配置结构体指针
 * @return 操作结果错误码
 */
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t* ctx,
    const hal_gpio_config_t* config);
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
- 枚举体左大括号与 `typedef enum` 同行

### 指针 const 正确性

```c
// 输入参数，不修改内容
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t* ctx,
    const hal_gpio_config_t* config);

// 输出参数，用于返回结果
hal_gpio_error_t hal_gpio_read(hal_gpio_context_t* ctx,
    uint8_t port,
    uint8_t pin,
    hal_gpio_pin_state_t* state);
```

### switch-case 缩进

```c
switch (port) {
case HAL_GPIO_PORT_A:
    __HAL_RCC_GPIOA_CLK_ENABLE();
    break;
case HAL_GPIO_PORT_B:
    __HAL_RCC_GPIOB_CLK_ENABLE();
    break;
default:
    break;
}
```

**规则**：`case` 关键字与 `switch` 同级缩进，case 体内的语句再缩进 4 空格。

---

## 错误处理规范

### 参数检查

```c
// 检查参数有效性
if (ctx == NULL || config == NULL) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
}

// 检查端口和引脚号是否有效
if (!is_valid_port(config->port) || !is_valid_pin(config->pin)) {
    return HAL_GPIO_ERROR_INVALID_PARAM;
}
```

### 未初始化检查

```c
// 检查操作函数是否已设置
if (ctx->ops == NULL || ctx->ops->init == NULL) {
    return HAL_GPIO_ERROR_UNINITIALIZED;
}
```

### 未使用参数

```c
static hal_gpio_error_t stm32_gpio_write(hal_gpio_context_t* ctx,
    uint8_t port,
    uint8_t pin,
    hal_gpio_pin_state_t state)
{
    (void)ctx;  // 显式标记未使用的参数
    HAL_GPIO_WritePin(port_map[port], HAL_GPIO_PIN_MASK(pin), ...);
    return HAL_GPIO_OK;
}
```

**规则：**
- 使用 `(void)param` 显式标记未使用的参数
- 避免编译器警告 `-Wunused-parameter`
- `(void)param` 放在函数体的第一行位置

### 多条件提前返回

平台层函数中，简单的参数检查可以合并在一行提前返回：

```c
if (data == NULL || size == 0 || !validate_uart_instance(instance))
    return HAL_UART_ERROR_INVALID_PARAM;
```

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

**关键模式 - ensure_ops：**

每个通用层模块都实现一个 `ensure_ops` 内联函数，在首次调用时自动装配平台 ops：

```c
static inline void hal_gpio_ensure_ops(hal_gpio_context_t* ctx)
{
    if (ctx->ops == NULL && g_hal_gpio_platform_ops != NULL) {
        ctx->ops = g_hal_gpio_platform_ops;
    }
}
```

**关键模式 - register_platform_ops：**

```c
static const hal_gpio_ops_t* g_hal_gpio_platform_ops = NULL;

void hal_gpio_register_platform_ops(const hal_gpio_ops_t* ops)
{
    g_hal_gpio_platform_ops = ops;
}
```

### 平台层 (stm32_hal_*.c)

**职责：**
- 实现平台特定的硬件操作
- 包含平台头文件
- 调用 STM32 HAL 库

**不应该：**
- 修改通用接口
- 包含业务逻辑

**关键模式 - ops 结构体定义：**

平台层使用 designated initializers 定义 ops 结构体：

```c
/* GPIO操作函数结构体 */
static const hal_gpio_ops_t stm32_gpio_ops = {
    .init = stm32_gpio_init,
    .deinit = stm32_gpio_deinit,
    .write = stm32_gpio_write,
    .read = stm32_gpio_read,
    .toggle = stm32_gpio_toggle,
    .register_callback = stm32_gpio_register_callback,
};
```

**关键模式 - constructor 自动注册：**

```c
__attribute__((constructor)) static void _stm32_gpio_auto_register(void)
{
    hal_gpio_register_platform_ops(&stm32_gpio_ops);
}
```

此函数在 `main()` 之前自动执行，使用者无需手动调用。

---

## 上下文设计规范

### 上下文结构体

```c
typedef struct hal_xxx_context hal_xxx_context_t;

struct hal_xxx_context {
    const struct hal_xxx_ops* ops;  /**< 平台特定的操作函数指针 */
    volatile uint8_t initialized;    /**< 初始化标志（0=未初始化，1=已初始化） */
    hal_xxx_callback_t callback;     /**< 回调函数指针 */
    void* user_data;                 /**< 用户自定义数据 */
};
```

**规则：**
- 使用前向声明 `typedef struct hal_xxx_context hal_xxx_context_t;`
- 第一个成员是 `ops` 指针（const，平台层设置）
- 第二个成员是 `volatile uint8_t initialized`
- 使用 `void* user_data` 存储用户数据
- 所有成员都有中文 `/**< */` 注释
- 支持多实例

### ops 操作函数结构体

```c
typedef struct hal_xxx_ops {
    hal_xxx_error_t (*init)(hal_xxx_context_t* ctx,
        const hal_xxx_config_t* config);
    hal_xxx_error_t (*deinit)(hal_xxx_context_t* ctx,
        hal_xxx_instance_t instance);
    // ... 其他操作函数
    void (*irq_handler)(hal_xxx_context_t* ctx,
        hal_xxx_instance_t instance);
} hal_xxx_ops_t;
```

**规则：**
- 所有函数指针第一个参数是 `hal_xxx_context_t* ctx`
- 使用 `const` 修饰只读参数
- 每个函数指针有完整的中文 Doxygen 注释

---

## 代码格式规范

本模块遵循 **WebKit 代码风格**，与项目根目录 `MODULE_CODING_GUIDE.md` 保持一致。

### 缩进
- 使用 **4 空格缩进**
- 不要使用 Tab

### 大括号位置

- **函数定义**：左大括号**另起一行**（Allman 风格）
- **控制语句**（if/for/while/switch）：左大括号**与关键字同行**（K&R 风格）
- **结构体/枚举定义**：左大括号与类型关键字同行

```c
// 函数定义（Allman 风格）
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t* ctx,
    const hal_gpio_config_t* config)
{
    // ...
}

// 控制语句（K&R 风格）
if (condition) {
    // ...
} else {
    // ...
}

for (uint32_t i = 0; i < count; i++) {
    // ...
}

while (condition) {
    // ...
}

// 结构体/枚举（K&R 风格）
typedef struct {
    int member;
} struct_name_t;

typedef enum {
    VALUE_A = 0,
    VALUE_B,
} enum_name_t;
```

### 行长
- 单行不超过 **100 字符**
- 超长行需要换行并对齐

### 函数声明换行

```c
// 参数较多时换行，参数对齐 4 空格
hal_gpio_error_t hal_gpio_init(hal_gpio_context_t* ctx,
    const hal_gpio_config_t* config);

// 参数进一步换行时也保持 4 空格缩进
hal_gpio_error_t hal_gpio_register_callback(hal_gpio_context_t* ctx,
    uint8_t port, uint8_t pin,
    hal_gpio_callback_t callback,
    void* user_data);
```

### 指针声明
- 星号紧贴类型：`hal_gpio_context_t* ctx`，而不是 `hal_gpio_context_t *ctx`
- `const` 修饰目标类型：`const hal_gpio_config_t* config`

### switch-case 格式

```c
switch (instance) {
case HAL_UART_INSTANCE_1:
    __HAL_RCC_USART1_CLK_ENABLE();
    break;
case HAL_UART_INSTANCE_2:
    __HAL_RCC_USART2_CLK_ENABLE();
    break;
default:
    break;
}
```

### 注释后空行

在 `/* ---- */` 分隔注释和代码之间留一个空行：

```c
/* Private variables ---------------------------------------------------------*/

static const hal_gpio_ops_t* g_hal_gpio_platform_ops = NULL;

/* Private functions ---------------------------------------------------------*/

static inline void hal_gpio_ensure_ops(hal_gpio_context_t* ctx)
...
```

---

## 检查清单

创建或修改 HAL 模块时，请确认：

- [ ] 严格参考 hal_gpio.h、hal_gpio.c、stm32_hal_gpio.c 的风格
- [ ] 文件头注释完整
- [ ] 命名符合规范（前缀、蛇形命名）
- [ ] 所有公共 API 有中文注释
- [ ] 参数验证完整（检查 NULL、范围、有效性）
- [ ] 错误码返回正确（OK=0，错误为负枚举值）
- [ ] 未使用参数用 `(void)` 标记
- [ ] 分层设计清晰（通用层不碰寄存器，平台层不含业务逻辑）
- [ ] 实现了 `ensure_ops` 模式
- [ ] 平台层使用 `constructor` 自动注册
- [ ] ops 结构体使用 designated initializers
- [ ] 代码格式符合要求（4 空格缩进，Allman 函数大括号，指针紧贴类型）
- [ ] 语法检查通过（无错误、无警告）

---

## 总结

**关键要点：**
1. **必须参考** hal_gpio.h、hal_gpio.c、stm32_hal_gpio.c 这三个文件
2. **必须遵循** WebKit 代码风格指南（4 空格缩进、Allman 函数大括号、K&R 控制语句大括号、`type*` 指针声明）
3. **必须使用** 中文注释（公共 API 和复杂逻辑）
4. **必须实现** ensure_ops / constructor 自动注册 / designated initializers 等通用模式
5. **未提及的** 参考 WebKit Code Style Guidelines 与项目 MODULE_CODING_GUIDE.md

---

**遵循本规范，保持代码一致性和可维护性！**
