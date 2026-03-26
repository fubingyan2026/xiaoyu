# applications/

**Generated:** 2026-02-13

FOC电机控制应用层。包含主入口点、编码器校准、霍尔传感器集成和CAN通信。

## 查找位置

| 任务 | 文件 | 说明 |
|------|------|-------|
| 主入口 | `app.c:APP_Run()` | 应用程序初始化和循环 |
| 编码器校准 | `encoder_alignment.c` | Flash存储的多步骤校准 |
| 霍尔传感器 | `hall_adjustment.c` | 线性霍尔集成 |
| CAN服务器 | `CAN_Server.c` | CANopen类协议处理器 |
| 性能监控 | `perf_counter.c/h` | 36KB头文件、周期计数 |

## 关键模块

- **encoder_alignment.c**: 方向检测、扇区跟踪、Flash持久化
- **hall_adjustment.c**: ADC采样、低通滤波器、角度计算
- **CAN_Server.c**: PDO映射、SDO请求、心跳管理
- **perf_counter.c**: Cortex-M周期计数器、中断延迟测量
