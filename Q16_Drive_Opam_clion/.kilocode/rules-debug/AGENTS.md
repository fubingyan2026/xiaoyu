# Debug Mode Rules

This file provides debugging-specific guidance for this repository.

## Non-Obvious Debug Patterns

- **SEGGER RTT**: Use `Ozone.jdebug` or RTT Viewer for real-time debug output - not printf
- **ISR Timing**: Q16 fixed-point math in ISR - don't expect float debugging to work in ISR context
- **Flash Persistence**: Calibration data in `modules/easyflash/` - check Flash content if calibration seems wrong
- **perf_counter.c**: 36KB header file provides cycle counting - useful for interrupt latency measurement

## Entry Points

- Main: `Core/Src/main.c` → `applications/app.c:APP_Run()`
- FOC State Machine: `Q16_FOC/foc_sm.c` (IDLE → ALIGN → CALIBRATE → RUN → HALL)
- Configuration: `Q16_FOC/foc_config_q16.h`
