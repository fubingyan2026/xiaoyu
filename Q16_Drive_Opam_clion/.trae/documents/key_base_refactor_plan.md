# key\_base 模块重构计划

## 概述

将 `c:\Users\fubingyan\Desktop\xiaoyu\common\keybase\key_base.c` 和 `key_base.h` 按照 `MODULE_CODING_GUIDE.md` 的规范重构到当前工作空间下，并更新所有使用的地方。

***

## 当前状态分析

### 原始文件位置

* 源文件：`c:\Users\fubingyan\Desktop\xiaoyu\common\keybase\key_base.c`

* 头文件：`c:\Users\fubingyan\Desktop\xiaoyu\common\keybase\key_base.h`

### 使用位置

1. `applications/app.h` - 包含 `#include "keybase/key_base.h"`
2. `applications/key_menu.h` - 包含 `#include "keybase/key_base.h"`
3. `applications/key_menu.c` - 使用 `key_base_t`, `key_config_t`, `KeyBaseRegister` 等

### 当前命名问题

| 当前命名              | 问题                          |
| ----------------- | --------------------------- |
| `KeyBaseInit`     | 驼峰命名，应为 `key_base_init`     |
| `KeyBaseDeinit`   | 驼峰命名，应为 `key_base_deinit`   |
| `KeyBaseRegister` | 驼峰命名，应为 `key_base_register` |
| `key_error_e`     | 后缀应为 `_t`                   |
| `key_event_e`     | 后缀应为 `_t`                   |
| `key_config_t`    | 命名正确                        |
| `key_base_t`      | 应为 `key_base_context_t`     |

### 结构体设计问题

当前 `key_base_t` 混合了配置和运行时状态，不符合规范的 config\_t/context\_t 分离设计。

***

## 重构目标

### 目标目录结构

```
Q16_Drive_Opam_clion/
└── modules/
    └── key_base/
        ├── key_base.h
        └── key_base.c
```

### 命名规范

模块前缀：`key_base_`

#### 类型定义

| 新命名                       | 说明     |
| ------------------------- | ------ |
| `key_base_error_t`        | 错误码枚举  |
| `key_base_event_t`        | 按键事件枚举 |
| `key_base_pin_state_t`    | 引脚状态枚举 |
| `key_base_batter_state_t` | 连击状态枚举 |
| `key_base_config_t`       | 配置结构体  |
| `key_base_context_t`      | 上下文结构体 |

#### 枚举值

```c
typedef enum {
  KEY_BASE_OK = 0,                   /**< 操作成功 */
  KEY_BASE_OK_EXISTED,               /**< 已存在 */
  KEY_BASE_ERROR_INVALID_PARAM,      /**< 无效参数 */
  KEY_BASE_ERROR_NO_MEMORY,          /**< 内存不足 */
  KEY_BASE_ERROR_NOT_FOUND,          /**< 未找到 */
  KEY_BASE_ERROR_ALREADY_EXIST,      /**< 已存在 */
  KEY_BASE_ERROR_INTERNAL,           /**< 内部错误 */
} key_base_error_t;
```

#### 函数命名

| 新命名                             | 原命名                      |
| ------------------------------- | ------------------------ |
| `key_base_init`                 | `KeyBaseInit`            |
| `key_base_deinit`               | `KeyBaseDeinit`          |
| `key_base_register`             | `KeyBaseRegister`        |
| `key_base_register_static`      | `KeyBaseRegisterStatic`  |
| `key_base_unregister`           | `KeyBaseUnregister`      |
| `key_base_task`                 | `KeyBaseTask`            |
| `key_base_get_instance`         | `GetKeyBaseInstance`     |
| `key_base_get_count`            | `KeyBaseGetCount`        |
| `key_base_combination_register` | `KeyCombinationRegister` |

***

## 实施步骤

### 步骤 1：创建目标目录

创建 `modules/key_base/` 目录。

### 步骤 2：创建重构后的头文件 `key_base.h`

按照规范重新设计：

* 文件头注释

* 包含保护宏 `__KEY_BASE_H`

* 类型定义（枚举、结构体）

* 公共函数声明

* 中文注释

### 步骤 3：创建重构后的源文件 `key_base.c`

按照规范重新实现：

* 文件头注释

* 代码组织顺序

* 分离 config\_t 和 context\_t

* 函数命名规范化

* 中文注释

### 步骤 4：更新 CMakeLists.txt

* 添加 `modules/key_base` 源文件目录

* 添加 `modules/key_base` 头文件包含路径

* 移除 `../common/keybase` 的引用

### 步骤 5：更新使用位置

更新以下文件：

1. `applications/app.h` - 修改 include 路径
2. `applications/key_menu.h` - 修改 include 路径
3. `applications/key_menu.c` - 更新类型名和函数名

### 步骤 6：验证编译

运行 CMake 构建验证无编译错误。

***

## 详细变更清单

### 新建文件

| 文件路径                          | 说明      |
| ----------------------------- | ------- |
| `modules/key_base/key_base.h` | 重构后的头文件 |
| `modules/key_base/key_base.c` | 重构后的源文件 |

### 修改文件

| 文件路径                      | 变更内容                |
| ------------------------- | ------------------- |
| `CMakeLists.txt`          | 更新源文件和头文件路径         |
| `applications/app.h`      | 修改 include 路径       |
| `applications/key_menu.h` | 修改 include 路径，更新类型名 |
| `applications/key_menu.c` | 更新类型名和函数调用          |

***

## API 映射表

### 类型映射

| 原类型               | 新类型                       |
| ----------------- | ------------------------- |
| `key_error_e`     | `key_base_error_t`        |
| `key_event_e`     | `key_base_event_t`        |
| `key_pin_state_e` | `key_base_pin_state_t`    |
| `key_batter_e`    | `key_base_batter_state_t` |
| `key_config_t`    | `key_base_config_t`       |
| `key_base_t`      | `key_base_context_t`      |

### 函数映射

| 原函数                        | 新函数                               |
| -------------------------- | --------------------------------- |
| `KeyBaseInit()`            | `key_base_init()`                 |
| `KeyBaseDeinit()`          | `key_base_deinit()`               |
| `KeyBaseRegister()`        | `key_base_register()`             |
| `KeyBaseRegisterStatic()`  | `key_base_register_static()`      |
| `KeyBaseUnregister()`      | `key_base_unregister()`           |
| `KeyBaseTask()`            | `key_base_task()`                 |
| `GetKeyBaseInstance()`     | `key_base_get_instance()`         |
| `KeyBaseGetCount()`        | `key_base_get_count()`            |
| `KeyCombinationRegister()` | `key_base_combination_register()` |

### 宏映射

| 原宏                     | 新宏                          |
| ---------------------- | --------------------------- |
| `KEY_IS_OK(err)`       | `KEY_BASE_IS_OK(err)`       |
| `KEY_IS_ERR(err)`      | `KEY_BASE_IS_ERR(err)`      |
| `KEY_EVENT_NAME_TABLE` | `KEY_BASE_EVENT_NAME_TABLE` |

***

## 风险评估

1. **兼容性风险**：API 变更较大，需要确保所有使用位置都已更新
2. **编译风险**：需要验证 CMake 构建配置正确
3. **运行时风险**：功能逻辑不变，但需要测试验证

***

## 预计工作量

* 创建新文件：2 个

* 修改文件：4 个

* 预计时间：15-20 分钟

