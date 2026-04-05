# 协议解析器模块重构计划

## 一、模块功能分析

### 1.1 当前模块概述

`b_protocol_core.c/h` 是一个**通用协议帧解析器**，主要功能包括：

| 功能模块 | 描述 |
|---------|------|
| 帧解析 | 支持自定义帧头、帧尾、校验方式 |
| FIFO缓冲 | 使用 kfifo 环形缓冲区存储接收数据 |
| 帧长度计算 | 通过回调函数动态计算帧长度 |
| 校验验证 | 通过回调函数进行帧校验 |
| 空闲超时检测 | 检测数据传输空闲状态 |
| 多协议支持 | 支持同时解析多种协议实例 |

### 1.2 调用位置分析

| 文件路径 | 使用协议 | 调用函数 |
|---------|---------|---------|
| `Q16_Drive_Opam_clion/applications/usart_receive.c` | S7协议 | `b_frame_init`, `b_frame_put`, `b_frame_check_get`, `b_frame_idle_timer` |
| `IMU_CTRL/applications/usart_receive.c` | A5协议 | `b_frame_init`, `b_frame_put`, `b_frame_check_get` |
| `Q16_Drive_Opam_clion/applications/app.c` | - | 仅包含头文件 |

---

## 二、命名规范问题分析

### 2.1 当前命名 vs 规范命名对照

| 类型 | 当前命名 | 规范命名 | 说明 |
|-----|---------|---------|------|
| 文件名 | `b_protocol_core.c/h` | `protocol_parser.c/h` | 使用模块功能命名 |
| 错误码枚举 | `b_error_t` | `protocol_parser_error_t` | 前缀+模块名+类型后缀 |
| 枚举值 | `B_SUCCESS`, `B_ERROR_NULL_PTR` | `PROTOCOL_PARSER_OK`, `PROTOCOL_PARSER_ERROR_NULL_PTR` | 大写蛇形命名 |
| 上下文结构体 | `b_frame_type` | `protocol_parser_context_t` | 前缀+模块名+_t后缀 |
| 配置结构体 | `b_frame_init_type` | `protocol_parser_config_t` | 前缀+模块名+_t后缀 |
| 回调类型 | `b_check_cb_t`, `b_get_frame_len_cb_t` | `protocol_parser_check_cb_t`, `protocol_parser_get_len_cb_t` | 前缀+模块名 |
| 函数名 | `b_frame_init`, `b_frame_put` | `protocol_parser_init`, `protocol_parser_feed` | 前缀+模块名+动词 |

### 2.2 函数命名重命名对照表

| 原函数名 | 新函数名 | 功能描述 |
|---------|---------|---------|
| `b_frame_init` | `protocol_parser_init` | 初始化协议解析器 |
| `b_frame_deinit` | `protocol_parser_deinit` | 反初始化协议解析器 |
| `b_frame_put` | `protocol_parser_feed` | 向解析器输入数据 |
| `b_frame_check_get` | `protocol_parser_parse` | 解析并获取完整帧 |
| `b_frame_fifo_clear` | `protocol_parser_clear` | 清空解析器缓冲区 |
| `b_frame_fifo_get` | `protocol_parser_get` | 阻塞式获取指定长度数据 |
| `b_frame_idle_timer` | `protocol_parser_tick` | 更新空闲计时器 |
| `b_frame_is_initialized` | `protocol_parser_is_initialized` | 检查是否已初始化 |
| `printHexAscii` | `protocol_parser_print_hex` | 打印十六进制数据(内部函数) |

---

## 三、重构实施步骤

### 步骤 1：创建新的头文件 `protocol_parser.h`

**文件路径**: `/home/fubingyan/桌面/xiaoyu/common/protocol/protocol_parser.h`

**主要内容**:
- 文件头注释（遵循 HAL_DRIVERS_STYLE_GUIDE.md 规范）
- 错误码枚举 `protocol_parser_error_t`
- 配置结构体 `protocol_parser_config_t`
- 上下文结构体 `protocol_parser_context_t`
- 回调函数类型定义
- 公共 API 函数声明
- 所有公共 API 使用中文注释

### 步骤 2：创建新的源文件 `protocol_parser.c`

**文件路径**: `/home/fubingyan/桌面/xiaoyu/common/protocol/protocol_parser.c`

**主要内容**:
- 文件头注释（遵循 HAL_DRIVERS_STYLE_GUIDE.md 规范）
- 代码组织顺序：头文件包含 → 私有变量 → 私有函数原型 → 导出函数 → 私有函数
- 重命名所有函数和变量
- 保持原有功能逻辑不变
- 添加完整的中文注释

