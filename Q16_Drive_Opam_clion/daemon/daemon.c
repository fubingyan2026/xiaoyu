//
// Created by fubingyan on 25-4-5.
//

/**
 * @file    daemon.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2025-04-05
 * @brief   守护进程模块实现
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
#include "daemon.h"

#include <string.h>

#include "memory_pool/memory_pool.h"

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static daemon_context_t* s_daemon_master = NULL;
static bool s_daemon_initialized = false;
static uint16_t s_daemon_count = 0;
static daemon_get_time_cb_t s_get_time_cb = NULL;

/* Private function prototypes -----------------------------------------------*/

static inline uint32_t daemon_get_time(void);
static inline uint32_t daemon_time_diff(uint32_t new_time, uint32_t old_time);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化守护进程系统
 * @param get_time_cb 获取系统时间戳的回调函数
 * @return 操作结果错误码
 */
daemon_error_t daemon_init(daemon_get_time_cb_t get_time_cb) {
  if (get_time_cb == NULL) {
    return DAEMON_ERROR_NULL_PTR;
  }

  if (s_daemon_initialized) {
    return DAEMON_OK_EXISTED;
  }

  s_get_time_cb = get_time_cb;
  s_daemon_master = NULL;
  s_daemon_count = 0;
  s_daemon_initialized = true;

  return DAEMON_OK;
}

/**
 * @brief 反初始化守护进程系统
 */
void daemon_deinit(void) {
  if (!s_daemon_initialized) {
    return;
  }

  daemon_context_t* current = s_daemon_master;
  daemon_context_t* next;

  while (current != NULL) {
    next = current->next;
    if (!current->is_static) {
      __free(current);
    }
    current = next;
  }

  s_daemon_master = NULL;
  s_daemon_count = 0;
  s_daemon_initialized = false;
}

/**
 * @brief 获取守护进程实例
 * @param name 守护进程名称
 * @return 守护进程上下文指针，未找到返回NULL
 */
daemon_context_t* daemon_get_instance(const char* name) {
  if (name == NULL || !s_daemon_initialized) {
    return NULL;
  }

  daemon_context_t* current = s_daemon_master;
  while (current != NULL) {
    if (strcmp(current->config.name, name) == 0) {
      return current;
    }
    current = current->next;
  }

  return NULL;
}

/**
 * @brief 注册守护进程(静态分配)
 * @param config 配置结构体指针
 * @param instance 预先分配的实例内存
 * @return 操作结果错误码
 */
daemon_error_t daemon_register_static(const daemon_config_t* config,
                                      daemon_context_t* instance) {
  if (config == NULL || config->name == NULL || instance == NULL) {
    return DAEMON_ERROR_NULL_PTR;
  }

  if (!s_daemon_initialized) {
    return DAEMON_ERROR_UNINITIALIZED;
  }

  daemon_context_t* existing = daemon_get_instance(config->name);
  if (existing != NULL) {
    return DAEMON_ERROR_ALREADY_EXIST;
  }

  memset(instance, 0, sizeof(daemon_context_t));
  memcpy(&instance->config, config, sizeof(daemon_config_t));

  instance->current_feed_time = daemon_get_time();
  instance->last_feed_time = instance->current_feed_time;
  instance->online_last = true;
  instance->online = true;
  instance->is_static = true;

  instance->next = s_daemon_master;
  s_daemon_master = instance;
  s_daemon_count++;

  return DAEMON_OK;
}

/**
 * @brief 注册守护进程(动态分配)
 * @param config 配置结构体指针
 * @return 守护进程上下文指针，失败返回NULL
 */
daemon_context_t* daemon_register(const daemon_config_t* config) {
  if (config == NULL || config->name == NULL) {
    return NULL;
  }

  if (!s_daemon_initialized) {
    return NULL;
  }

  daemon_context_t* existing = daemon_get_instance(config->name);
  if (existing != NULL) {
    return existing;
  }

  daemon_context_t* new_daemon =
      (daemon_context_t*)__malloc(sizeof(daemon_context_t));
  if (new_daemon == NULL) {
    return NULL;
  }

  memset(new_daemon, 0, sizeof(daemon_context_t));
  memcpy(&new_daemon->config, config, sizeof(daemon_config_t));

  new_daemon->current_feed_time = daemon_get_time();
  new_daemon->last_feed_time = new_daemon->current_feed_time;
  new_daemon->online_last = true;
  new_daemon->online = true;
  new_daemon->is_static = false;

  new_daemon->next = s_daemon_master;
  s_daemon_master = new_daemon;
  s_daemon_count++;

  return new_daemon;
}

/**
 * @brief 注销守护进程
 * @param name 守护进程名称
 * @return 操作结果错误码
 */
