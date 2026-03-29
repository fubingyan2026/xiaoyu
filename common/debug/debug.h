#ifndef DEBUG_H
#define DEBUG_H

#include "stdint.h"

/* 调试配置宏 */
#define ENABLE_ASSERT 1            // 启用断言
#define ENABLE_DEBUG_PRINT 1       // 启用调试打印
#define ENABLE_COLOR_OUTPUT 1      // 启用颜色输出
#define ENABLE_FUNCTION_TRACE 1    // 启用函数跟踪
#define ENABLE_PRINTF_USING_RTT 1  // 使用RTT打印(0=使用串口)

/* 缓冲区配置 */
#define BSP_PRINTF_BUFF_SIZE 256
#define MAX_NAME_LEN 8

/* RTOS配置 */
#define USING_RTOS_TYPE RTOS_USING_NONE
#define SEGGER_RTT_PRINTF_TERMINAL 0

#if (ENABLE_DEBUG_PRINT)
#if (ENABLE_PRINTF_USING_RTT)
#include "SEGGER_RTT.h"
#define SEGGER_WRITE SEGGER_RTT_Write
#else
#define SEGGER_WRITE(...)
#endif
void BSP_Printf(const char* fmt, ...);
#else
#define BSP_Printf(...) \
  do {                  \
  } while (0)
#endif

#if (ENABLE_ASSERT)
extern void Assert_Failed(const char* func, uint32_t line);
#define ASSERT(expr) ((expr) ? (void)0 : Assert_Failed(__func__, __LINE__))
#else
#define ASSERT(expr) \
  do {               \
  } while (0)
#endif

/* 调试级别 */
typedef enum {
  DEBUG_LEVEL_ERROR = 0,
  DEBUG_LEVEL_WARN = 1,
  DEBUG_LEVEL_INFO = 2,
  DEBUG_LEVEL_DEBUG = 3,
  DEBUG_LEVEL_TRACE = 4
} debug_level_t;

/* ANSI颜色代码 */
#if (ENABLE_COLOR_OUTPUT && ENABLE_DEBUG_PRINT)
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_GRAY "\033[90m"

#define COLOR_BOLD "\033[1m"
#define COLOR_UNDERLINE "\033[4m"

/* 带颜色的级别标签 */
#define ERROR_TAG COLOR_BOLD COLOR_RED "[ERROR]" COLOR_RESET
#define WARN_TAG COLOR_BOLD COLOR_YELLOW "[WARN] " COLOR_RESET
#define INFO_TAG COLOR_BOLD COLOR_GREEN "[INFO] " COLOR_RESET
#define DEBUG_TAG COLOR_BOLD COLOR_CYAN "[DEBUG]" COLOR_RESET
#define TRACE_TAG COLOR_BOLD COLOR_MAGENTA "[TRACE]" COLOR_RESET
#else
#define COLOR_RESET ""
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_MAGENTA ""
#define COLOR_CYAN ""
#define COLOR_WHITE ""
#define COLOR_GRAY ""
#define COLOR_BOLD ""
#define COLOR_UNDERLINE ""

#define ERROR_TAG "[ERROR]"
#define WARN_TAG "[WARN] "
#define INFO_TAG "[INFO] "
#define DEBUG_TAG "[DEBUG]"
#define TRACE_TAG "[TRACE]"
#endif

/* 条件调试打印宏 */
#if (ENABLE_DEBUG_PRINT)
#define DEBUG_PRINT(level, ...)         \
  do {                                  \
    if (debug_get_level() >= (level)) { \
      BSP_Printf(__VA_ARGS__);          \
      BSP_Printf("\r\n");               \
    }                                   \
  } while (0)

#define DEBUG_ERROR(...) \
  DEBUG_PRINT(DEBUG_LEVEL_ERROR, ERROR_TAG ":" __VA_ARGS__)
#define DEBUG_WARN(...) DEBUG_PRINT(DEBUG_LEVEL_WARN, WARN_TAG ":" __VA_ARGS__)
#define DEBUG_INFO(...) DEBUG_PRINT(DEBUG_LEVEL_INFO, INFO_TAG ":" __VA_ARGS__)
#define DEBUG_DEBUG(...) \
  DEBUG_PRINT(DEBUG_LEVEL_DEBUG, DEBUG_TAG ":" __VA_ARGS__)
