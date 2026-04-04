/**
 * @file led.c
 * @author fubingyan qq:3245784484
 * @brief LED控制模块实现 (大厂重构版)
 * @version 2.0.0
 * @date 2025-03-25
 *
 * @copyright Copyright (c) 2025 fubingyan, All Rights Reserved.
 */

#include "led.h"

#include <string.h>

#include "algorithm/maths.h"
#include "bsp_delay.h"
#include "debug/debug.h"
#include "kfifo/kfifo.h"
#include "memory_pool/memory_pool.h"
#include "message_center/message_center.h"

/* ==================== 私有宏与常量 ==================== */
#define LED_PRINTF(...) DEBUG_INFO(__VA_ARGS__)
#define LED_CMD_FIFO_SIZE 8 /* 异步命令队列深度，必须为2的幂 */

/* ==================== 静态变量 = :=================== */
static led_handle_t* led_master = NULL;
static led_get_time_func led_get_time = NULL;
static uint8_t led_system_initialized = 0;
static uint16_t led_count = 0;
static hal_gpio_context_t gpio_ctx;
static hal_tim_pwm_context_t tim_pwm_ctx;

/* ==================== 内部辅助函数 ==================== */

/** @brief 安全时间差计算 */
static inline uint32_t led_time_diff(uint32_t new_time, uint32_t old_time) {
  return (new_time >= old_time) ? (new_time - old_time)
                                : (0xFFFFFFFF - old_time + new_time + 1);
}

/** @brief 获取系统时间 */
static inline uint32_t get_times(void) {
  if (led_get_time) return led_get_time();
  return 0;
}

/** @brief 物理写电平 */
static void led_phys_write(led_handle_t* handle, bool on) {
  hal_gpio_pin_state_t state =
      on ? handle->config.active_level
         : (hal_gpio_pin_state_t)!handle->config.active_level;
  hal_gpio_pin_state_t old_state;
  hal_gpio_read(&gpio_ctx, handle->config.port, handle->config.pin, &old_state);

  hal_gpio_write(&gpio_ctx, handle->config.port, handle->config.pin, state);

  /* 检测边沿变化并触发回调 */
  if (handle->gpio_edge_cb && old_state != state) {
    /* 检测上升沿或下降沿 */
    if ((old_state == HAL_GPIO_PIN_RESET && state == HAL_GPIO_PIN_SET) ||
        (old_state == HAL_GPIO_PIN_SET && state == HAL_GPIO_PIN_RESET)) {
      ((led_gpio_edge_callback_t)handle->gpio_edge_cb)(
          handle, state, handle->callback_user_data);
    }
  }
}

/** @brief 物理读逻辑电平 (返回 true 表示逻辑开启) */
static bool led_phys_read(led_handle_t* handle) {
  hal_gpio_pin_state_t state;
  hal_gpio_read(&gpio_ctx, handle->config.port, handle->config.pin, &state);
  return (state == handle->config.active_level);
}

/* ==================== FSM 状态处理器与回调 ==================== */

static fsm_state_t led_fsm_none_handler(fsm_context_t* ctx) {
  return LED_STATE_NONE;
}

static fsm_state_t led_fsm_off_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  led_phys_write(handle, false);
  return LED_STATE_OFF;
}

static fsm_state_t led_fsm_on_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  led_phys_write(handle, true);
  return LED_STATE_ON;
}

static fsm_state_t led_fsm_blink_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = get_times();

  if (handle->blink_code_phase == BLINK_CODE_PHASE_BLINKING) {
    if (led_time_diff(now, handle->last_toggle_time) >=
        handle->config.blink_interval_ms) {
      handle->last_toggle_time = now;
      bool is_on = led_phys_read(handle);
      led_phys_write(handle, !is_on);
      if (!led_phys_read(handle)) {
        handle->current_blink_counts++;
        /* 当 counts=0 时，每次闪烁后进入间隔；或者达到指定次数后进入间隔 */
        if (handle->config.blink_counts == 0 ||
            handle->current_blink_counts >= handle->config.blink_counts) {
          handle->last_blink_code_phase = handle->blink_code_phase;
          handle->blink_code_phase = BLINK_CODE_PHASE_INTERVAL;
          handle->interval_start_time = now;
          handle->current_blink_counts = 0;
          /* 如果 blink_counts > 0，表示单次闪烁模式，完成后自动关闭 */
          if (handle->config.blink_counts > 0) {
            return LED_STATE_OFF; /* 单次闪烁完成，自动关闭 */
          }
        }
      }
    }
  } else {
    /* INTERVAL 阶段：检查是否完成单次闪烁模式 */
    if (led_time_diff(now, handle->interval_start_time) >=
        handle->config.blink_interval_wait_ms) {
      /* 否则是连续闪烁模式，继续闪烁 */
      handle->blink_code_phase = BLINK_CODE_PHASE_BLINKING;
      handle->last_toggle_time = now;
    }
  }
  return LED_STATE_BLINK_CODE;
}

