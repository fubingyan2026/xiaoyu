# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## OpenCode Agent Usage

When working with this codebase using OpenCode agents:

- **Language**: Agents should use English for communication, Chinese comments in `applications/` and `modules/`, English in `Q16_FOC/` core files
- **Build First**: Always verify changes compile before reporting completion
- **No Float in ISR**: Never use `float` in interrupt contexts - use Q16 fixed-point math
- **Verify Diagnostics**: Run `lsp_diagnostics` on changed files before completing tasks

## Project Overview

This is an STM32G431-based FOC (Field-Oriented Control) motor drive system. It uses Q16 fixed-point math in ISRs for timing-critical operations and includes encoder calibration, Hall sensor integration, and CAN communication.

---

## Build Commands

```bash
# Build (requires arm-none-eabi toolchain in PATH)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake
cmake --build build

# Build with specific target
cmake --build build --target Q16_Drive_Opam_clion

# Verbose build (show all compiler commands)
cmake --build build --verbose

# Parallel build (faster on multi-core)
cmake --build build -- -j$(nproc)

# Debug build (with debug symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake

# Clean build
rm -rf build && cmake -B build -DCMAKE_TOOLCHAIN_FILE=gcc-arm-none-eabi.cmake
```

### Formatting

```bash
# Format single file
clang-format -i <file.c>

# Format all C/H files in project
find . -name "*.c" -o -name "*.h" | xargs clang-format -i
```

### Static Analysis

```bash
# Run clang-tidy (if available)
clang-tidy <file.c> -- -I. -I../common

# Check compilation database
compile_commands.json is generated in build/ for IDE integration
```

### Flashing

```bash
# Flash via ST-Link (st-flash)
st-flash write build/Q16_Drive_Opam_clion.bin 0x8000000

# Flash via J-Link / ST-Link (uses STM32_Programmer_CLI)
./flash.sh

# Manual ST-Link flash with openocd
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program build/Q16_Drive_Opam_clion.elf verify reset exit"
```

### Testing

This project has **no unit tests**. Testing is done on hardware. For manual testing:
- Use CAN commands via `CAN_Server.c`
- Debug output via UART (`debug/debug.h`)
- Performance profiling via `perf_counter.c`

---

## Critical Patterns (Non-Obvious)

| Pattern | Details |
|---------|---------|
| **Q16 Fixed-Point Math** | All ISR math uses `Q16_FOC/q16_16_math.c` - never use float in ISR |
| **Comment Language** | Chinese comments in `applications/` and `modules/`, English in `Q16_FOC/` core files |
| **Flash Persistence** | Calibration data stored via `modules/easyflash/` - never use STM32 HAL EEPROM |
| **Memory Pool** | Use `modules/memory_pool/` - no dynamic allocation (malloc/free forbidden) |
| **State Machine** | FOC uses `Q16_FOC/foc_sm.c`: IDLE → ALIGN → CALIBRATE → RUN → HALL |

---

## Anti-Patterns (Must Avoid)

| Rule | Reason |
|------|--------|
| **Never modify** `Drivers/` or `CMSIS/` | STM32CubeMX regenerates these from `.ioc` file |
| **Never block in ISR** | Use flags, defer work to main loop |
| **Never use malloc()/free()** | Use memory pool or static allocation |
| **Never use float in ISR** | Use Q16 fixed-point math in `q16_16_math.c` |

---

## Code Style Guidelines

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Functions/variables | snake_case | `motor_speed_rpm`, `foc_ctrl_init()` |
| Structs | PascalCase | `FocControl_t`, `PidController` |
| Enums | PascalCase + prefix | `FOC_STATE_IDLE`, `MOTOR_DIR_FORWARD` |
| Defines/constants | UPPER_SNAKE_CASE | `MAX_CURRENT`, `PWM_FREQ_HZ` |
| File names | snake_case | `foc_ctrl_q16.c`, `can_server.h` |

### C Standard & Compiler

