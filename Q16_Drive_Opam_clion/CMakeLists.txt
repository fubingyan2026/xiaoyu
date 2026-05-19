cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

add_compile_options(
        -Wno-unused-function      # 未使用的函数
        -Wno-unused-parameter     # 未使用的形参
        -Wno-unused-variable      # 未使用的变量
        -Wno-unused-but-set-variable  # 设了值但从未读取
)

# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Set the project name
set(CMAKE_PROJECT_NAME Q16_Drive_Opam_clion)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# 强制Ninja使用彩色输出
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "OFF" CACHE BOOL "" FORCE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM CXX)
string(REPLACE "-Wpedantic" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})
# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths
)

# 批量收集源代码目录
aux_source_directory(${CMAKE_SOURCE_DIR}/applications APP)
aux_source_directory(${CMAKE_SOURCE_DIR}/bsp_drivers DRIVER)
aux_source_directory(${CMAKE_SOURCE_DIR}/hal_drivers HAL)
aux_source_directory(${CMAKE_SOURCE_DIR}/Q16_FOC FOC)

# Middlewares 目录
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/algorithm MID_algorithm)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/algorithm/pll MID_pll)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/algorithm/controller MID_controller)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/algorithm/filter MID_filter)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/algorithm/math MID_math)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/key_base MID_key_base)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/fsm MID_fsm)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/message_center MID_message_center)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/protocol MID_protocol)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/services/can_comm MID_can_comm)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/services/daemon MID_daemon)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/services/debug MID_debug)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/services/led MID_led)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/utils MID_utils)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/utils/toolkit MID_utils_toolkit)

# Third_Party 模块
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/SEGGER_RTT/RTT MID_SEGGER_RTT)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/easyflash/src MID_easyflash)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/CmBacktrace/cm_backtrace MID_CMBT)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lwmem/src/lwmem MID_lwmem)
aux_source_directory(${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lwshell/src MID_lwshell)

# common 目录 (仅保留 toolkit)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/toolkit COMMON_toolkit)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${APP}
        ${HAL}
        ${FOC}
        ${MID_algorithm}
        ${MID_pll}
        ${MID_controller}
        ${MID_filter}
        ${MID_math}
        ${MID_key_base}
        ${MID_fsm}
        ${MID_message_center}
        ${MID_protocol}
        ${MID_can_comm}
        ${MID_daemon}
        ${MID_debug}
        ${MID_led}
        ${MID_utils}
        ${MID_SEGGER_RTT}
        ${MID_easyflash}
        ${MID_CMBT}
        ${MID_lwmem}
        ${MID_lwshell}
        ${MID_utils_toolkit}

        # 汇编文件需要单独添加
        Middlewares/Third_Party/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.S
        # 单独的源文件
        Middlewares/Third_Party/easyflash/port/ef_port.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        applications
        hal_drivers
        Q16_FOC
        Middlewares
        Middlewares/algorithm
        Middlewares/algorithm/pll
        Middlewares/algorithm/controller
        Middlewares/algorithm/filter
        Middlewares/algorithm/math
        Middlewares/key_base
        Middlewares/fsm
        Middlewares/message_center
        Middlewares/protocol
        Middlewares/services/can_comm
        Middlewares/services/daemon
        Middlewares/services/debug
        Middlewares/services/led
        Middlewares/utils
        Middlewares/utils/toolkit

        Middlewares/Third_Party/CmBacktrace/cm_backtrace
        Middlewares/Third_Party/SEGGER_RTT/RTT/config
        Middlewares/Third_Party/SEGGER_RTT/RTT
        Middlewares/Third_Party/lwmem/include
        Middlewares/Third_Party/lwshell/inc
        Middlewares/Third_Party/easyflash/inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        # Add user defined libraries
)

# 生成Bin文件
add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ${CMAKE_PROJECT_NAME}.elf -Oihex ${CMAKE_PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} ${CMAKE_PROJECT_NAME}.elf -Obinary ${CMAKE_PROJECT_NAME}.bin
        COMMENT "Generating hex || bin file for STM32"
)