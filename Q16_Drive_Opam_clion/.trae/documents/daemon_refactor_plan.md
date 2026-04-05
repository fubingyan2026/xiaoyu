# Daemon 模块重构计划

## 概述

将 `/home/fubingyan/桌面/xiaoyu/common/daemon/daemon.c` 和 `daemon.h` 按照 `MODULE_CODING_GUIDE.md` 规范重构到当前工作区间，并同步更新所有调用位置。

## 重构目标

### 1. 命名规范调整

#### 类型命名
- `daemon_error_e` → `daemon_error_t`
- `daemon_init_config_t` → `daemon_config_t`
- `daemon_t` → `daemon_context_t`

#### 函数命名（全部改为小写蛇形命名）
- `DaemonInit` → `daemon_init`
- `DaemonDeinit` → `daemon_deinit`
- `DaemonRegister` → `daemon_register`
- `DaemonRegisterStatic` → `daemon_register_static`
- `DaemonUnregister` → `daemon_unregister`
- `DaemonReload` → `daemon_reload`
- `DaemonIsOnline` → `daemon_is_online`
- `DaemonGetName` → `daemon_get_name`
- `DaemonTask` → `daemon_task`
- `DaemonGetInstance` → `daemon_get_instance`
- `DaemonGetCount` → `daemon_get_count`
- `DaemonGetMasterPointer` → `daemon_get_master_pointer`

### 2. 结构体重构

#### 配置结构体 (daemon_config_t)
```c
typedef struct {
  const char* name;                        /**< 守护进程名称 */
  void* owner_ptr;                         /**< 拥有者指针 */
  daemon_offline_cb_t offline_cb;          /**< 离线回调函数 */
  uint16_t reload_timeout_ms;              /**< 超时时间(ms)，0xFFFF或0表示永不超时 */
  uint16_t init_wait_time_ms;              /**< 初始化等待时间(ms) */
} daemon_config_t;
```

#### 上下文结构体 (daemon_context_t)
```c
typedef struct daemon_context daemon_context_t;

struct daemon_context {
  daemon_config_t config;                  /**< 配置参数 */

  // 运行时状态
  uint32_t last_feed_time;                 /**< 上次喂狗时间 */
  uint32_t current_feed_time;              /**< 当前喂狗时间 */
  float feed_frequency;                    /**< 喂狗频率(Hz) */
  uint32_t init_wait_counter;              /**< 初始化等待计数器 */
  bool online;                             /**< 在线状态 */
  bool online_last;                        /**< 上次在线状态 */
  uint8_t rx_counter;                      /**< 接收计数器 */
  bool is_static;                          /**< 静态分配标志 */
  struct daemon_context* next;             /**< 链表下一个节点 */
};
```

### 3. 错误码重构

```c
typedef enum {
  DAEMON_OK = 0,                           /**< 操作成功 */
  DAEMON_OK_EXISTED,                       /**< 已存在 */
  DAEMON_ERROR_NULL_PTR,                   /**< 空指针错误 */
  DAEMON_ERROR_INVALID_PARAM,              /**< 无效参数 */
  DAEMON_ERROR_NO_MEMORY,                  /**< 内存不足 */
  DAEMON_ERROR_NOT_FOUND,                  /**< 未找到 */
  DAEMON_ERROR_ALREADY_EXIST,              /**< 已存在 */
  DAEMON_ERROR_UNINITIALIZED,              /**< 未初始化 */
  DAEMON_ERROR_GENERIC,                    /**< 通用错误 */
} daemon_error_t;
```

### 4. 回调函数类型重命名

- `offline_callback` → `daemon_offline_cb_t`
- `daemon_get_time_func` → `daemon_get_time_cb_t`

## 重构步骤

### 步骤 1：创建新的 daemon 模块目录
- 在当前工作区间创建 `modules/daemon/` 目录
- 创建 `daemon.h` 和 `daemon.c` 文件

### 步骤 2：重构头文件 (daemon.h)
1. 添加完整的文件头注释
2. 按规范组织代码结构
3. 重命名所有类型和函数
4. 添加完整的中文注释
5. 使用前向声明

### 步骤 3：重构源文件 (daemon.c)
1. 添加完整的文件头注释
2. 按规范组织代码顺序：
   - 文件头注释
   - 头文件包含
   - 私有宏定义
   - 私有变量
   - 私有函数原型
   - 导出函数
   - 私有函数
