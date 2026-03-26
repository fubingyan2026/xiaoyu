/**
 * @brief:  调试模块
 * @FilePath: debug.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-26 19:55:46
 * @LastEditTime: 2025-09-26 20:15:16
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "debug/debug.h"


#include "kfifo/kfifo.h"
#include "usart_receive.h"
#include <stdarg.h>
#include <stdio.h>

/* 外部变量声明 */

/* 静态缓冲区 - 减少栈使用 */
static char bsp_buffer[BSP_PRINTF_BUFF_SIZE];

/* 调试级别配置 */
static debug_level_t current_debug_level = DEBUG_LEVEL_TRACE;

/* FIFO初始化标志 */
static uint8_t fifo_initialized = 0;

/* 颜色输出使能标志 */
static uint8_t color_output_enabled = ENABLE_COLOR_OUTPUT;

/**
 * @brief 初始化调试FIFO（线程安全）
 */
static void debug_fifo_init(void)
{
    if (!fifo_initialized && fifo_usart1_tx == NULL)
    {
        fifo_usart1_tx = kfifo_alloc(1024, NULL);
        fifo_initialized = 1;
    }
}

/**
 * @brief 设置调试级别
 * @param level 调试级别
 */
void debug_set_level(debug_level_t level)
{
    current_debug_level = level;
}

/**
 * @brief 获取当前调试级别
 * @return debug_level_t 当前调试级别
 */
debug_level_t debug_get_level(void)
{
    return current_debug_level;
}

/**
 * @brief 启用或禁用颜色输出
 * @param enable 1启用，0禁用
 */
void debug_enable_color(uint8_t enable)
{
    color_output_enabled = enable;
}

/**
 * @brief 检查颜色输出是否启用
 * @return uint8_t 1启用，0禁用
 */
uint8_t debug_is_color_enabled(void)
{
    return color_output_enabled;
}

/**
 * @brief 格式化输出到串口（支持颜色输出）
 *
 * 此函数采用格式化字符串和可变参数，将数据格式化到静态缓冲区，
 * 然后通过FIFO队列在USART1上传输结果字符串。如果格式化字符串
 * 超过缓冲区大小，则会被截断以适应。
 *
 * @param fmt 格式化字符串（类似printf）
 * @param ... 对应格式化字符串的可变参数列表
 */
void BSP_Printf(const char *fmt, ...)
{
    /* 参数检查 */
    if (fmt == NULL)
        return;

    va_list args;
    /* 格式化字符串 */
    va_start(args, fmt);
    uint16_t len = vsnprintf(bsp_buffer, sizeof(bsp_buffer), fmt, args);
    va_end(args);

    /* 确保字符串以null结尾 */
    if (len >= sizeof(bsp_buffer))
    {
        len = sizeof(bsp_buffer) - 1;
        bsp_buffer[len] = '\0';
    }

    /* 初始化FIFO（如果需要） */
    debug_fifo_init();

    /* 发送数据到FIFO */
    if (fifo_usart1_tx != NULL && len > 0)
    {
        kfifo_put(fifo_usart1_tx, (const unsigned char *)bsp_buffer, len);
    }
}

/**
 * @brief 断言失败处理函数（带颜色输出）
 * @param func 函数名
 * @param line 行号
 */
void Assert_Failed(const char *func, const uint32_t line)
{
    /* 输出断言失败信息 */
#if (ENABLE_DEBUG_PRINT)
    BSP_Printf(COLOR_BOLD COLOR_RED "Assertion failed: %s, line %lu\r\n" COLOR_RESET, func, line);
#endif

    /* 进入死循环 */
    for (;;)
    {
        /* 可以添加看门狗喂狗或其他恢复机制 */
    }
}