static fsm_state_t led_fsm_breathing_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = get_times();

  if (led_time_diff(now, handle->last_breath_time) >=
      handle->config.breath_interval_ms) {
    handle->last_breath_time = now;

    if (handle->entry_breath_wait_counts <
        (handle->config.breath_max + handle->config.breath_step) / 2) {
      handle->entry_breath_wait_counts += handle->config.breath_step;
    } else {
      static uint32_t breath_cycle = 0;
      breath_cycle += 3;

      if (breath_cycle >= 1000) {
        breath_cycle = 0;
      }
      // 使用正弦函数实现非线性亮度变化，使呼吸效果更加自然
      // 计算呼吸周期的相位（0-2π）
      float phase = (float)breath_cycle * 0.001f * 2.0f * M_PIf;

      // 计算亮度值，使用正弦函数的平方使变化更加平滑
      float brightness_ratio = (sin_approx(phase) + 1.0f) * 0.5f;
      // brightness_ratio *= brightness_ratio; // 平方使变化更加平滑
      // 添加gamma校正，使亮度变化更加自然
      int gamma = 2;
      float gamma_corrected = powerf(brightness_ratio, gamma);
      // 计算最终的PWM值
      handle->breath_value = (uint16_t)(handle->config.breath_min +
                                        (uint16_t)((handle->config.breath_max -
                                                    handle->config.breath_min) *
                                                   gamma_corrected));
    }

    if (handle->pwm_init_flag) {
      hal_tim_pwm_set_duty_cycle(
          &tim_pwm_ctx, handle->config.pwm_cfg.timer_instance,
          handle->config.pwm_cfg.channel, handle->breath_value);
    }
  }
  return LED_STATE_BREATHING;
}

static void led_fsm_on_entry(fsm_context_t* ctx, fsm_state_t state) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = get_times();

  /* 触发状态变化回调 */
  if (handle->state_change_cb) {
    ((led_state_change_callback_t)handle->state_change_cb)(
        handle, state, handle->callback_user_data);
  }

  switch (state) {
    case LED_STATE_ON:
      led_phys_write(handle, true);
      break;
    case LED_STATE_OFF:
      led_phys_write(handle, false);
      break;
    case LED_STATE_BLINK_CODE:
      handle->current_blink_counts = 0;
      handle->last_blink_code_phase = handle->blink_code_phase;
      handle->blink_code_phase = BLINK_CODE_PHASE_INTERVAL;
      handle->interval_start_time = now;
      led_phys_write(handle, false);

      break;
    case LED_STATE_BREATHING:
      handle->breath_direction = 1;
      handle->breath_value = handle->config.breath_min;
      handle->last_breath_time = now;
      handle->entry_breath_wait_counts = 0;
      if (handle->config.gpio_pwm_init_cb) {
        handle->config.gpio_pwm_init_cb();
      } else {
        if (handle->config.gpio_pwm_init_cb) {
          handle->config.gpio_pwm_init_cb();
        } else {
          if (handle->config.pwm_cfg.timer_instance != 0) {
            /* 如果没有配置 PWM 初始化回调，默认配置为输出模式 */
            hal_tim_pwm_init(&tim_pwm_ctx, &handle->config.pwm_cfg);
            hal_tim_pwm_start(&tim_pwm_ctx,
                              handle->config.pwm_cfg.timer_instance,
                              handle->config.pwm_cfg.channel);
            handle->pwm_init_flag = 1;
          }
        }
        hal_tim_pwm_gpio_alternate(
            &tim_pwm_ctx,
            &handle->config.pwm_cfg.gpio);  // 配置GPIO复用功能
      }
      break;
    default:
      break;
  }
}

