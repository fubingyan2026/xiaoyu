//
// Created by fubingyan on 25-4-12.
//

/**
 * @file    key_base.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-04-12
 * @brief   按键状态机模块实现
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
#include "key_base.h"

#include <limits.h>
#include <string.h>

#include "debug.h"
#include "memory_pool/memory_pool.h"

/* Private constants ---------------------------------------------------------*/

#define KEY_BASE_DEBOUNCE_TIME_MS 50
#define KEY_BASE_MIN_TIME_THRESHOLD_MS 10
#define KEY_BASE_MAX_TIME_THRESHOLD_MS 60000

/* Private variables ---------------------------------------------------------*/

static key_base_context_t* s_key_master = NULL;
static uint16_t s_key_count = 0;
static bool s_system_initialized = false;

/* Private function prototypes -----------------------------------------------*/

static uint32_t key_base_time_diff(uint32_t new_time, uint32_t old_time);
static void key_base_debounce_process(key_base_context_t* ctx,
                                      uint32_t current_time);
static void key_base_combination_process(key_base_context_t* ctx);
static void key_base_state_machine_process(key_base_context_t* ctx,
                                           uint32_t current_time);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 计算时间差（处理溢出）
 * @param new_time 新时间
 * @param old_time 旧时间
 * @return 时间差
 */
static uint32_t key_base_time_diff(uint32_t new_time, uint32_t old_time) {
  return (new_time >= old_time) ? (new_time - old_time)
                                : (UINT32_MAX - old_time + new_time + 1);
}

/**
 * @brief 消抖处理函数
 * @param ctx 按键上下文指针
 * @param current_time 当前时间（由调用方缓存）
 */
static void key_base_debounce_process(key_base_context_t* ctx,
                                      uint32_t current_time) {
  ctx->data.timer = current_time;

  ctx->data.diff_timer = key_base_time_diff(current_time, ctx->data.last_timer);

  ctx->data.last_pin_state = ctx->data.pin_state;
  ctx->data.last_timer = current_time;

  if (ctx->data.diff_timer == 0) {
    return;
  }

  uint8_t current_pin = ctx->config.read_pin_cb();

  if (current_pin) {
    if (ctx->data.pin_state != KEY_BASE_PIN_STATE_PRESS) {
      if (ctx->data.press_debounce_count == 0) {
        ctx->data.press_debounce_start = current_time;
      }
      ctx->data.press_debounce_count++;
      if (ctx->data.diff_timer >= KEY_BASE_DEBOUNCE_TIME_MS ||
          ctx->data.press_debounce_count >= 3) {
        ctx->data.press_debounce_count = 0;
        ctx->data.pin_state = KEY_BASE_PIN_STATE_PRESS;
        ctx->data.press_start_time = current_time;
      }
    }
    ctx->data.release_debounce_count = 0;
  } else {
    if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
      if (ctx->data.release_debounce_count == 0) {
        ctx->data.release_debounce_start = current_time;
      }
      ctx->data.release_debounce_count++;
      if (ctx->data.diff_timer >= KEY_BASE_DEBOUNCE_TIME_MS ||
          ctx->data.release_debounce_count >= 3) {
        ctx->data.release_debounce_count = 0;
        ctx->data.pin_state = KEY_BASE_PIN_STATE_RELEASE;
      }
    }
    ctx->data.press_debounce_count = 0;
  }
}

/**
 * @brief 组合按键处理函数
 * @param ctx 按键上下文指针
 */
static void key_base_combination_process(key_base_context_t* ctx) {
  if (ctx->config.event_callback == NULL) {
    return;
  }

  if (ctx->combination_partner) {
    key_base_context_t* partner = ctx->combination_partner;
    if (partner->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
      if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS &&
          !ctx->data.last_pin_state) {
        ctx->data.combination_active = true;
        partner->data.combination_active = true;
        ctx->data.key_event = KEY_BASE_EVENT_COMBINATION;
        ctx->config.event_callback(ctx->data.key_event, ctx);
        ctx->data.combination_handled = true;
        partner->data.combination_handled = true;
        ctx->data.combination_long_handled = false;
      } else if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS &&
                 ctx->data.last_pin_state == KEY_BASE_PIN_STATE_PRESS) {
        if (!ctx->data.combination_long_handled) {
          const uint32_t effective_long_press_time =
              (ctx->config.long_press_time_ms < KEY_BASE_MIN_TIME_THRESHOLD_MS)
                  ? KEY_BASE_MIN_TIME_THRESHOLD_MS
                  : ctx->config.long_press_time_ms;
          if (key_base_time_diff(ctx->data.timer, ctx->data.press_start_time) >=
              effective_long_press_time) {
            ctx->data.combination_long_handled = true;
            ctx->data.key_event = KEY_BASE_EVENT_COMBINATION_LONG;
            ctx->config.event_callback(ctx->data.key_event, ctx);
          }
        }
      }
    }
  }

  if (ctx->data.combination_active &&
      ctx->data.pin_state == KEY_BASE_PIN_STATE_RELEASE &&
      ctx->data.last_pin_state == KEY_BASE_PIN_STATE_PRESS) {
    ctx->data.combination_active = false;
    ctx->data.combination_handled = false;
    ctx->data.combination_long_handled = false;
    if (ctx->combination_partner) {
      ctx->combination_partner->data.combination_active = false;
      ctx->combination_partner->data.combination_handled = false;
      ctx->combination_partner->data.combination_long_handled = false;
    }
  }
}

