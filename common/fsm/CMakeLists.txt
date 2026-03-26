cmake_minimum_required(VERSION 3.10)

# 设置使用MSVC编译器
if(MSVC)
    set(CMAKE_C_COMPILER "cl.exe")
    set(CMAKE_CXX_COMPILER "c++.exe")
    # 添加UTF-8编码支持，解决中文乱码
    add_compile_options(/utf-8)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
    # 添加UTF-8源文件输入，输出为 UTF-8
    add_compile_options(-fexec-charset=GB2312)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset=UTF-8 -fexec-charset=GB2312")
endif()

project(fsm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(fsm_example fsm_example.c fsm.c fsm.h fsm_config.h)