static void led_fsm_on_exit(fsm_context_t* ctx, fsm_state_t state) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  switch (state) {
    case LED_STATE_BREATHING:
      if (handle->config.gpio_init_cb) {
        handle->config.gpio_init_cb();
      } else {
        /* 如果没有配置 GPIO 初始化回调，默认配置为输出模式 */
        hal_gpio_config_t gpio_cfg = {
            .port = handle->config.port,
            .pin = handle->config.pin,
            .mode = HAL_GPIO_MODE_OUTPUT_PP,
            .pull = HAL_GPIO_PULL_NONE,
            .speed = HAL_GPIO_SPEED_FREQ_LOW,
            .default_state = !handle->config.active_level};
        hal_gpio_init(&gpio_ctx, &gpio_cfg);
      }
      break;
  }
}

/* ==================== API 实现 ==================== */

led_error_t LedInit(led_get_time_func get_time_cb) {
  if (!get_time_cb) return LED_ERR_INVALID_PARAM;
  if (led_system_initialized) return LED_OK_EXISTED;

  led_get_time = get_time_cb;
  led_master = NULL;
  led_count = 0;
  led_system_initialized = 1;

  stm32_tim_pwm_init_context(&tim_pwm_ctx);

  MessageCenterInit(); /* 确保消息中心已就绪 */

  LED_PRINTF("LED system initialized\n");
  return LED_OK;
}

void LedDeinit(void) {
  if (!led_system_initialized) return;

  led_handle_t* curr = led_master;
  while (curr) {
    led_handle_t* next = curr->next;
    LedUnregister(curr->config.led_name);
    curr = next;
  }

  led_system_initialized = 0;
  led_master = NULL;
  led_count = 0;
}

led_handle_t* LedGetInstance(const char* name) {
  if (!name || !led_system_initialized) return NULL;
  led_handle_t* curr = led_master;
  while (curr) {
    if (__strcmp(curr->config.led_name, name) == 0) return curr;
    curr = curr->next;
  }
  return NULL;
}

led_error_t LedRegisterStatic(const led_config_t* config,
                              led_handle_t* instance) {
  if (!config || !instance || !config->led_name) return LED_ERR_INVALID_PARAM;
  if (!led_system_initialized) return LED_ERR_INTERNAL;

  if (LedGetInstance(config->led_name)) return LED_ERR_ALREADY_EXIST;

  __memset(instance, 0, sizeof(led_handle_t));
  __memcpy(&instance->config, config, sizeof(led_config_t));
  instance->is_static = 1;
  instance->initialized = 1;

  /* 初始化 FSM (大厂规范) */
  static const char* led_state_names[] = {"NONE", "OFF", "ON", "BLINK",
                                          "BREATHING"};
  fsm_init(&instance->fsm, config->init_state, instance);
  fsm_register_handler(&instance->fsm, LED_STATE_NONE, led_fsm_none_handler);
  fsm_register_handler(&instance->fsm, LED_STATE_OFF, led_fsm_off_handler);
  fsm_register_handler(&instance->fsm, LED_STATE_ON, led_fsm_on_handler);
  fsm_register_handler(&instance->fsm, LED_STATE_BLINK_CODE,
                       led_fsm_blink_handler);
  fsm_register_handler(&instance->fsm, LED_STATE_BREATHING,
                       led_fsm_breathing_handler);
  fsm_set_callbacks(&instance->fsm, led_fsm_on_entry, led_fsm_on_exit);
  fsm_set_state_names(&instance->fsm, led_state_names,
                      sizeof(led_state_names) / sizeof(led_state_names[0]));

  /* 允许任意状态切换 (LED 模块由外部指令驱动) */
  for (uint8_t i = 0; i < LED_STATE_MAX; i++) {
    for (uint8_t j = 0; j < LED_STATE_MAX; j++) {
      if (i != j) fsm_add_transition(&instance->fsm, i, j, NULL);
    }
  }

  /* 初始化命令队列 */
  instance->cmd_fifo = kfifo_alloc(LED_CMD_FIFO_SIZE * sizeof(led_cmd_t), NULL);
  if (!instance->cmd_fifo) return LED_ERR_NO_MEMORY;

  /* 如果是呼吸灯，且配置了PWM信息，则初始化PWM */

  /* 硬件底层初始化 */
  if (config->gpio_init_cb) {
    config->gpio_init_cb();
  } else {
    /* 如果没有配置 GPIO 初始化回调，默认配置为输出模式 */
    stm32_gpio_init_context(&gpio_ctx);
    hal_gpio_config_t gpio_cfg = {.port = config->port,
                                  .pin = config->pin,
                                  .mode = HAL_GPIO_MODE_OUTPUT_PP,
                                  .pull = HAL_GPIO_PULL_NONE,
                                  .speed = HAL_GPIO_SPEED_FREQ_LOW,
                                  .default_state = !config->active_level};
    hal_gpio_init(&gpio_ctx, &gpio_cfg);
  }

  /* 加入链表 (头插法) */
  instance->next = led_master;
  led_master = instance;
  led_count++;

  LED_PRINTF("LED static registered: %s\n", config->led_name);
  return LED_OK;
}

