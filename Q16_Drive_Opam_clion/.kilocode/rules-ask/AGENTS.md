# Ask Mode Rules

This file provides guidance for understanding this codebase.

## Codebase Organization

- **Q16_FOC/**: FOC core algorithms - English comments, Q16 fixed-point math
- **applications/**: Motor control applications - Chinese comments, calibration, CAN
- **modules/**: Reusable libraries in ../common/ directory
- **Core/**: STM32CubeMX generated code - DO NOT modify directly
- **hal_drivers/**: Hardware abstraction layer

## Key Components

- FOC Control Loop: `Q16_FOC/foc_ctrl_q16.c` - main PI controllers
- SVPWM Generation: `Q16_FOC/foc_svpwm_q16.c` - space vector PWM
- Encoder Calibration: `applications/encoder_alignment.c` - multi-step calibration
- Hall Sensor: `applications/hall_adjustment.c` - linear hall integration
- CAN Communication: `modules/can_comm/` - CANopen-like protocol
- PID Controller: `modules/controller/pid.c` - motor PID implementation

## Configuration

- Configuration file: `Q16_FOC/foc_config_q16.h`
- Hardware config: `odrive-G431.ioc` (STM32CubeMX)