daemon_error_t daemon_unregister(const char* name) {
  if (name == NULL) {
    return DAEMON_ERROR_NULL_PTR;
  }

  if (!s_daemon_initialized) {
    return DAEMON_ERROR_NOT_FOUND;
  }

  daemon_context_t** ptr = &s_daemon_master;
  while (*ptr != NULL) {
    if (strcmp((*ptr)->config.name, name) == 0) {
      daemon_context_t* to_remove = *ptr;
      *ptr = (*ptr)->next;

      if (!to_remove->is_static) {
        __free(to_remove);
      }

      s_daemon_count--;
      return DAEMON_OK;
    }
    ptr = &(*ptr)->next;
  }

  return DAEMON_ERROR_NOT_FOUND;
}

/**
 * @brief 获取守护进程数量
 * @return 守护进程数量
 */
uint16_t daemon_get_count(void) { return s_daemon_count; }

/**
 * @brief 获取守护进程主指针
 * @return 守护进程链表头指针
 */
daemon_context_t* daemon_get_master_pointer(void) {
  if (!s_daemon_initialized) {
    return NULL;
  }
  return s_daemon_master;
}

/**
 * @brief 重新加载守护进程(喂狗)
 * @param ctx 守护进程上下文指针
 */
void daemon_reload(daemon_context_t* ctx) {
  if (ctx == NULL) {
    return;
  }

  ctx->last_feed_time = ctx->current_feed_time;
  ctx->current_feed_time = daemon_get_time();
}

/**
 * @brief 检查守护进程是否在线
 * @param ctx 守护进程上下文指针
 * @return true表示在线，false表示离线
 */
bool daemon_is_online(const daemon_context_t* ctx) {
  if (ctx == NULL) {
    return false;
  }
  return ctx->online;
}

/**
 * @brief 获取守护进程名称
 * @param ctx 守护进程上下文指针
 * @return 守护进程名称，失败返回"not_found!"
 */
const char* daemon_get_name(const daemon_context_t* ctx) {
  if (ctx == NULL) {
    return "not_found!";
  }
  return ctx->config.name;
}

/**
 * @brief 守护进程任务函数
 * @note 需在主循环或定时器中定期调用
 */
void daemon_task(void) {
  if (!s_daemon_initialized || s_daemon_master == NULL) {
    return;
  }

  static uint32_t last_time = 0;
  uint32_t current_time = daemon_get_time();
  uint32_t diff_time = daemon_time_diff(current_time, last_time);
  last_time = current_time;

  if (diff_time == 0) {
    diff_time = 1;
  }

  daemon_context_t* current = s_daemon_master;
  while (current != NULL) {
    if (current->init_wait_counter <= current->config.init_wait_time_ms) {
      current->init_wait_counter += diff_time;
      if (current->init_wait_counter > current->config.init_wait_time_ms) {
        current->online = true;
      }
    } else if (daemon_time_diff(current_time, current->current_feed_time) >
               current->config.reload_timeout_ms) {
      if (current->config.reload_timeout_ms == 0xFFFF ||
          current->config.reload_timeout_ms == 0) {
        current->online = true;
      } else {
        current->online = false;
      }
      current->rx_counter = 0;
    } else {
      uint32_t threshold = (diff_time > 0)
                               ? (DAEMON_STABLE_TIMES_MS / diff_time)
                               : DAEMON_STABLE_TIMES_MS;
      if (threshold == 0) {
        threshold = 1;
      }

      if (++current->rx_counter >= threshold) {
        current->rx_counter = 0;
        current->online = true;
      }
    }

    if (current->current_feed_time > current->last_feed_time) {
      uint32_t time_diff =
          daemon_time_diff(current->current_feed_time, current->last_feed_time);
      if (time_diff > 0) {
        current->feed_frequency = 1000.0f / (float)time_diff;
      }
    }

    if (current->online_last != current->online) {
      if (current->config.offline_cb != NULL) {
        current->config.offline_cb(current->config.owner_ptr);
      }
      current->online_last = current->online;
    }

    current = current->next;
  }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 获取系统时间戳
 * @return 系统时间戳(毫秒)
 */
static inline uint32_t daemon_get_time(void) {
  if (s_get_time_cb != NULL) {
    return s_get_time_cb();
  }
  return 0;
}

/**
 * @brief 计算时间差，安全处理32位时间戳溢出回绕
 * @param new_time 新时间戳
 * @param old_time 旧时间戳
 * @return 时间差(毫秒)
 */
static inline uint32_t daemon_time_diff(uint32_t new_time, uint32_t old_time) {
  return (new_time >= old_time) ? (new_time - old_time)
                                : (0xFFFFFFFF - old_time + new_time + 1);
}
