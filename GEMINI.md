# Project Overview

This workspace contains two primary embedded C/C++ projects for STM32G4 microcontrollers, along with a shared `common` library of modules. Both projects utilize CMake for their build systems, integrate with STM32CubeMX generated code, and are designed for real-time control applications.

1.  **`IMU_CTRL` (Gimbal Control System):** This project implements a three-axis gimbal stabilization platform. It processes input from a PS4 controller, integrates attitude feedback from an MPU6500 IMU, and uses motor encoder data for high-precision closed-loop control of pitch, roll, and yaw axes. It relies on FreeRTOS for concurrent task execution.

2.  **`Q16_Drive_Opam_clion` (FOC Motor Drive System):** This project focuses on Field-Oriented Control (FOC) for motor drives. It features advanced encoder calibration, support for linear Hall sensor-based angle acquisition, and Space Vector Pulse Width Modulation (SVPWM) generation. It uses a custom software timer and event-driven architecture for scheduling.

## Core Technologies and Features:
*   **Microcontroller:** STM32G4 series (Cortex-M4).
*   **Operating System/Scheduling:** FreeRTOS (in `IMU_CTRL`), custom `toolkit` for software timers and event handling.
*   **Control Algorithms:** PID control, FOC (Field-Oriented Control), data fusion (IMU and encoders).
*   **Communication:** CAN FD (with custom protocol, CRC, and daemon-based monitoring), UART (for PS4 controller and debugging).
*   **Debugging & Logging:** SEGGER RTT, CmBacktrace for fault handling, custom `debug` module.
*   **Configuration & Memory:** EasyFlash for non-volatile storage, lwmem for dynamic memory management, custom memory pools.
*   **Shell:** `lwshell` for command-line interaction.

# Building and Running

Both projects use CMake for building and require the `gcc-arm-none-eabi` toolchain.

## General Build Process:
To build either project, navigate to its respective directory (`IMU_CTRL` or `Q16_Drive_Opam_clion`) and use CMake:

```bash
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release # Or Debug
cmake --build .
```
Alternatively, a `build.sh` script is available in each project's root directory:
```bash
./build.sh
```

## Flashing to Device:
After building, the generated `.hex` or `.bin` file can be flashed to the STM32 microcontroller using tools like `st-link` or `openocd`. A `flash.sh` script is typically provided for this purpose in each project's root directory:

```bash
./flash.sh
```

# Development Conventions

*   **Language:** C (C17 standard) with some C++ features in `Q16_Drive_Opam_clion`. Assembly is used for fault handlers.
*   **Code Style:** Adherence to `clang-format` is indicated by `.clang-format` files.
*   **Comments:** Mix of Chinese and English comments for code explanation.
*   **Error Handling:** Assertions (`ASSERT`) are used for critical error conditions.
*   **Modularity:** Extensive use of linked lists for managing instances of various modules (e.g., PID controllers, CAN instances).
*   **Task Management:** Tasks are defined either as FreeRTOS tasks (e.g., `StartCaculateTask` in `IMU_CTRL`) or as functions called periodically by custom software timers (e.g., `tk_timer` in `Q16_Drive_Opam_clion`).
*   **Custom Protocols:** Custom binary protocols are used over CAN and UART for efficient data exchange.
*   **Resource Management:** Dedicated modules for memory (`lwmem`, `memory_pool`), event handling (`tk_event`), and daemon supervision (`daemon`).
