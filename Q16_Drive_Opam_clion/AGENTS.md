# AGENTS.md - Q16 FOC Motor Driver

Agent guidelines for working with this STM32G431 FOC motor driver codebase.

## Build Commands

### Build Project
```bash
# Using CMake with Ninja generator
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja

# Or with Make
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Flash and Debug
```bash
# Flash via ST-Link (assuming st-flash installed)
st-flash write build/Q16_Drive_Opam_clion.bin 0x08000000

# Debug with GDB
arm-none-eabi-gdb build/Q16_Drive_Opam_clion.elf
```

### Lint/Format
```bash
# Format code with clang-format
clang-format -i --style=file src/*.c

# Check formatting
clang-format --style=file --check src/*.c

# compile_commands.json is generated at build/compile_commands.json
```

### Single File Compilation Test
```bash
# Compile single source file to check syntax
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
  -I. -I../common -Iapplications -IQ16_FOC -Ihal_drivers \
  -std=c17 -Wall -Werror src/file.c
```

### Running Tests
**Note**: This is an embedded project - no automatic unit tests exist. Testing is done manually via hardware.

---

## Code Style Guidelines

### Naming Conventions
- **Functions**: `snake_case` (e.g., `foc_ctrl_init()`, `encoder_read_angle()`)
- **Structs/Types**: `PascalCase` (e.g., `FocController`, `MotorConfig`)
- **Enums**: `UPPER_SNAKE_CASE` (e.g., `FOC_STATE_IDLE`, `ENCODER_TYPE_MT6816`)
- **Macros**: `UPPER_SNAKE_CASE` (e.g., `#define MOTOR_POLES 8`)
- **Variables**: `snake_case` (e.g., `current_angle`, `target_velocity`)

### Comments
- **applications/ and modules/**: Chinese comments
- **Q16_FOC/ core**: English comments
- Use `//` for single-line, `/* */` for multi-line

### Formatting
- **.clang-format** config is in project root (Google-based, 2-space indent)
- Column limit: 80
- Use spaces, not tabs
- Braces on same line (Google style)

### Includes
- Sort includes with clang-format
- Order: system headers, project headers, local headers
- Use forward declarations when possible

### Types
- **ISR math**: Use Q16 fixed-point (`Q16_FOC/q16_16_math.c`) - NEVER use float
- **Fixed-width integers**: `int32_t`, `uint16_t`, etc. (stdint.h)
- **Booleans**: Use `bool` from stdbool.h

### Memory
- **NEVER use malloc()/free()** - use `modules/memory_pool/` instead
- Prefer static allocation and global buffers
- Use memory pool for dynamic needs

### Flash Storage
- Use `modules/easyflash/` for calibration persistence
- NEVER use STM32 HAL EEPROM functions

---

## Project Structure

```
Q16_Drive_Opam_clion/
├── applications/     # Motor control apps (Chinese comments)
├── Q16_FOC/          # FOC core algorithms (English comments)
├── hal_drivers/      # Hardware abstraction layer
├── bsp_drivers/      # Board support drivers
├── can_comm/         # CAN communication
├── Core/             # STM32CubeMX generated (DO NOT EDIT)
├── cmake/            # Build configuration
└── ../common/        # Shared modules

Entry Points:
- Main: Core/Src/main.c → applications/app.c:APP_Run()
- FOC State Machine: Q16_FOC/foc_sm.c
- Configuration: Q16_FOC/foc_config_q16.h
```

---

## State Machine

FOC State Machine: `Q16_FOC/foc_sm.c`
- IDLE → ALIGN → CALIBRATE → RUN → HALL

---

## Critical Constraints

- **Hardware**: NEVER modify Drivers/ or CMSIS/ directories
- STM32CubeMX regenerates these from `.ioc` file
- Edit `.ioc` file and regenerate when hardware config changes

---

## Debugging

- Use SEGGER RTT for real-time debug output (not printf)
- Check `applications/perf_counter.c` for cycle counting
- Calibration data stored in Flash via easyflash

---

## Error Handling

- Return error codes (negative values) for failures
- Use `assert()` for debugging, remove in release
- Log errors to persistent storage for field diagnostics