led_handle_t* LedRegister(const led_config_t* config) {
  if (!config || !config->led_name) return NULL;

  led_handle_t* instance = (led_handle_t*)__malloc(sizeof(led_handle_t));
  if (!instance) return NULL;

  led_error_t err = LedRegisterStatic(config, instance);
  if (LED_IS_ERR(err)) {
    __free(instance);
    return NULL;
  }
  instance->is_static = 0; /* 修正标记 */
  return instance;
}

led_error_t LedUnregister(const char* name) {
  if (!name || !led_system_initialized) return LED_ERR_INVALID_PARAM;

  led_handle_t** ptr = &led_master;
  while (*ptr) {
    if (__strcmp((*ptr)->config.led_name, name) == 0) {
      led_handle_t* to_remove = *ptr;
      *ptr = to_remove->next;

      /* 释放命令队列 */
      if (to_remove->cmd_fifo) {
        kfifo_free((kfifo_t*)to_remove->cmd_fifo);
      }

      if (!to_remove->is_static) {
        __free(to_remove);
      }
      led_count--;
      return LED_OK;
    }
    ptr = &(*ptr)->next;
  }
  return LED_ERR_NOT_FOUND;
}

void LedSetState(led_handle_t* instance, led_state_t state) {
  if (!instance) return;
  led_cmd_t cmd = {.target_state = state};
  kfifo_put((kfifo_t*)instance->cmd_fifo, (unsigned char*)&cmd,
            sizeof(led_cmd_t));
}

led_error_t LedSetBlinkInterval(led_handle_t* instance, uint16_t interval_ms,
                                uint16_t interval_wait_ms, uint16_t counts) {
  if (!instance) return LED_ERR_INVALID_PARAM;

  /* 检查参数是否真的变化 */
  bool param_changed =
      (instance->config.blink_interval_ms != interval_ms) ||
      (instance->config.blink_interval_wait_ms != interval_wait_ms) ||
      (instance->config.blink_counts != counts);

  if (!param_changed) {
    return LED_OK; /* 参数未变化，直接返回 */
  }

  /* 如果 LED 正在编码闪烁模式 */
  if (fsm_get_current_state(&instance->fsm) == LED_STATE_BLINK_CODE) {
    /* 读取当前 LED 状态（true 表示亮，false 表示灭） */
    bool is_led_on = led_phys_read(instance);

    if (is_led_on) {
      /* LED 亮着，标记需要等待熄灭后更新，将参数暂存到 cmd_fifo */
      instance->pending_blink_update = 1;
      led_cmd_t cmd = {.target_state = LED_STATE_BLINK_CODE,
                       .blink_interval_ms = interval_ms,
                       .blink_interval_wait_ms = interval_wait_ms,
                       .blink_counts = counts};
      return kfifo_put((kfifo_t*)instance->cmd_fifo, (unsigned char*)&cmd,
                       sizeof(led_cmd_t)) == sizeof(led_cmd_t)
                 ? LED_OK
                 : LED_ERR_INTERNAL;
    } else {
      /* LED 已熄灭，立即切换参数 */
      instance->config.blink_interval_ms = interval_ms;
      instance->config.blink_interval_wait_ms = interval_wait_ms;
      instance->config.blink_counts = counts;
      instance->pending_blink_update = 0;

      /* 重置闪烁进度，强制进入间隔阶段，等待间隔结束后再开始闪烁 */
      instance->current_blink_counts = 0;
      instance->blink_code_phase = BLINK_CODE_PHASE_INTERVAL;
      instance->interval_start_time = get_times();
      led_phys_write(instance, false); /* 确保保持熄灭状态 */

      return LED_OK;
    }
  } else {
    /* 不在闪烁模式，直接更新参数 */
    instance->config.blink_interval_ms = interval_ms;
    instance->config.blink_interval_wait_ms = interval_wait_ms;
    instance->config.blink_counts = counts;
    instance->pending_blink_update = 0;
    return LED_OK;
  }
}

