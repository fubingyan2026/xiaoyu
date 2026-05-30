# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

STM32G431CB (Cortex-M4, 128KB flash, 32KB RAM) FOC motor controller firmware.

## Build & Flash

```bash
./build.sh                        # cmake -B build/RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja && ninja -C build/RelWithDebInfo
./flash_jlink.sh                  # Flash via J-Link (SWD)
./flash_stlink.sh                 # Flash via ST-Link (STM32_Programmer_CLI)
```

Build output: `build/RelWithDebInfo/Q16_Drive_Opam_clion.{elf,hex,bin}`.
Toolchain: `arm-none-eabi-gcc` via `cmake/gcc-arm-none-eabi.cmake`.
CMake presets available: `cmake --preset RelWithDebInfo` (also Debug, Release, MinSizeRel).

## 4-Layer Architecture

| Layer | Location | Role |
|---|---|---|
| STM32 HAL | `Core/`, `Drivers/` | CubeMX-generated, modify only inside `/* USER CODE BEGIN */` blocks |
| HAL abstraction | `hal_drivers/` | ops-table polymorphism (`hal_xxx.h/c` + `stm32_hal_xxx.c`) |
| Middlewares | `Middlewares/` | Business logic: algorithm, protocol, services, utils, message_center, logic |
| Applications | `applications/`, `Q16_FOC/` | FOC control loop, sensor drivers, communication, main loop |

Application entry points: `AppInit()` and `AppRunning()` in `applications/app.c`. The main loop is event-driven using `tk_event_t` flags — ISRs post events, the loop processes them.

## Coding Standards

生成代码时严格遵循以下规范文档：

- **全项目通用**：[`MODULE_CODING_GUIDE.md`](MODULE_CODING_GUIDE.md) — 适用于所有目录，定义了代码风格（WebKit）、命名、注释、模块模式、内存管理等规范
- **HAL 层专用**：[`hal_drivers/HAL_DRIVERS_STYLE_GUIDE.md`](hal_drivers/HAL_DRIVERS_STYLE_GUIDE.md) — **仅对 `hal_drivers/` 目录生效**，定义了硬件抽象层的分层架构、ops 表模式、平台注册等特有规范

两个文档的格式化规则一致（WebKit 风格：4 空格缩进、Allman 函数大括号、K&R 控制语句大括号、`type*` 指针声明、不超过 100 字符行长）。HAL 层文档在通用规范基础上叠加了分层设计、上下文管理、constructor 自动注册等专属规则。

所有公共 API 和复杂逻辑注释使用**中文**。

## Middleware Module Pattern

Every middleware module follows this convention:

```c
typedef struct { ... callback_t cb; } module_config_t;   // All config + callbacks, const-correct
typedef struct { module_config_t config; bool initialized; ... } module_context_t;  // config nested inside

module_error_t module_init(module_context_t* ctx, const module_config_t* config);
void module_deinit(module_context_t* ctx);
bool module_is_initialized(const module_context_t* ctx);
```

- `config_t` — all init-time parameters including callbacks. `context_t` — nests config + runtime state.
- `init()` checks `ctx->initialized`, calls `deinit()` first if already init'd, copies config with `ctx->config = *config`.
- Error enum values: `MODULE_OK = 0`, negative values are errors.

## Critical: __malloc → memset

The project uses `__malloc`/`__free` (`Middlewares/utils/memory_pool.h`) for dynamic memory. **Every `__malloc` must be immediately zeroed** with standard `memset`:

```c
ctx = (context_t*)__malloc(sizeof(context_t));
if (ctx == NULL) return ERROR;
memset(ctx, 0, sizeof(context_t));  // MANDATORY
```

Without this, `ctx->initialized` may be garbage-true, causing `init()` to call `deinit()` on garbage pointers → hard fault.

`__malloc`/`__free` 之外的其他函数使用 C 标准库（`memset`, `memcpy` 等）。

## Middlewares 优先

生成代码时，**优先使用 `Middlewares/` 下已有的功能模块**，避免重复造轮。通过 `#include "public.h"` 即可引入所有 Middlewares 模块。

