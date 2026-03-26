# Architect Mode Rules

This file provides architectural guidance for this repository.

## Architecture Overview

STM32G431 based FOC motor driver with hard real-time requirements. C17 embedded project using STM32CubeMX + CMake build system.

## Critical Constraints

- **ISR Timing**: All ISR math must use Q16 fixed-point (`Q16_FOC/q16_16_math.c`) - float causes timing jitter
- **No Dynamic Allocation**: Use `modules/memory_pool/` - no malloc()/free() allowed
- **Flash Persistence**: Calibration data via `modules/easyflash/` - STM32 HAL EEPROM not used
- **STM32CubeMX**: All hardware config in `odrive-G431.ioc` - regenerate Drivers/ on changes

## State Machine

FOC State Machine: `Q16_FOC/foc_sm.c`
- IDLE → ALIGN → CALIBRATE → RUN → HALL

## Code Style

- Format: Microsoft-based `.clang-format`
- Comment language: Chinese for applications/, English for Q16_FOC/
- Naming: snake_case functions, PascalCase structs
- Float ABI: hard FP (`-mfloat-abi=hard -mfpu=fpv4-sp-d16`)