blink_code_phase_t LedGetBlinkPhase(led_handle_t* instance) {
  if (!instance) return BLINK_CODE_PHASE_INTERVAL;
  return (blink_code_phase_t)instance->blink_code_phase;
}

void LedSetStateChangeCallback(led_handle_t* instance,
                               led_state_change_callback_t state_cb,
                               led_blink_phase_callback_t blink_phase_cb,
                               led_gpio_edge_callback_t gpio_edge_cb,
                               void* user_data) {
  if (!instance) return;

  instance->state_change_cb = (void*)state_cb;
  instance->blink_phase_cb = (void*)blink_phase_cb;
  instance->gpio_edge_cb = (void*)gpio_edge_cb;
  instance->callback_user_data = user_data;
}

/** @brief 处理单个 LED 的命令队列 */
static void process_led_cmds(led_handle_t* handle) {
  led_cmd_t cmd;
  while (kfifo_get(handle->cmd_fifo, (unsigned char*)&cmd, sizeof(led_cmd_t)) ==
         sizeof(led_cmd_t)) {
    if (cmd.target_state != LED_STATE_NONE) {
      fsm_request_transition(&handle->fsm, cmd.target_state);
    }

    if (cmd.target_state == LED_STATE_BLINK_CODE) {
      /* 如果有待处理的更新且 LED 正在闪烁，检查是否需要等待熄灭 */
      if (handle->pending_blink_update &&
          fsm_get_current_state(&handle->fsm) == LED_STATE_BLINK_CODE) {
        /* 读取当前 LED 状态（true 表示亮，false 表示灭） */
        bool is_led_on = led_phys_read(handle);

        if (is_led_on) {
          /* LED 还亮着，将命令重新放回队列，等待下次处理 */
          kfifo_put(handle->cmd_fifo, (unsigned char*)&cmd, sizeof(led_cmd_t));
          break; /* 跳出循环，等待 LED 熄灭 */
        }

        /* LED 已熄灭，清除等待标志 */
        handle->pending_blink_update = 0;
      }

      /* 应用新参数 */
      if (cmd.blink_interval_ms > 0)
        handle->config.blink_interval_ms = cmd.blink_interval_ms;
      if (cmd.blink_interval_wait_ms > 0)
        handle->config.blink_interval_wait_ms = cmd.blink_interval_wait_ms;
      if (cmd.blink_counts > 0) handle->config.blink_counts = cmd.blink_counts;

      /* 如果已经在闪烁模式，则强制进入间隔阶段，等待间隔结束后再开始闪烁 */
      if (fsm_get_current_state(&handle->fsm) == LED_STATE_BLINK_CODE) {
        handle->current_blink_counts = 0;
        handle->blink_code_phase = BLINK_CODE_PHASE_INTERVAL;
        handle->interval_start_time = get_times();
        led_phys_write(handle, false); /* 确保保持熄灭状态 */
      }
    }

    /* 使用 Message Center 发布状态变更事件 (可选) */
    /* ... */
  }
}

/** @brief 检测并触发闪烁阶段变化回调（轮询检测变化） */
static void check_blink_phase_change(led_handle_t* handle) {
  if (!handle || !handle->blink_phase_cb) return;

  if (handle->blink_code_phase != handle->last_blink_code_phase) {
    /* 检测到阶段变化，触发回调 */
    ((led_blink_phase_callback_t)handle->blink_phase_cb)(
        handle, handle->blink_code_phase, handle->callback_user_data);
    handle->last_blink_code_phase = handle->blink_code_phase;
  }
}

void LedTaskRefresh(void) {
  if (!led_system_initialized || !led_master) return;

  led_handle_t* curr = led_master;

  while (curr) {
    if (!curr->initialized) {
      curr = curr->next;
      continue;
    }

    /* 处理异步命令 */
    process_led_cmds(curr);

    /* 执行 FSM 步进 (驱动状态机运行) */
    fsm_step(&curr->fsm);

    /* 检测闪烁阶段变化并触发回调 */
    check_blink_phase_change(curr);

    curr = curr->next;
  }
}
