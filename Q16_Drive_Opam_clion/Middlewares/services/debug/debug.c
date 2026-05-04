//
// Created by fubingyan on 25-4-12.
//

/**
 * @file    debug.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-04-12
 * @brief   调试模块实现（ESP32风格日志输出）
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "debug.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "public.h"

/* Private constants ---------------------------------------------------------*/

static const char* const s_level_chars[] = {
    "N", /**< NONE */
    "E", /**< ERROR */
    "W", /**< WARN */
    "I", /**< INFO */
    "D", /**< DEBUG */
    "T", /**< TRACE */
};

static const char* const s_level_colors[] = {
    DEBUG_COLOR_RESET,  /**< NONE */
    DEBUG_COLOR_RED,    /**< ERROR - 红色 */
    DEBUG_COLOR_YELLOW, /**< WARN - 黄色 */
    DEBUG_COLOR_GREEN,  /**< INFO - 绿色 */
    DEBUG_COLOR_BLUE,   /**< DEBUG - 蓝色 */
    DEBUG_COLOR_RESET,  /**< TRACE - 无颜色 */
};

/* Private variables ---------------------------------------------------------*/

static char s_format_buffer[DEBUG_DEFAULT_FORMAT_BUFFER_SIZE];
static uint8_t s_debug_tx_buffer[DEBUG_DEFAULT_TX_BUFFER_SIZE];

static kfifo_t s_tx_fifo;
static kfifo_t* s_tx_fifo_ptr = NULL;

static uint8_t s_current_level = DEBUG_DEFAULT_LEVEL;
static bool s_initialized = false;

/* Private function prototypes -----------------------------------------------*/

static uint32_t debug_default_timestamp(void);
static debug_error_t debug_format_output(debug_level_t level, const char* tag,
                                         const char* fmt, va_list args);

/* Exported functions --------------------------------------------------------*/

static debug_config_t s_config = {
    .name = "debug",
    .tx_buffer = s_debug_tx_buffer,
    .tx_buffer_size = sizeof(s_debug_tx_buffer),
    .get_timestamp_cb = debug_default_timestamp,
    .format_buffer_size = DEBUG_DEFAULT_FORMAT_BUFFER_SIZE,
    .default_level = DEBUG_DEFAULT_LEVEL,
    .enable_color = DEBUG_DEFAULT_ENABLE_COLOR,
    .enable_timestamp = DEBUG_DEFAULT_ENABLE_TIMESTAMP,
};

/**
 * @brief 初始化调试模块
 * @param config 配置结构体指针
 * @return 操作结果错误码
 */
debug_error_t debug_init(const debug_config_t* config) {
  if (!config) {
    return DEBUG_ERROR_NULL_PTR;
  }

  if (s_initialized) {
    debug_deinit();
  }

  if (config->get_timestamp_cb) {
    s_config.get_timestamp_cb = config->get_timestamp_cb;
  }

  if (config->name) {
    s_config.name = config->name;
  }

  kfifo_init(&s_tx_fifo, s_config.tx_buffer, s_config.tx_buffer_size, NULL);
  s_tx_fifo_ptr = &s_tx_fifo;

  s_initialized = true;

  return DEBUG_OK;
}

/**
 * @brief 反初始化调试模块
 */
void debug_deinit(void) {
  s_config.name = "debug";
  s_config.tx_buffer = NULL;
  s_config.get_timestamp_cb = NULL;
  s_config.tx_buffer_size = 0;
  s_config.format_buffer_size = DEBUG_DEFAULT_FORMAT_BUFFER_SIZE;
  s_config.default_level = DEBUG_DEFAULT_LEVEL;
  s_config.enable_color = DEBUG_DEFAULT_ENABLE_COLOR;
  s_config.enable_timestamp = DEBUG_DEFAULT_ENABLE_TIMESTAMP;
  s_current_level = DEBUG_DEFAULT_LEVEL;
  s_tx_fifo_ptr = NULL;
  s_initialized = false;
}

