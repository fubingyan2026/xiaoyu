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
set(CMAKE_PROJECT_NAME IMU_CTRL)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# 批量收集源代码目录
aux_source_directory(${CMAKE_SOURCE_DIR}/hal_drivers HAL_DRIVER)
aux_source_directory(${CMAKE_SOURCE_DIR}/applications APP)

# common目录下的所有子目录 (手动处理fsm以排除示例文件)
foreach(subdir algorithm can_comm controller crc daemon debug encoder fsm keybase kfifo ledshow memory_pool message_center protocol toolkit)
    aux_source_directory(${CMAKE_SOURCE_DIR}/../common/${subdir} COMMON_${subdir})
endforeach()

# 特殊处理的模块 (有子目录的)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/SEGGER_RTT/RTT SEGGER)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/easyflash/src FLASH)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/CmBacktrace/cm_backtrace CM_B_T)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/lwmem/src/lwmem COMMON_lwmem)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/lwshell/src COMMON_lwshell)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${APP}
    ${HAL_DRIVER}
    ${COMMON_algorithm}
    ${COMMON_can_comm}
    ${COMMON_controller}
    ${COMMON_crc}
    ${COMMON_daemon}
    ${COMMON_debug}
    # ${COMMON_encoder}
    ${COMMON_keybase}
    ${COMMON_kfifo}
    ${COMMON_ledshow}
    ${COMMON_lwmem}
    ${COMMON_lwshell}
    ${COMMON_memory_pool}
    ${COMMON_message_center}
    ${COMMON_protocol}
    ${COMMON_toolkit}
    ${FLASH}
    ${SEGGER}
    ${CM_B_T}

    # # 汇编文件需要单独添加
    ../common/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.S

    # 单独的源文件
    ./port/ef_port.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    applications
    hal_drivers
    ../common
    ../common/CmBacktrace/cm_backtrace
    ../common/SEGGER_RTT/RTT/config
    ../common/lwmem/include
    ../common/lwshell/inc
    ../common/easyflash/inc
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