#define DEBUG_TRACE(...) \
  DEBUG_PRINT(DEBUG_LEVEL_TRACE, TRACE_TAG ":" __VA_ARGS__)

/* 函数跟踪宏 */
#if (ENABLE_FUNCTION_TRACE)
#define FUNCTION_ENTER() DEBUG_TRACE("--> %s()", __func__)
#define FUNCTION_EXIT() DEBUG_TRACE("<-- %s()", __func__)
#define FUNCTION_EXIT_VAL(value) \
  DEBUG_TRACE("<-- %s() = %ld", __func__, (long)(value))
#else
#define FUNCTION_ENTER() \
  do {                   \
  } while (0)
#define FUNCTION_EXIT() \
  do {                  \
  } while (0)
#define FUNCTION_EXIT_VAL(value) \
  do {                           \
  } while (0)
#endif

/* 打印当前函数名 */
#define PRINT_FUNCTION() DEBUG_TRACE("Current function: %s", __func__)
#define PRINT_FUNCTION_COLOR(color) \
  BSP_Printf(color "Function: %s" COLOR_RESET "\r\n", __func__)

/* 彩色打印函数（直接使用颜色） */
#define PRINT_RED(...) BSP_Printf(COLOR_RED __VA_ARGS__ COLOR_RESET)
#define PRINT_GREEN(...) BSP_Printf(COLOR_GREEN __VA_ARGS__ COLOR_RESET)
#define PRINT_YELLOW(...) BSP_Printf(COLOR_YELLOW __VA_ARGS__ COLOR_RESET)
#define PRINT_BLUE(...) BSP_Printf(COLOR_BLUE __VA_ARGS__ COLOR_RESET)
#define PRINT_CYAN(...) BSP_Printf(COLOR_CYAN __VA_ARGS__ COLOR_RESET)
#define PRINT_MAGENTA(...) BSP_Printf(COLOR_MAGENTA __VA_ARGS__ COLOR_RESET)
#define PRINT_WHITE(...) BSP_Printf(COLOR_WHITE __VA_ARGS__ COLOR_RESET)
#define PRINT_GRAY(...) BSP_Printf(COLOR_GRAY __VA_ARGS__ COLOR_RESET)

#define PRINT_BOLD(...) BSP_Printf(COLOR_BOLD __VA_ARGS__ COLOR_RESET)
#define PRINT_UNDERLINE(...) BSP_Printf(COLOR_UNDERLINE __VA_ARGS__ COLOR_RESET)

#else
#define DEBUG_PRINT(level, ...) \
  do {                          \
  } while (0)
#define DEBUG_ERROR(...) \
  do {                   \
  } while (0)
#define DEBUG_WARN(...) \
  do {                  \
  } while (0)
#define DEBUG_INFO(...) \
  do {                  \
  } while (0)
#define DEBUG_DEBUG(...) \
  do {                   \
  } while (0)
#define DEBUG_TRACE(...) \
  do {                   \
  } while (0)

#define FUNCTION_ENTER() \
  do {                   \
  } while (0)
#define FUNCTION_EXIT() \
  do {                  \
  } while (0)
#define FUNCTION_EXIT_VAL(value) \
  do {                           \
  } while (0)

#define PRINT_FUNCTION() \
  do {                   \
  } while (0)
#define PRINT_FUNCTION_COLOR(color) \
  do {                              \
  } while (0)

#define PRINT_RED(...) \
  do {                 \
  } while (0)
#define PRINT_GREEN(...) \
  do {                   \
  } while (0)
#define PRINT_YELLOW(...) \
  do {                    \
  } while (0)
#define PRINT_BLUE(...) \
  do {                  \
  } while (0)
#define PRINT_CYAN(...) \
  do {                  \
  } while (0)
#define PRINT_MAGENTA(...) \
  do {                     \
  } while (0)
#define PRINT_WHITE(...) \
  do {                   \
  } while (0)
#define PRINT_GRAY(...) \
  do {                  \
  } while (0)
#define PRINT_BOLD(...) \
  do {                  \
  } while (0)
#define PRINT_UNDERLINE(...) \
  do {                       \
  } while (0)
#endif

/* 函数声明 */
void debug_set_level(debug_level_t level);
debug_level_t debug_get_level(void);
void debug_enable_color(uint8_t enable);
uint8_t debug_is_color_enabled(void);
void debug_enable_function_trace(uint8_t enable);
uint8_t debug_is_function_trace_enabled(void);

#endif  // DEBUG_H