| 分类 | 模块 | 路径 | 用途 |
|---|---|---|---|
| **Algorithm** | PID | `algorithm/controller/pid.h` | 通用 PID 控制器 |
| | Gimbal PID | `algorithm/controller/gimbal_pid.h` | 云台专用 PID |
| | Filter | `algorithm/filter/filter.h` | 数字滤波器（低通、移动平均等） |
| | PLL | `algorithm/pll/pll.h` | 锁相环（用于无感 FOC） |
| | Math | `algorithm/math/maths.h` | Q16 定点数学库 |
| | CRC | `algorithm/crc.h` | CRC 校验 |
| **Services** | Debug | `services/debug/debug.h` | ESP32 风格日志（`DEBUG_LOGE/W/I/D/T`） |
| | Daemon | `services/daemon/daemon.h` | 任务健康监控 + 看门狗 |
| | LED | `services/led/led.h` | LED 状态管理 |
| | CAN Comm | `services/can_comm/can_comm.h` | CAN 通信服务 |
| **Protocol** | Parser | `protocol/protocol_parser.h` | 数据包解析（帧匹配、校验） |
| | Packer | `protocol/protocol_packer.h` | 数据包封装 |
| **Logic** | FSM | `fsm/fsm.h` | O(1) 状态机，on-entry/on-exit 回调 |
| | Key Base | `key_base/key_base.h` | 按键扫描与事件分发 |
| **Message** | Message Center | `message_center/message_center.h` | 发布-订阅消息总线 |
| **Sensor** | Angle Sensor | `angle_sensor/angle_sensor.h` | 角度传感器抽象 |
| **Utils** | kfifo | `utils/kfifo.h` | 无锁环形缓冲区（DMA 友好） |
| | Memory Pool | `utils/memory_pool.h` | `__malloc`/`__free` 内存池 |
| | clist | `utils/clist.h` | 通用双向链表 |
| | Toolkit | `utils/toolkit/toolkit.h` | 工具函数集 |

## public.h Pattern

`Middlewares/public.h` aggregates all middleware headers. Application code includes only this:

```c
#include "public.h"
```

Adding a new middleware module requires: add its `#include` to `public.h` + `aux_source_directory` in `CMakeLists.txt`.

## NOINITRAM

32-byte section at `0x20000000` (`STM32G431XX_FLASH.ld`) that survives resets — bss zero-init does not touch it. Usage:

```c
__attribute__((section(".noinitram.system_boot_count"), used)) volatile uint32_t update_flag;
__attribute__((section(".noinitram.system_device_name"), used)) volatile char device_name[4];
```

Sub-sections: `system_boot_count`, `system_device_name`, `system_status_flags`, `system_persistent_data`.

## Key Constraints

- **NEVER edit CubeMX files** (`Core/Src/main.c`, `stm32g4xx_hal_msp.c`, `stm32g4xx_it.c`) outside `/* USER CODE BEGIN */` / `/* USER CODE END */` blocks. Re-generation from `.ioc` overwrites other changes.
- **No heap in ISR** — FOC control runs in ADC interrupt; all math is Q16 fixed-point (`Q16_FOC/q16_16_math.c`).
- **DMA-based UART TX** — debug output goes through kfifo → timer_uartTask (100ms) checks DMA idle before sending.
- **No tests, no CI** — verify by building, flashing, and observing hardware behavior.

## Third-Party Libraries

| Library | Location | Purpose |
|---|---|---|
| CmBacktrace | `Middlewares/Third_Party/CmBacktrace/` | Hard fault stack trace |
| EasyFlash | `Middlewares/Third_Party/easyflash/` | Wear-leveled flash KV store |
| lwmem | `Middlewares/Third_Party/lwmem/` | Lightweight memory pool |
| lwshell | `Middlewares/Third_Party/lwshell/` | Interactive debug shell |
| SEGGER_RTT | `Middlewares/Third_Party/SEGGER_RTT/` | Real-time terminal via J-Link |

## Per-Directory Docs

Additional conventions: `Core/AGENTS.md`, `Q16_FOC/AGENTS.md`, `Middlewares/algorithm/AGENTS.md`, `Middlewares/Third_Party/easyflash/AGENTS.md`.
