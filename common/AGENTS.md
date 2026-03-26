# modules/

可复用库和组件，在FOC电机控制系统中共享。包含算法、控制器、通信协议和工具。

## 结构

```
modules/
├── algorithm/        # 数学工具、滤波器、位置计算
├── controller/       # PID控制器和电机控制算法
├── can_comm/         # CANopen类通信协议
├── encoder/          # 多编码器支持 (MT6816, AS5047P, 线性霍尔)
├── easyflash/        # 校准数据持久化Flash存储
├── memory_pool/      # 无malloc/free的内存管理
├── protocol/         # 通信协议和消息处理
├── toolkit/          # 定时器、队列、事件工具
└── [其他模块]       # 各种支持库
```

## 查找位置

| 任务 | 位置 | 说明 |
|------|----------|-------|
| 数学工具 | `algorithm/` | 定点数、滤波器、三角函数 |
| PID控制器 | `controller/pid.c` | 电机控制算法 |
| CAN通信 | `can_comm/` | CANopen类协议栈 |
| 编码器驱动 | `encoder/` | 多协议编码器支持 |
| Flash存储 | `easyflash/` | 校准数据持久化 |
| 内存管理 | `memory_pool/` | 嵌入式安全分配 |

## 约定

- **模块化设计**: 每个模块自包含，接口清晰
- **头文件保护**: 标准 `#ifndef MODULE_H` 模式
- **静态函数**: 内部函数标记为static
- **一致命名**: `module_function()` 模式

## 反模式 (MODULES)

- **不要** 在ISR关键模块中使用浮点数
- **避免** 全局变量 - 使用模块实例
- **绝不** 在库模块中包含应用特定头文件

## 独特风格

- **算法模块**: ISR兼容的定点数数学
- **EasyFlash**: 嵌入式磨损均衡Flash管理
- **内存池**: 实时安全的预分配块

## 注意事项

- 所有模块为嵌入式约束设计
- 生产代码中无动态分配
- ISR安全函数适当标记