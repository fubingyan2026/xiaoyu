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
aux_source_directory(${CMAKE_SOURCE_DIR}/protocol_parser PROTOCOL_PARSER)
aux_source_directory(${CMAKE_SOURCE_DIR}/daemon DAEMON)
aux_source_directory(${CMAKE_SOURCE_DIR}/message_center MESSAGE_CENTER)

# common目录下的所有子目录 (手动处理fsm以排除示例文件)
foreach(subdir algorithm controller crc encoder keybase kfifo memory_pool toolkit)
    aux_source_directory(${CMAKE_SOURCE_DIR}/../common/${subdir} COMMON_${subdir})
endforeach()

# 本地can_comm目录
aux_source_directory(${CMAKE_SOURCE_DIR}/can_comm CAN_COMM)

# 特殊处理的模块 (有子目录的)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/SEGGER_RTT/RTT COMMON_SEGGER_RTT)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/easyflash/src COMMON_easyflash)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/CmBacktrace/cm_backtrace COMMON_CMBT)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/lwmem/src/lwmem COMMON_lwmem)
aux_source_directory(${CMAKE_SOURCE_DIR}/../common/lwshell/src COMMON_lwshell)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${APP}
        ${HAL}
        ${FOC}
        ${PROTOCOL_PARSER}
        ${DAEMON}
        ${COMMON_algorithm}
        ${CAN_COMM}
        ${COMMON_controller}
        ${COMMON_crc}
        ${COMMON_encoder}
        ${COMMON_keybase}
        ${COMMON_kfifo}
        ${COMMON_lwmem}
        ${COMMON_lwshell}
        ${COMMON_memory_pool}
        ${MESSAGE_CENTER}
        ${COMMON_protocol}
        ${COMMON_toolkit}
        ${COMMON_easyflash}
        ${COMMON_SEGGER_RTT}
        ${COMMON_CMBT}

        debug/debug.c
        # LED模块
        led/led.c
        # FSM模块
        ../common/fsm/fsm.c
        # 汇编文件需要单独添加
        ../common/CmBacktrace/cm_backtrace/fault_handler/gcc/cmb_fault.S
        # 单独的源文件
        ../common/easyflash/port/ef_port.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        applications
        hal_drivers
        Q16_FOC
        can_comm
        led
        protocol_parser
        daemon
        message_center
        debug
        ../common
        ../common/CmBacktrace/cm_backtrace
        ../common/SEGGER_RTT/RTT/config
        ../common/SEGGER_RTT/RTT
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