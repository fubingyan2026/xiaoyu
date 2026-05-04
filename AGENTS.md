# xiaoyu — STM32G4 Firmware Monorepo

Two independent STM32G431 (Cortex-M4) firmware projects + shared embedded C libraries.

## Top-level layout

| Path | Purpose |
|------|---------|
| `common/` | Shared embedded C libraries (algorithm, controller/PID, encoder, easyflash, debug, crc, fsm, lwmem, lwshell, toolkit, ...) |
| `IMU_CTRL/` | FreeRTOS gimbal stabilization — MPU6500 IMU, PS4 controller input, CAN bus motor control |
| `Q16_Drive_Opam_clion/` | FOC motor driver — field-oriented control, SVPWM, Q16 fixed-point math, Hall/encoder sensor |

## Build

Both projects use **CMake + Ninja** with `arm-none-eabi-gcc`.

```bash
# Per project — configure + build (default RelWithDebInfo)
cd IMU_CTRL && cmake --preset RelWithDebInfo && cmake --build --preset RelWithDebInfo
cd Q16_Drive_Opam_clion && cmake --preset RelWithDebInfo && cmake --build --preset RelWithDebInfo

# Or use the convenience wrapper (auto-detects project root)
./IMU_CTRL/build.sh
./Q16_Drive_Opam_clion/build.sh
# With custom type:  ./build.sh -t Debug
```

Presets available: `Debug`, `RelWithDebInfo`, `Release`, `MinSizeRel`.

Output: `.elf` → `.hex` + `.bin` (post-build custom command in CMakeLists.txt).

**Compilation database** for clangd: `build/RelWithDebInfo/compile_commands.json` in each project.

## Flash

| Project | Flash script | Tool | Address |
|---------|-------------|------|---------|
| `IMU_CTRL/` | `flash.sh` | STM32_Programmer_CLI (ST-Link) | `0x08005000` |
| `Q16_Drive_Opam_clion/` | `flash_stlink.sh` | STM32_Programmer_CLI (ST-Link) | `0x08000000` |
| `Q16_Drive_Opam_clion/` | `flash_jlink.sh` | JLinkExe (SEGGER J-Link) | `0x08000000` |

Both projects share `stlink.cfg` for OpenOCD.

## Tests

Only `IMU_CTRL/` has tests — **GoogleTest** for pure algorithm code only (no HAL dependency).

```bash
cd IMU_CTRL
cmake -B tests/build -S tests -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tests/build
./tests/build/run_tests
```

Tests cover `common/controller/pid.c`, `common/algorithm/maths.c`, `common/algorithm/filter.c`.  
GoogleTest v1.14.0 is fetched automatically via FetchContent.

## Code style

| Project | Style | Language |
|---------|-------|----------|
| `IMU_CTRL/` | Microsoft (`.clang-format: BasedOnStyle: Microsoft`) | C17 |
| `Q16_Drive_Opam_clion/` | Google (`.clang-format: BasedOnStyle: Google, IndentWidth: 2`) | C17 + C++17 |

## Toolchain quirks

- Compiler warnings suppressed project-wide: `-Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable`
- clangd configured in both projects — compilation database at `build/RelWithDebInfo`, `unused-includes` diagnostic suppressed
- C++ enabled **only** in `Q16_Drive_Opam_clion` (`enable_language(C ASM CXX)`); `IMU_CTRL` is pure C (`enable_language(C ASM)`)
- No RTTI, no exceptions, no threadsafe statics in C++ (`-fno-rtti -fno-exceptions -fno-threadsafe-statics`)
- Linker: nano specs, `--gc-sections`, print memory usage
- MCU flags: `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`

## Shared libraries

`common/` contains reusable embedded modules. The `IMU_CTRL` project references them directly via `../common/` in its CMakeLists.txt. The `Q16_Drive_Opam_clion` project maintains its own copy under `Middlewares/` (algorithm, services, Third_Party subdirs) and also links a subset from `../common/toolkit`.

Key modules (see each directory's own `AGENTS.md` for detail):
- `algorithm/` — Q16 fixed-point math, filters, kinematics
- `controller/` — position/delta PID controllers
- `encoder/` — MT6816, AS5047P, linear Hall
- `easyflash/` — wear-leveled flash storage for calibration data
- `memory_pool/` — no-malloc embedded memory management
- `toolkit/` — timers, queues, event utilities

## Existing per-module AGENTS.md references

- `IMU_CTRL/AGENTS.md` — tasks, priorities, unit test details
- `common/AGENTS.md`, `common/algorithm/AGENTS.md`, `common/easyflash/AGENTS.md`
- `Q16_Drive_Opam_clion/Q16_FOC/AGENTS.md` — FOC control modules
- `Q16_Drive_Opam_clion/Core/AGENTS.md` — CubeMX generated code conventions
- `Q16_Drive_Opam_clion/Middlewares/algorithm/AGENTS.md`
- `Q16_Drive_Opam_clion/Middlewares/Third_Party/easyflash/AGENTS.md`
