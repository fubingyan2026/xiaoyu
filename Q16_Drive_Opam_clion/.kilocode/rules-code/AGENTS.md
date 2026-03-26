# Code Mode Rules

This file provides coding-specific guidance for this repository.

## Non-Obvious Coding Rules

- **Q16 Fixed-Point Math**: All ISR math uses `Q16_FOC/q16_16_math.c` - never use float in ISR
- **Chinese comments** in `applications/` and `modules/`, **English** in `Q16_FOC/` core files
- **Flash storage**: Use `modules/easyflash/` for calibration persistence - never use STM32 HAL EEPROM
- **Memory allocation**: Use `modules/memory_pool/` only - no malloc()/free()
- **Never modify Drivers/ or CMSIS/**: STM32CubeMX regenerates from `.ioc` file

## Entry Points

- Main: `Core/Src/main.c` → `applications/app.c:APP_Run()`
- FOC State Machine: `Q16_FOC/foc_sm.c`
- Configuration: `Q16_FOC/foc_config_q16.h`
