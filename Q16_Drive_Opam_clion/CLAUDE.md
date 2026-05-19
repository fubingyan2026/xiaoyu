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

Two conflicting style guides exist. Resolution:

- **`.clang-format` takes precedence** — 2-space indent, Google style, K&R braces, 80-column limit
- `hal_drivers/HAL_DRIVERS_STYLE_GUIDE.md` matches clang-format (2-space, K&R)
- `Middlewares/protocol/MODULE_CODING_GUIDE.md` specifies 4-space indent and Allman-style function braces, but `.clang-format` reformats over this

Write code that passes `clang-format`. All public API and complex logic comments in **Chinese**.

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

## Critical: __malloc → __memset

The project uses `__malloc`/`__free` (`Middlewares/utils/memory_pool.h`). **Every `__malloc` must be immediately zeroed:**

```c
ctx = (context_t*)__malloc(sizeof(context_t));
if (ctx == NULL) return ERROR;
__memset(ctx, 0, sizeof(context_t));  // MANDATORY
```

Without this, `ctx->initialized` may be garbage-true, causing `init()` to call `deinit()` on garbage pointers → hard fault.

## Key Frameworks

All accessible via `#include "public.h"`:

- **FSM** (`Middlewares/logic/fsm.h`) — O(1) transition table, on-entry/on-exit callbacks, debug names. Used by FOC, LED, keys, CAN.
- **Message Center** (`Middlewares/message_center/message_center.h`) — pub-sub with shared ring-buffer per topic. Publisher owns the queue; subscribers track individual read positions.
- **kfifo** (`Middlewares/utils/kfifo.h`) — lockless ring buffer, buffer size must be power-of-2. `kfifo_skip_in()`/`kfifo_move_in()` for DMA sync.
- **Daemon** (`Middlewares/services/daemon/daemon.h`) — linked-list task health monitor with reload timeouts. `daemon_task()` must be called periodically from main loop.
- **Debug** (`Middlewares/services/debug/debug.h`) — ESP32-style logging: `DEBUG_LOGE/W/I/D/T(tag, ...)`, `DEBUG_ASSERT(expr)`. Output via internal kfifo, flushed by timer to UART DMA.

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