/**
 * @brief 状态机处理函数
 * @param ctx 按键上下文指针
 * @param current_time 当前时间（由调用方缓存）
 */
static void key_base_state_machine_process(key_base_context_t* ctx,
                                           uint32_t current_time) {
  if (ctx->config.event_callback == NULL) {
    return;
  }

  if (ctx->data.combination_active && ctx->data.combination_handled) {
    return;
  }

  ctx->data.timer = current_time;

  const uint32_t effective_long_press_time =
      (ctx->config.long_press_time_ms < KEY_BASE_MIN_TIME_THRESHOLD_MS)
          ? KEY_BASE_MIN_TIME_THRESHOLD_MS
          : ctx->config.long_press_time_ms;
  const uint32_t cooling_window = effective_long_press_time / 2;

  if (ctx->data.post_long_release_time &&
      key_base_time_diff(ctx->data.timer, ctx->data.post_long_release_time) <
          cooling_window) {
    if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
      return;
    }
  }

  const uint32_t click_window = (ctx->config.multi_click_time_ms > 0)
                                    ? ctx->config.multi_click_time_ms
                                    : effective_long_press_time;

  if (ctx->data.last_pin_state != ctx->data.pin_state) {
    ctx->data.last_pin_state = ctx->data.pin_state;
    if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
      if (key_base_time_diff(ctx->data.timer, ctx->data.press_time) >=
          effective_long_press_time) {
        ctx->data.key_event = KEY_BASE_EVENT_LONG_WAIT_PRESS;
        ctx->config.event_callback(ctx->data.key_event, ctx);
      } else {
        ctx->data.key_event = KEY_BASE_EVENT_CLICK;
        ctx->config.event_callback(ctx->data.key_event, ctx);
      }
    } else {
      ctx->data.press_time = ctx->data.timer;
      if (ctx->data.long_hold_state) {
        ctx->data.long_hold_state = false;
        ctx->data.key_event = KEY_BASE_EVENT_LONG_HOLD_RELEASE;
        ctx->data.batter_event = KEY_BASE_BATTER_STATE_IDLE;
        ctx->config.event_callback(ctx->data.key_event, ctx);
      } else {
        ctx->data.key_event = KEY_BASE_EVENT_DOWN;
        ctx->config.event_callback(ctx->data.key_event, ctx);
      }
    }
  } else {
    if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
      if (!ctx->data.long_hold_state) {
        if (key_base_time_diff(ctx->data.timer, ctx->data.press_start_time) >=
            effective_long_press_time) {
          ctx->data.long_hold_state = true;
          ctx->data.key_event = KEY_BASE_EVENT_LONG_HOLD;
          ctx->data.post_long_release_time = ctx->data.timer;
          ctx->config.event_callback(ctx->data.key_event, ctx);
        }
      }
    }
  }

  switch (ctx->data.batter_event) {
    case KEY_BASE_BATTER_STATE_IDLE:
      if (ctx->data.last_key_event != ctx->data.key_event) {
        if (ctx->data.pin_state == KEY_BASE_PIN_STATE_PRESS) {
          ctx->data.batter_counts = 0;
          ctx->data.batter_event = KEY_BASE_BATTER_STATE_WAIT;
          ctx->data.batter_reset_time = ctx->data.timer;
        }
      }
      break;

    case KEY_BASE_BATTER_STATE_WAIT:
      if (key_base_time_diff(ctx->data.timer, ctx->data.batter_reset_time) <=
          click_window) {
        if (ctx->data.last_key_event != ctx->data.key_event) {
          ctx->data.batter_reset_time = ctx->data.timer;
          if (ctx->data.key_event == KEY_BASE_EVENT_DOWN) {
            ctx->data.batter_counts++;
          }
        }
      } else {
        if (ctx->data.pin_state == KEY_BASE_PIN_STATE_RELEASE) {
          if (ctx->data.batter_counts > 0) {
            static const key_base_event_t click_map[] = {
                [0] = KEY_BASE_EVENT_ONE_CLICK,
                [1] = KEY_BASE_EVENT_DOUBLE_CLICK,
                [2] = KEY_BASE_EVENT_TRIPLE_CLICK,
                [3] = KEY_BASE_EVENT_REPEAT_CLICK};
            const uint8_t cnt =
                ctx->data.batter_counts - 1U > sizeof(click_map) - 1U
                    ? sizeof(click_map) - 1U
                    : ctx->data.batter_counts - 1U;
            ctx->data.key_event = click_map[cnt];
            ctx->config.event_callback(ctx->data.key_event, ctx);
          }
          ctx->data.batter_counts = 0;
          ctx->data.batter_event = KEY_BASE_BATTER_STATE_IDLE;
        }
      }
      break;

    default:
      break;
  }

  ctx->data.last_key_event = ctx->data.key_event;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化按键系统
 */
