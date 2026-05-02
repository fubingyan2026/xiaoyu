# 协议打包器 (protocol_packer) 实现计划

## 概述

参考现有的 `protocol_parser` 模块，实现一个互补的协议打包器 `protocol_packer`，用于将数据打包成符合协议格式的帧。

## 设计分析

### protocol_parser 核心功能
- 帧头检测与匹配
- 帧尾检测与匹配
- 帧长度计算（通过回调）
- 帧校验（通过回调）
- FIFO 数据缓冲
- 空闲超时处理

### protocol_packer 互补功能
- 添加帧头
- 添加帧尾
- 计算并添加校验码（通过回调）
- 帧长度字段填充（通过回调）
- 输出缓冲区管理

## 文件结构

```
protocol_packer/
├── protocol_packer.h    # 模块头文件
├── protocol_packer.c    # 模块实现
```

## 数据结构设计

### 错误码枚举
```c
typedef enum {
  PROTOCOL_PACKER_OK = 0,                   // 操作成功
  PROTOCOL_PACKER_ERROR_NULL_PTR,           // 空指针错误
  PROTOCOL_PACKER_ERROR_INVALID_PARAM,      // 无效参数
  PROTOCOL_PACKER_ERROR_UNINITIALIZED,      // 未初始化
  PROTOCOL_PACKER_ERROR_BUFFER_OVERFLOW,    // 缓冲区溢出
  PROTOCOL_PACKER_ERROR_BUFFER_TOO_SMALL,   // 缓冲区过小
  PROTOCOL_PACKER_ERROR_CALLBACK_NULL,      // 回调函数为空
  PROTOCOL_PACKER_ERROR_GENERIC,            // 通用错误
} protocol_packer_error_t;
```

### 回调函数类型
```c
// 校验码计算回调
typedef protocol_packer_error_t (*protocol_packer_checksum_cb_t)(
    const uint8_t* data, uint16_t len, uint8_t* checksum_out, uint16_t* checksum_len);

// 帧长度字段填充回调
typedef protocol_packer_error_t (*protocol_packer_fill_len_cb_t)(
    uint8_t* buffer, uint16_t payload_len);
```

### 配置结构体
```c
typedef struct {
  const char* name;                          // 协议名称
  const uint8_t* header;                     // 帧头数据
  const uint8_t* footer;                     // 帧尾数据
  uint8_t* output_buffer;                    // 输出缓冲区
  protocol_packer_checksum_cb_t checksum_cb; // 校验码计算回调
  protocol_packer_fill_len_cb_t fill_len_cb; // 帧长度填充回调
  uint16_t header_len;                       // 帧头长度
  uint16_t footer_len;                       // 帧尾长度
  uint16_t checksum_len;                     // 校验码长度
  uint16_t output_buffer_len;                // 输出缓冲区大小
} protocol_packer_config_t;
```

### 上下文结构体
```c
typedef struct protocol_packer_context protocol_packer_context_t;

struct protocol_packer_context {
  protocol_packer_config_t config;  // 配置参数
  bool initialized;                 // 初始化标志
};
```

## 公共 API 设计

| 函数 | 描述 |
|------|------|
| `protocol_packer_init()` | 初始化打包器 |
| `protocol_packer_deinit()` | 反初始化打包器 |
| `protocol_packer_pack()` | 打包数据为完整帧 |
| `protocol_packer_get_frame()` | 获取打包后的帧数据 |
| `protocol_packer_clear()` | 清空输出缓冲区 |
| `protocol_packer_is_initialized()` | 检查初始化状态 |

### 核心 API 详解

#### protocol_packer_pack()
```c
/**
 * @brief 打包数据为完整帧
 * @param ctx 打包器上下文指针
 * @param data 输入数据指针
 * @param len 数据长度
 * @param p_out_frame 输出参数，返回帧数据指针
 * @param p_out_len 输出参数，返回帧长度
 * @return 操作结果错误码
 * 
 * 帧结构: [帧头][数据][校验码][帧尾]
 */
protocol_packer_error_t protocol_packer_pack(
    protocol_packer_context_t* ctx,
    const uint8_t* data,
    uint16_t len,
    uint8_t** p_out_frame,
    uint16_t* p_out_len);
```

## 实现步骤

### 步骤 1: 创建头文件 protocol_packer.h
- 定义错误码枚举
- 定义回调函数类型
- 定义配置结构体和上下文结构体
- 声明公共 API

### 步骤 2: 创建源文件 protocol_packer.c
- 实现初始化/反初始化函数
- 实现 `protocol_packer_pack()` 核心打包逻辑:
  1. 检查参数有效性
  2. 检查缓冲区大小
  3. 复制帧头到输出缓冲区
  4. 复制数据到输出缓冲区
  5. 调用校验回调计算校验码
  6. 复制帧尾到输出缓冲区
  7. 返回帧数据指针和长度
- 实现辅助函数

### 步骤 3: 验证
- 检查代码格式符合规范
- 确保与 protocol_parser 风格一致

## 与 protocol_parser 的对应关系

| protocol_parser (解析) | protocol_packer (打包) |
|------------------------|------------------------|
| 检测帧头 | 添加帧头 |
| 检测帧尾 | 添加帧尾 |
| 校验帧数据 | 计算并添加校验码 |
| 计算帧长度 | 填充帧长度字段 |
| 输入数据流 → 输出帧 | 输入数据 → 输出帧 |

## 注意事项

1. 遵循 MODULE_CODING_GUIDE.md 规范
2. 使用中文注释
3. 使用 snake_case 命名
4. 不使用动态内存分配（打包器不需要 FIFO）
5. 参数检查完整
6. 错误码返回明确