### 步骤 3：更新 `Q16_Drive_Opam_clion/applications/usart_receive.c`

**修改内容**:
- 更新 `#include` 引用
- 重命名类型：`b_frame_type` → `protocol_parser_context_t`
- 重命名类型：`b_frame_init_type` → `protocol_parser_config_t`
- 重命名函数调用
- 更新回调函数返回值类型

### 步骤 4：更新 `IMU_CTRL/applications/usart_receive.c`

**修改内容**:
- 更新 `#include` 引用
- 重命名类型和函数调用
- 更新回调函数返回值类型

### 步骤 5：更新 `Q16_Drive_Opam_clion/applications/app.c`

**修改内容**:
- 更新 `#include` 引用

### 步骤 6：删除旧文件

**删除文件**:
- `/home/fubingyan/桌面/xiaoyu/common/protocol/b_protocol_core.h`
- `/home/fubingyan/桌面/xiaoyu/common/protocol/b_protocol_core.c`

### 步骤 7：验证编译

**验证内容**:
- 编译 Q16_Drive_Opam_clion 项目
- 编译 IMU_CTRL 项目
- 确保无编译错误和警告

---

## 四、详细代码变更

### 4.1 新头文件结构示例

```c
//
// Created by fubingyan on 25-4-5.
//

#ifndef __PROTOCOL_PARSER_H
#define __PROTOCOL_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 协议解析器错误码枚举
 */
typedef enum {
  PROTOCOL_PARSER_OK = 0,              /**< 操作成功 */
  PROTOCOL_PARSER_ERROR_NULL_PTR,      /**< 空指针错误 */
  PROTOCOL_PARSER_ERROR_BUFFER_OVERFLOW, /**< 缓冲区溢出 */
  PROTOCOL_PARSER_ERROR_TIMEOUT,       /**< 超时错误 */
  PROTOCOL_PARSER_ERROR_CHECKSUM,      /**< 校验失败 */
  PROTOCOL_PARSER_ERROR_INIT_FAILED,   /**< 初始化失败 */
  PROTOCOL_PARSER_ERROR_INVALID_PARAM, /**< 无效参数 */
  PROTOCOL_PARSER_ERROR_BUFFER_TOO_SMALL, /**< 缓冲区过小 */
  PROTOCOL_PARSER_ERROR_FRAME_INVALID, /**< 帧无效 */
  PROTOCOL_PARSER_ERROR_MEMORY_ALLOC,  /**< 内存分配失败 */
  PROTOCOL_PARSER_ERROR_UNINITIALIZED, /**< 未初始化 */
  PROTOCOL_PARSER_ERROR_GENERIC,       /**< 通用错误 */
} protocol_parser_error_t;

// ... 其他类型定义和函数声明
```

### 4.2 回调函数类型变更

| 原类型 | 新类型 |
|-------|-------|
| `b_check_cb_t` | `protocol_parser_check_cb_t` |
| `b_get_frame_len_cb_t` | `protocol_parser_get_len_cb_t` |

---

## 五、影响范围

### 5.1 直接影响文件

| 文件 | 修改类型 |
|-----|---------|
| `common/protocol/b_protocol_core.h` | 删除 |
| `common/protocol/b_protocol_core.c` | 删除 |
| `common/protocol/protocol_parser.h` | 新建 |
| `common/protocol/protocol_parser.c` | 新建 |
| `Q16_Drive_Opam_clion/applications/usart_receive.c` | 修改 |
| `Q16_Drive_Opam_clion/applications/app.c` | 修改 |
| `IMU_CTRL/applications/usart_receive.c` | 修改 |

### 5.2 兼容性考虑

- 所有调用代码需要同步更新
- 无向后兼容性要求（内部模块）

---

## 六、验证清单

- [ ] 新文件符合 HAL_DRIVERS_STYLE_GUIDE.md 规范
- [ ] 所有公共 API 有中文注释
- [ ] 命名符合规范（前缀、蛇形命名）
- [ ] 参数验证完整
- [ ] 错误码返回正确
- [ ] 未使用参数用 `(void)` 标记
- [ ] 代码格式符合要求（2空格缩进）
- [ ] Q16_Drive_Opam_clion 项目编译通过
- [ ] IMU_CTRL 项目编译通过
- [ ] 无编译警告

---

## 七、风险评估

| 风险项 | 风险等级 | 缓解措施 |
|-------|---------|---------|
| 函数重命名遗漏 | 低 | 使用 grep 全局搜索确认 |
| 编译错误 | 低 | 逐步修改，及时编译验证 |
| 功能回归 | 低 | 保持原有逻辑不变，仅重命名 |

---

**计划创建时间**: 2026-04-05
**计划创建者**: AI Assistant
