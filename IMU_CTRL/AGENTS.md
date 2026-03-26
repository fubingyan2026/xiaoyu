# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## 项目概述

STM32G4 云台控制系统，基于 FreeRTOS 的三轴稳定平台。

## 构建命令

```bash
# 方式1: 使用构建脚本 (推荐)
./build.sh

# 方式2: 手动 CMake + Ninja
cmake -B build/RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
ninja -C build/RelWithDebInfo

# 烧录
./flash.sh
```

## 目录结构

| 目录 | 说明 |
|------|------|
| `applications/` | 应用层任务 (云台、IMU、CAN通信) |
| `hal_drivers/` | 硬件抽象层驱动 |
| `Core/` | STM32CubeMX 生成代码 |
| `../common/` | 共享库 (算法、通信、协议) |

## 关键模块

- `applications/app.c` - 主应用入口，任务创建和初始化
- `applications/ins_task.c` - IMU 姿态解算
- `applications/gimbal_task.c` - 云台控制逻辑
- `applications/CAN_Server.c` - CAN 通信服务

## 任务调度 (app.c)

| 任务 | 周期 | 优先级 |
|------|------|--------|
| StartCaculateTask | 1ms | 最高 |
| StartCanCommTask | 1ms | 高 |
| StartNormalTask | 2ms | 中 |
| StartDebugTask | 20ms | 低 |

## 代码风格

- 格式: `.clang-format` (基于 Microsoft 风格)
- 语言标准: C17
- 编译数据库: `build/RelWithDebInfo/compile_commands.json`

## 单元测试

本项目使用 GoogleTest 框架进行单元测试，仅适用于不依赖硬件抽象层 (HAL) 的纯算法代码。

### 构建和运行测试

```bash
# 配置测试项目
cmake -B tests/build -S tests -DCMAKE_BUILD_TYPE=RelWithDebInfo

# 编译测试
cmake --build tests/build

# 运行测试
./tests/build/run_tests
```

### 测试覆盖范围

- `controller/pid.c` - PID 控制器算法
- `algorithm/maths.c` - 数学工具函数
- `algorithm/filter.c` - 滤波器算法

### 注意事项

- 硬件相关代码（如 `gimbal_pid.c` 依赖 `bsp_delay.h`）无法进行单元测试
- 共享库位于 `../common/` (项目父目录)
- 编译输出: `build/RelWithDebInfo/IMU_CTRL.{elf,hex,bin}`