3. 重命名所有函数
4. 调整代码格式（2空格缩进）
5. 完善中文注释

### 步骤 4：更新调用文件

需要更新的文件列表：
1. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/app.c`
2. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/usart_receive.c`
3. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/can_comm/can_comm.c`
4. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/CAN_Server.c`
5. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/warning_task.c`
6. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/hall_adjustment.c`
7. `/home/fubingyan/桌面/xiaoyu/Q16_Drive_Opam_clion/applications/MT6816.c`

更新内容：
- 修改 `#include "daemon/daemon.h"` 为 `#include "modules/daemon/daemon.h"`
- 替换所有函数名为新的命名
- 替换所有类型名为新的命名
- 更新结构体成员访问方式

### 步骤 5：验证和测试
1. 编译检查是否有语法错误
2. 确保所有调用位置已正确更新
3. 检查是否有遗漏的调用位置

## 详细变更对照表

### 函数签名变更

| 原函数签名 | 新函数签名 |
|-----------|-----------|
| `daemon_error_e DaemonInit(daemon_get_time_func get_time_cb)` | `daemon_error_t daemon_init(daemon_get_time_cb_t get_time_cb)` |
| `void DaemonDeinit(void)` | `void daemon_deinit(void)` |
| `daemon_t* DaemonRegister(const daemon_init_config_t* config)` | `daemon_context_t* daemon_register(const daemon_config_t* config)` |
| `daemon_error_e DaemonRegisterStatic(const daemon_init_config_t* config, daemon_t* instance)` | `daemon_error_t daemon_register_static(const daemon_config_t* config, daemon_context_t* instance)` |
| `daemon_error_e DaemonUnregister(const char* name)` | `daemon_error_t daemon_unregister(const char* name)` |
| `void DaemonReload(daemon_t* _daemon)` | `void daemon_reload(daemon_context_t* ctx)` |
| `uint8_t DaemonIsOnline(const daemon_t* _daemon)` | `bool daemon_is_online(const daemon_context_t* ctx)` |
| `const char* DaemonGetName(const daemon_t* _daemon)` | `const char* daemon_get_name(const daemon_context_t* ctx)` |
| `void DaemonTask(void)` | `void daemon_task(void)` |
| `daemon_t* DaemonGetInstance(const char* name)` | `daemon_context_t* daemon_get_instance(const char* name)` |
| `uint16_t DaemonGetCount(void)` | `uint16_t daemon_get_count(void)` |
| `daemon_t* DaemonGetMasterPointer(void)` | `daemon_context_t* daemon_get_master_pointer(void)` |

### 结构体成员变更

| 原成员访问 | 新成员访问 |
|-----------|-----------|
| `daemon->config.owner_name` | `ctx->config.name` |
| `daemon->config.owner_pointer` | `ctx->config.owner_ptr` |
| `daemon->config.callback` | `ctx->config.offline_cb` |
| `daemon->config.reload_time_out` | `ctx->config.reload_timeout_ms` |
| `daemon->config.init_wait_time` | `ctx->config.init_wait_time_ms` |
| `daemon->data.temp_time` | `ctx->current_feed_time` |
| `daemon->data.last_temp_time` | `ctx->last_feed_time` |
| `daemon->data.daemon_frequent` | `ctx->feed_frequency` |
| `daemon->data.online` | `ctx->online` |
| `daemon->data.online_last` | `ctx->online_last` |
| `daemon->data.rx_counts` | `ctx->rx_counter` |
| `daemon->data.init_wait_time` | `ctx->init_wait_counter` |
| `daemon->data.is_static` | `ctx->is_static` |
| `daemon->next_daemon_lists` | `ctx->next` |

## 风险评估

### 潜在风险
1. **API 兼容性破坏**：所有调用位置必须同步更新，否则编译失败
2. **结构体布局变更**：可能影响内存布局和访问方式
3. **头文件路径变更**：需要更新所有 `#include` 路径

### 缓解措施
1. 使用批量查找替换确保所有调用位置更新
2. 编译验证确保无遗漏
3. 保持功能逻辑不变，仅重构命名和结构

## 预期成果

1. 完全符合 `MODULE_CODING_GUIDE.md` 规范的 daemon 模块
2. 清晰的代码结构和命名
3. 完整的中文注释
4. 所有调用位置同步更新
5. 无编译错误和警告
