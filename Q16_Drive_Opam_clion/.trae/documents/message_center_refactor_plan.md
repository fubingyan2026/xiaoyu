# message_center 模块重构计划

## 任务概述

将 `/home/fubingyan/桌面/xiaoyu/common/message_center/message_center.h` 和 `.c` 文件按照 `MODULE_CODING_GUIDE.md` 的规范重构到当前工作区间 `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion` 下。

## 重构步骤

### 1. 创建目录结构
- 在当前工作区创建 `message_center/` 目录
- 目录结构：
  ```
  message_center/
  ├── message_center.h              # 模块头文件
  ├── message_center.c              # 模块实现
  └── MODULE_STYLE_GUIDE.md         # 模块规范文档（可选）
  ```

### 2. 重构头文件 (message_center.h)

按照规范模板重构：
- 添加标准文件头注释（Created by 格式）
- 使用标准头文件保护格式（`__MODULE_NAME_H`）
- 添加 `extern "C"` 保护
- 按照标准顺序组织代码：
  - Includes（使用标准库头文件）
  - Exported types（错误码、回调函数类型、配置结构体、上下文结构体）
  - Exported constants
  - Exported macro
  - Exported functions prototypes
- 所有类型添加 `message_center_` 前缀
- 所有函数添加 `message_center_` 前缀
- 添加详细的中文注释

### 3. 重构源文件 (message_center.c)

按照规范模板重构：
- 添加标准文件头注释（包含@file, @author, @version, @date, @brief, @attention）
- 按照标准顺序组织代码：
  - Includes
  - Private constants
  - Private variables
  - Private function prototypes
  - Exported functions
  - Private functions
- 使用 2 空格缩进
- 所有私有函数使用 `static` 修饰
- 参数验证完整
- 错误处理规范
- 未使用参数用 `(void)` 标记

### 4. 主要架构改进

#### 4.1 错误码规范
- 第一个值为 `MESSAGE_CENTER_OK = 0`
- 使用大写蛇形命名，带模块前缀
- 每个错误码都有中文注释

#### 4.2 配置结构体设计
创建 `message_center_config_t` 结构体：
- 包含所有配置参数
- 回调函数（如果有）放在配置结构体中
- 使用 `const` 修饰只读指针

#### 4.3 上下文结构体设计
创建 `message_center_context_t` 结构体：
- 嵌套配置结构体 `config` 成员
- 运行时状态与配置分离
- 使用前向声明

#### 4.4 函数设计规范
- 初始化函数：`message_center_init()`
- 反初始化函数：`message_center_deinit()`
- 状态检查函数：`message_center_is_initialized()`
- 所有公共函数都有完整的参数检查和错误返回

### 5. 代码格式调整

- 缩进：2 空格
- 行宽：不超过 100 字符
- 大括号位置：Google 风格（与函数名同行）
- 函数声明换行：参数对齐

### 6. 类型安全

- 使用固定宽度整数类型（stdint.h）
- 使用布尔类型（stdbool.h）
- const 正确性（输入参数使用 const）

### 7. 注释规范

- 所有公共 API 使用中文注释
- 枚举值、结构体成员都有详细中文注释
- 函数注释包含@brief, @param, @return

## 验收标准

- [ ] 文件头注释完整
- [ ] 代码组织顺序正确
- [ ] 命名符合规范（message_center_ 前缀）
- [ ] 所有公共 API 有中文注释
- [ ] 配置结构体和上下文结构体设计合理
- [ ] 参数验证完整
- [ ] 错误码返回正确
- [ ] 使用固定宽度整数类型
- [ ] 代码格式符合要求（2 空格缩进）
- [ ] 无编译错误和警告
