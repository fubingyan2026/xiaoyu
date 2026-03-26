# Project Overview: Q16_Drive_Opam_clion (FOC Motor Drive System)

High-performance Field-Oriented Control (FOC) motor drive system optimized for the **STM32G431** microcontroller. It features advanced sensor support, fixed-point math for efficiency, and a dedicated Python-based host interface for calibration and monitoring.

## Core Technologies
- **Microcontroller:** STM32G4 series (Cortex-M4 with FPU/OPAMPs).
- **Algorithms:** FOC (Field-Oriented Control) with SVPWM (Space Vector Pulse Width Modulation) using **Q16.16 fixed-point math**.
- **Scheduling:** Event-driven architecture with custom software timers (`tk_timer`) and events (`tk_event`).
- **Communication:** CAN FD (custom protocol) and UART (A5 protocol for host PC).
- **Peripherals:** Integrated OPAMPs for current sensing, ADC for phase current and Hall sensing, HRTIM/TIM for PWM.
- **Support Libraries:** `lwmem` (dynamic memory), `lwshell` (CLI), `easyflash` (NVM), `cm_backtrace` (fault handling).

## Key Modules
- **`applications/`**: Application entry (`app.c`) and high-level tasks (LED, UART, Driver, Warning).
- **`Q16_FOC/`**: Core FOC implementation, including the state machine (`IDLE`, `ALIGN`, `CALIBRATE`, `RUN`, `HALL`), SVPWM, and Q16.16 math.
- **`hal_drivers/`**: Hardware abstraction layer for peripherals (FDCAN, GPIO, PWM, UART).
- **`pc_host/`**: Python/PyQt6 GUI for real-time monitoring (current, speed, position), control, and encoder calibration.
- **`common/`**: Shared utility library (external, one level up) providing PID, circular buffers (kfifo), protocols, and more.

## Building and Running

### Firmware (STM32)
The project uses CMake with the `gcc-arm-none-eabi` toolchain.

```bash
# Standard Build Process
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Using provided script
./build.sh
```

### Flashing
Use the provided `flash.sh` script (requires `st-link` or `openocd`).

```bash
./flash.sh
```

### Host PC (Python GUI)
Requirements: Python 3.x, PyQt6, PySerial, etc.

```bash
cd pc_host
pip install -r requirements.txt
python main.py
```

## Development Conventions
- **Code Style:** Adheres to `.clang-format` (LLVM-based style).
- **Math:** Prefer `Q16_FOC/q16_16_math.h` for performance-critical control loops.
- **Error Handling:** Use `ASSERT()` for critical checks and `cm_backtrace` for debugging faults.
- **Configuration:** Hardware and algorithm parameters are centralized in `Q16_FOC/foc_config_q16.h`.
- **Modularity:** Extensive use of linked lists and instance-based management (e.g., PID, CAN instances).
- **Comments:** Mix of Chinese and English; ensure critical logic is documented.

## Key Files
- `README.md`: System architecture and calibration flow diagrams.
- `applications/app.c`: Main application logic and task scheduling.
- `Q16_FOC/foc_config_q16.h`: Crucial motor and control parameters.
- `protocol.md`: Documentation for the binary communication protocol.
- `pc_host/core/protocol.py`: Host-side implementation of the A5 protocol.