/**
 * @brief 检查调试模块是否已初始化
 * @return true表示已初始化，false表示未初始化
 */
bool debug_is_initialized(void) { return s_initialized; }

/**
 * @brief 设置调试级别
 * @param level 调试级别
 * @return 操作结果错误码
 */
debug_error_t debug_set_level(debug_level_t level) {
  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  if (level > DEBUG_LEVEL_TRACE) {
    return DEBUG_ERROR_INVALID_PARAM;
  }

  s_current_level = level;

  return DEBUG_OK;
}

/**
 * @brief 获取当前调试级别
 * @return 当前调试级别
 */
debug_level_t debug_get_level(void) {
  if (!s_initialized) {
    return DEBUG_LEVEL_NONE;
  }

  return (debug_level_t)s_current_level;
}

/**
 * @brief 启用或禁用颜色输出
 * @param enable true启用，false禁用
 * @return 操作结果错误码
 */
debug_error_t debug_set_color_enable(bool enable) {
  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  s_config.enable_color = enable;

  return DEBUG_OK;
}

/**
 * @brief 启用或禁用时间戳
 * @param enable true启用，false禁用
 * @return 操作结果错误码
 */
debug_error_t debug_set_timestamp_enable(bool enable) {
  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  s_config.enable_timestamp = enable;

  return DEBUG_OK;
}

/**
 * @brief 日志输出（ESP32风格）
 * @param level 调试级别
 * @param tag 日志标签
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @return 操作结果错误码
 */
debug_error_t debug_log(debug_level_t level, const char* tag, const char* fmt,
                        ...) {
  if (!fmt) {
    return DEBUG_ERROR_NULL_PTR;
  }

  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  if (level > s_current_level) {
    return DEBUG_OK;
  }

  va_list args;
  va_start(args, fmt);
  debug_error_t result = debug_format_output(level, tag, fmt, args);
  va_end(args);

  return result;
}

/**
 * @brief 十六进制转储输出
 * @param tag 日志标签
 * @param data 数据指针
 * @param len 数据长度
 * @return 操作结果错误码
 */
debug_error_t debug_hexdump(const char* tag, const uint8_t* data,
                            uint32_t len) {
  if (!data) {
    return DEBUG_ERROR_NULL_PTR;
  }

  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  if (len == 0) {
    return DEBUG_OK;
  }

  char* buf = s_format_buffer;
  uint16_t buf_size = s_config.format_buffer_size;
  uint32_t offset = 0;

  for (uint32_t i = 0; i < len; i += 16) {
    offset = 0;

    offset +=
        snprintf(buf + offset, buf_size - offset, "%04lX: ", (unsigned long)i);

    for (uint32_t j = 0; j < 16; j++) {
      if (i + j < len) {
        offset +=
            snprintf(buf + offset, buf_size - offset, "%02X ", data[i + j]);
      } else {
        offset += snprintf(buf + offset, buf_size - offset, "   ");
      }

      if (j == 7) {
        offset += snprintf(buf + offset, buf_size - offset, " ");
      }
    }

    offset += snprintf(buf + offset, buf_size - offset, " |");

    for (uint32_t j = 0; j < 16 && i + j < len; j++) {
      char c = (char)data[i + j];
      if (c >= 0x20 && c <= 0x7E) {
        offset += snprintf(buf + offset, buf_size - offset, "%c", c);
      } else {
        offset += snprintf(buf + offset, buf_size - offset, ".");
      }
    }

    offset += snprintf(buf + offset, buf_size - offset, "|\r\n");

    if (s_tx_fifo_ptr) {
      kfifo_put(s_tx_fifo_ptr, (const uint8_t*)buf, offset);
    }
  }

  return DEBUG_OK;
}

/**
 * @brief 原始数据输出（不添加格式）
 * @param data 数据指针
 * @param len 数据长度
 * @return 操作结果错误码
 */