- **Standard**: C17 with hard FP (`-mfloat-abi=hard -mfpu=fpv4-sp-d16`)
- **MCU**: Cortex-M4 (STM32G431)
- **Compiler flags**:
  ```
  -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
  -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections
  ```

### Code Organization

```
applications/     # App layer (Chinese comments)
  - app.c         # Main entry: APP_Run()
  - CAN_Server.c  # CANopen-like protocol
  - encoder_alignment.c

Q16_FOC/          # Core FOC (English comments)
  - foc_sm.c      # State machine
  - foc_ctrl_q16.c
  - q16_16_math.c # Fixed-point math

modules/          # Common modules from ../common/
  - memory_pool/
  - easyflash/
  - pid controller
```

### Include Order (recommended)

1. Corresponding header (`"app.h"`)
2. Project headers (`"CAN_Server.h"`, `"foc_ctrl_q16.h"`)
3. Module headers (`"can_comm/can_comm.h"`, `"controller/pid.h"`)
4. External libs (`"easyflash.h"`, `"lwmem/lwmem.h"`)
5. Standard headers (`<stdint.h>`, `<string.h>`)

### Error Handling

- **Return codes**: Use `0` for success, negative for error
- **Asserts**: Use compile-time asserts for invariants
- **Debug**: Use `DEBUG_DEBUG()`, `DEBUG_INFO()`, `DEBUG_ERROR()` from `debug/debug.h`

### Memory & Performance

- **ISR-safe**: No blocking calls, no dynamic memory in ISR context
- **Stack**: Keep stack usage low; use static buffers for large data
- **Q16 conversion**: Use `Q16_FLOOR()`, `Q16_FRAC()` macros for fixed-point

---

## Entry Points

| Component | File | Function |
|-----------|------|----------|
| Main | `Core/Src/main.c` | SystemInit() → APP_Run() |
| App Loop | `applications/app.c` | APP_Run() |
| FOC State Machine | `Q16_FOC/foc_sm.c` | foc_sm_run() |
| Configuration | `Q16_FOC/foc_config_q16.h` | All tunable parameters |
| CAN Protocol | `applications/CAN_Server.c` | CAN_Server_Process() |
| Encoder | `applications/encoder_alignment.c` | Encoder_Alignment_Task() |

---

## Key Dependencies

| Library | Location | Purpose |
|---------|----------|---------|
| easyflash | `modules/easyflash/` | Flash storage for calibration |
| lwmem | `modules/lwmem/` | Static memory manager |
| lwshell | `modules/lwshell/` | Shell/CLI interface |
| CmBacktrace | `modules/CmBacktrace/` | Hard fault debugger |
| SEGGER RTT | `modules/SEGGER_RTT/` | Debug logging |

---

## Development Workflow

1. **Edit code** in `applications/`, `Q16_FOC/`, or `modules/`
2. **Format**: `clang-format -i <changed_files>`
3. **Build**: `cmake --build build`
4. **Flash**: `./flash.sh` or `st-flash write build/Q16_Drive_Opam_clion.bin 0x8000000`
5. **Debug**: Use RTT viewer or UART debug output

---

## File Header Template

```c
/**
 * @brief: [Brief description]
 * @FilePath: filename.c
 * @author: [author]
 * @Date: YYYY-MM-DD HH:MM:SS
 * @LastEditTime: YYYY-MM-DD HH:MM:SS
 * @version: V1.0.0
 * @note: Additional notes
 * @copyright (c) YYYY by author, All Rights Reserved.
 */
```

---

## Common Tasks Quick Reference

| Task | Where to look |
|------|---------------|
| Adjust PID gains | `foc_config_q16.h` or CAN PID commands |
| Change encoder type | `encoder/MT6816.c`, `encoder/line_hall_pll.c` |
| Add new CAN command | `CAN_Server.c` + `protocol/b_protocol_core.c` |
| Debug FOC | Enable `DEBUG_DEBUG()` in `foc_ctrl_q16.c` |
| Calibrate encoder | Trigger `encoder_alignment.c` state |