void key_base_init(void) {
  if (s_system_initialized) {
    return;
  }
  s_key_master = NULL;
  s_key_count = 0;
  s_system_initialized = true;
  DEBUG_LOGI("key_base", "Key system initialized");
}

/**
 * @brief 反初始化按键系统，释放所有资源
 */
void key_base_deinit(void) {
  if (!s_system_initialized) {
    return;
  }

  key_base_context_t* current = s_key_master;
  while (current) {
    key_base_context_t* next = current->next;

    if (current->combination_partner) {
      current->combination_partner->combination_partner = NULL;
    }

    if (!current->is_static) {
      __free(current);
    }

    current = next;
  }

  s_key_master = NULL;
  s_key_count = 0;
  s_system_initialized = false;
  DEBUG_LOGI("key_base", "Key system deinitialized");
}

/**
 * @brief 获取按键实例数量
 * @return 按键实例数量
 */
uint16_t key_base_get_count(void) { return s_key_count; }

/**
 * @brief 注册按键实例（动态内存版本）
 * @param config 按键配置指针
 * @param instance 输出参数，返回注册的按键实例指针（可为NULL）
 * @return 错误码
 */
key_base_error_t key_base_register(const key_base_config_t* config,
                                   key_base_context_t** instance) {
  if (!config) {
    DEBUG_LOGI("key_base", "key_base_register failed: config is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!config->name) {
    DEBUG_LOGI("key_base", "key_base_register failed: config->name is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!s_system_initialized) {
    key_base_init();
  }

  key_base_context_t* existing = key_base_get_instance(config->name);
  if (existing) {
    DEBUG_LOGI("key_base",
               "key_base_register warning: key %s already exists, return "
               "existing instance",
               config->name);
    if (instance) {
      *instance = existing;
    }
    return KEY_BASE_OK_EXISTED;
  }

  if (config->read_pin_cb == NULL) {
    DEBUG_LOGI("key_base", "key_base_register failed: read_pin_cb is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (config->event_callback == NULL) {
    DEBUG_LOGI("key_base", "key_base_register failed: event_callback is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (config->get_time_cb == NULL) {
    DEBUG_LOGI("key_base", "key_base_register failed: get_time_cb is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  key_base_context_t* new_key =
      (key_base_context_t*)__malloc(sizeof(key_base_context_t));
  if (!new_key) {
    DEBUG_LOGI("key_base",
               "key_base_register failed: memory allocation failed");
    return KEY_BASE_ERROR_NO_MEMORY;
  }

  memset(new_key, 0, sizeof(key_base_context_t));
  new_key->config = *config;

  new_key->next = s_key_master;
  s_key_master = new_key;
  s_key_count++;

  DEBUG_LOGI("key_base", "key_base_register success: %s (total: %u)",
             new_key->config.name, s_key_count);

  if (instance) {
    *instance = new_key;
  }

  return KEY_BASE_OK;
}

/**
 * @brief 注册按键实例（静态内存版本）
 * @param config 按键配置指针
 * @param instance 静态内存指针
 * @return 错误码
 */
key_base_error_t key_base_register_static(const key_base_config_t* config,
                                          key_base_context_t* instance) {
  if (!config) {
    DEBUG_LOGI("key_base", "key_base_register_static failed: config is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!config->name) {
    DEBUG_LOGI("key_base",
               "key_base_register_static failed: config->name is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!instance) {
    DEBUG_LOGI("key_base", "key_base_register_static failed: instance is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!s_system_initialized) {
    key_base_init();
  }

  key_base_context_t* existing = key_base_get_instance(config->name);
  if (existing) {
    DEBUG_LOGI("key_base",
               "key_base_register_static warning: key %s already exists",
               config->name);
    return KEY_BASE_ERROR_ALREADY_EXIST;
  }

  if (config->read_pin_cb == NULL) {
    DEBUG_LOGI("key_base",
               "key_base_register_static failed: read_pin_cb is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (config->event_callback == NULL) {
    DEBUG_LOGI("key_base",
               "key_base_register_static failed: event_callback is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (config->get_time_cb == NULL) {
    DEBUG_LOGI("key_base",
               "key_base_register_static failed: get_time_cb is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  memset(instance, 0, sizeof(key_base_context_t));
  instance->config = *config;
  instance->is_static = 1;

  instance->next = s_key_master;
  s_key_master = instance;
  s_key_count++;

  DEBUG_LOGI("key_base", "key_base_register_static success: %s (total: %u)",
             instance->config.name, s_key_count);

  return KEY_BASE_OK;
}

/**
 * @brief 删除按键实例
 * @param name 按键名称
 * @return 错误码
 */
key_base_error_t key_base_unregister(const char* name) {
  if (!name) {
    DEBUG_LOGI("key_base", "key_base_unregister failed: name is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  if (!s_system_initialized) {
    DEBUG_LOGI("key_base",
               "key_base_unregister failed: system not initialized");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  key_base_context_t** ptr = &s_key_master;
  while (*ptr) {
    if (strcmp((*ptr)->config.name, name) == 0) {
      key_base_context_t* to_unregister = *ptr;
      key_base_context_t* partner = to_unregister->combination_partner;
      *ptr = (*ptr)->next;

      if (partner) {
        partner->combination_partner = NULL;
        partner->data.combination_active = 0;
        partner->data.combination_handled = 0;
        partner->data.combination_long_handled = 0;
      }

      to_unregister->combination_partner = NULL;
      to_unregister->data.combination_active = 0;
      to_unregister->data.combination_handled = 0;
      to_unregister->data.combination_long_handled = 0;

      if (to_unregister->is_static) {
        DEBUG_LOGI("key_base", "key_base_unregister: static key %s skip free",
                   name);
      } else {
        __free(to_unregister);
      }

      s_key_count--;
      DEBUG_LOGI("key_base", "key_base_unregister success: %s (remaining: %u)",
                 name, s_key_count);
      return KEY_BASE_OK;
    }
    ptr = &(*ptr)->next;
  }

  DEBUG_LOGI("key_base", "key_base_unregister failed: key %s not found", name);
  return KEY_BASE_ERROR_NOT_FOUND;
}

/**
 * @brief 注册组合按键
 * @param control_key_name 控制按键名称
 * @param command_key_name 命令按键名称
 * @return 错误码
 */
key_base_error_t key_base_combination_register(const char* control_key_name,
                                               const char* command_key_name) {
  if (command_key_name == NULL || control_key_name == NULL) {
    DEBUG_LOGI("key_base",
               "key_base_combination_register failed: param is NULL");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  key_base_context_t* command_key = key_base_get_instance(command_key_name);
  key_base_context_t* ctrl_key = key_base_get_instance(control_key_name);

  if (command_key == NULL || ctrl_key == NULL) {
    DEBUG_LOGI("key_base",
               "key_base_combination_register failed: key not found");
    return KEY_BASE_ERROR_NOT_FOUND;
  }

  if (command_key == ctrl_key) {
    DEBUG_LOGI("key_base",
               "key_base_combination_register failed: cannot combine key "
               "with itself");
    return KEY_BASE_ERROR_INVALID_PARAM;
  }

  command_key->combination_partner = ctrl_key;
  ctrl_key->combination_partner = command_key;

  DEBUG_LOGI("key_base", "key_base_combination_register success: %s + %s",
             control_key_name, command_key_name);
  return KEY_BASE_OK;
}

/**
 * @brief 按键任务处理函数
 */
void key_base_task(void) {
  if (!s_system_initialized || !s_key_master) {
    return;
  }

  uint32_t current_time = 0;
  key_base_context_t* ctx = s_key_master;
  while (ctx) {
    if (ctx->config.get_time_cb != NULL) {
      current_time = ctx->config.get_time_cb();
      break;
    }
    ctx = ctx->next;
  }
  if (!ctx) {
    return;
  }

  ctx = s_key_master;
  while (ctx) {
    key_base_context_t* next = ctx->next;

    if (ctx->config.read_pin_cb != NULL && ctx->config.get_time_cb != NULL) {
      key_base_debounce_process(ctx, current_time);
    }

    key_base_combination_process(ctx);

    if (ctx->config.event_callback != NULL && ctx->config.get_time_cb != NULL) {
      key_base_state_machine_process(ctx, current_time);
    }

    ctx = next;
  }
}

/**
 * @brief 获取按键实例
 * @param name 按键名称
 * @return 按键实例指针，未找到返回NULL
 */
key_base_context_t* key_base_get_instance(const char* name) {
  if (!name || !s_system_initialized) {
    return NULL;
  }
  key_base_context_t* ctx = s_key_master;
  while (ctx) {
    if (strcmp(ctx->config.name, name) == 0) {
      return ctx;
    }
    ctx = ctx->next;
  }
  return NULL;
}