debug_error_t debug_write(const uint8_t* data, uint32_t len) {
  if (!data) {
    return DEBUG_ERROR_NULL_PTR;
  }

  if (!s_initialized) {
    return DEBUG_ERROR_UNINITIALIZED;
  }

  if (len == 0) {
    return DEBUG_OK;
  }

  if (s_tx_fifo_ptr) {
    kfifo_put(s_tx_fifo_ptr, data, len);
  }

  return DEBUG_OK;
}

/**
 * @brief 获取输出缓冲区中的数据长度
 * @return 缓冲区中的数据长度，未初始化返回0
 */
uint32_t debug_tx_len(void) {
  if (!s_initialized || !s_tx_fifo_ptr) {
    return 0;
  }

  return kfifo_len(s_tx_fifo_ptr);
}

/**
 * @brief 从输出缓冲区取出数据
 * @param buffer 目标缓冲区
 * @param len 期望取出的长度
 * @return 实际取出的长度
 */
uint32_t debug_tx_get(uint8_t* buffer, uint32_t len) {
  if (!buffer || len == 0) {
    return 0;
  }

  if (!s_initialized || !s_tx_fifo_ptr) {
    return 0;
  }

  return kfifo_get(s_tx_fifo_ptr, buffer, len);
}

/**
 * @brief 断言失败处理函数
 * @param func 函数名
 * @param line 行号
 */
void debug_assert_failed(const char* func, uint32_t line) {
  DEBUG_LOGE("ASSERT FAILED: %s:%lu", func ? func : "unknown",
             (unsigned long)line);
  for (;;) {
  }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 默认时间戳获取函数
 * @return 当前时间戳（毫秒）
 */
static uint32_t debug_default_timestamp(void) {
  static uint32_t counter = 0;
  return counter++;
}

/**
 * @brief 格式化并输出日志
 * @param level 调试级别
 * @param tag 日志标签
 * @param fmt 格式化字符串
 * @param args 可变参数列表
 * @return 操作结果错误码
 */
static debug_error_t debug_format_output(debug_level_t level, const char* tag,
                                         const char* fmt, va_list args) {
  char* buf = s_format_buffer;
  uint16_t buf_size = s_config.format_buffer_size;
  uint32_t offset = 0;

  if (level > DEBUG_LEVEL_TRACE) {
    level = DEBUG_LEVEL_TRACE;
  }

  const char* level_char = s_level_chars[level];
  const char* level_color = s_level_colors[level];

  if (tag == NULL) {
    tag = s_config.name;
  }

  if (s_config.enable_color) {
    offset += snprintf(buf + offset, buf_size - offset, "%s", level_color);
  }

  offset += snprintf(buf + offset, buf_size - offset, "%s (", level_char);

  if (s_config.enable_timestamp) {
    uint32_t timestamp = 0;
    if (s_config.get_timestamp_cb) {
      timestamp = s_config.get_timestamp_cb();
    } else {
      timestamp = debug_default_timestamp();
    }
    offset += snprintf(buf + offset, buf_size - offset, "%lu",
                       (unsigned long)timestamp);
  }

  offset += snprintf(buf + offset, buf_size - offset, ") %s: ", tag);

  int msg_len = vsnprintf(buf + offset, buf_size - offset, fmt, args);
  if (msg_len < 0) {
    return DEBUG_ERROR_INVALID_PARAM;
  }

  offset += msg_len;
  if (offset >= buf_size) {
    offset = buf_size - 1;
  }

  if (s_config.enable_color) {
    offset += snprintf(buf + offset, buf_size - offset, "%s", DEBUG_COLOR_RESET);
  }

  offset += snprintf(buf + offset, buf_size - offset, "\r\n");

  if (s_tx_fifo_ptr) {
    kfifo_put(s_tx_fifo_ptr, (const uint8_t*)buf, offset);
  }

  return DEBUG_OK;
}
