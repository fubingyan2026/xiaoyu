//
// Created by fubingyan on 25-9-20.
//
/**
 * @file    led.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2025-09-20
 * @brief   LED 控制模块实现
 * @attention
 *
 * Copyright (c) 2025 fubingyan.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "led.h"

#include <string.h>

#include "algorithm/maths.h"
#include "debug/debug.h"
#include "kfifo/kfifo.h"
#include "memory_pool/memory_pool.h"
#include "message_center/message_center.h"

/* Private macros ------------------------------------------------------------*/
#define LED_PRINTF(...) DEBUG_INFO(__VA_ARGS__)
#define LED_CMD_FIFO_SIZE 8            /* 异步命令队列深度，必须为2的幂 */
#define LED_BREATH_MAX_RESOLUTION 1000 /* 呼吸灯最大分辨率 */
#define LED_GAMMA_CORRECTION 2         /* Gamma 校正值 */

/* Private variables ---------------------------------------------------------*/
static led_handle_t* led_master = NULL;
static led_get_time_func_t led_get_time = NULL;
static uint8_t led_system_initialized = 0;
static uint16_t led_count = 0;
static hal_gpio_context_t gpio_ctx;
static hal_tim_pwm_context_t tim_pwm_ctx;

/* Private function prototypes -----------------------------------------------*/
static inline uint32_t led_time_diff(uint32_t new_time, uint32_t old_time);
static inline uint32_t led_get_time_now(void);
static void led_phys_write(led_handle_t* handle, bool on);
static bool led_phys_read(led_handle_t* handle);
static fsm_state_t led_fsm_none_handler(fsm_context_t* ctx);
static fsm_state_t led_fsm_off_handler(fsm_context_t* ctx);
static fsm_state_t led_fsm_on_handler(fsm_context_t* ctx);
static fsm_state_t led_fsm_blink_handler(fsm_context_t* ctx);
static fsm_state_t led_fsm_breathing_handler(fsm_context_t* ctx);
static void led_fsm_on_entry(fsm_context_t* ctx, fsm_state_t state);
static void led_fsm_on_exit(fsm_context_t* ctx, fsm_state_t state);
static void led_process_cmds(led_handle_t* handle);
static void led_check_blink_phase_change(led_handle_t* handle);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  安全时间差计算
 */
static inline uint32_t led_time_diff(uint32_t new_time, uint32_t old_time) {
  return (new_time >= old_time) ? (new_time - old_time)
                                : (0xFFFFFFFF - old_time + new_time + 1);
}

/**
 * @brief  获取系统时间
 */
static inline uint32_t led_get_time_now(void) {
  if (led_get_time) return led_get_time();
  return 0;
}

/**
 * @brief  物理写电平
 */
static void led_phys_write(led_handle_t* handle, bool on) {
  hal_gpio_pin_state_t state =
      on ? handle->config.active_level
         : (hal_gpio_pin_state_t)!handle->config.active_level;
  hal_gpio_pin_state_t old_state;
  hal_gpio_read(&gpio_ctx, handle->config.port, handle->config.pin, &old_state);

  hal_gpio_write(&gpio_ctx, handle->config.port, handle->config.pin, state);

  // 检测边沿变化并触发回调
  if (handle->gpio_edge_cb && old_state != state) {
    // 检测上升沿或下降沿
    if ((old_state == HAL_GPIO_PIN_RESET && state == HAL_GPIO_PIN_SET) ||
        (old_state == HAL_GPIO_PIN_SET && state == HAL_GPIO_PIN_RESET)) {
      ((led_gpio_edge_callback_t)handle->gpio_edge_cb)(
          handle, state, handle->callback_user_data);
    }
  }
}

/**
 * @brief  物理读逻辑电平 (返回 true 表示逻辑开启)
 */
static bool led_phys_read(led_handle_t* handle) {
  hal_gpio_pin_state_t state;
  hal_gpio_read(&gpio_ctx, handle->config.port, handle->config.pin, &state);
  return (state == handle->config.active_level);
}

/**
 * @brief  FSM NONE 状态处理器
 */
static fsm_state_t led_fsm_none_handler(fsm_context_t* ctx) {
  (void)ctx;
  return LED_STATE_NONE;
}

/**
 * @brief  FSM OFF 状态处理器
 */
static fsm_state_t led_fsm_off_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  led_phys_write(handle, false);
  return LED_STATE_OFF;
}

/**
 * @brief  FSM ON 状态处理器
 */
static fsm_state_t led_fsm_on_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  led_phys_write(handle, true);
  return LED_STATE_ON;
}

/**
 * @brief  FSM BLINK 状态处理器
 */
static fsm_state_t led_fsm_blink_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = led_get_time_now();

  if (handle->blink_code_phase == LED_BLINK_PHASE_BLINKING) {
    if (led_time_diff(now, handle->last_toggle_time) >=
        handle->current_cmd.led_blink_cycle_ms) {
      handle->last_toggle_time = now;
      bool is_on = led_phys_read(handle);
      led_phys_write(handle, !is_on);
      if (!led_phys_read(handle)) {
        handle->current_led_blink_code_counts++;
        // 当 counts=0 时，每次闪烁后进入间隔；或者达到指定次数后进入间隔
        if (handle->current_cmd.led_blink_code_counts == 0 ||
            handle->current_led_blink_code_counts >=
                handle->current_cmd.led_blink_code_counts) {
          handle->last_blink_code_phase = handle->blink_code_phase;
          handle->blink_code_phase = LED_BLINK_PHASE_INTERVAL;
          handle->interval_start_time = now;
          handle->current_led_blink_code_counts = 0;
          // 如果 led_blink_code_counts > 0，表示单次闪烁模式，完成后自动关闭
          if (handle->current_cmd.led_blink_code_counts > 0) {
            return LED_STATE_OFF;  // 单次闪烁完成，自动关闭
          }
        }
      }
    }
  } else {
    // INTERVAL 阶段：检查是否完成单次闪烁模式
    if (led_time_diff(now, handle->interval_start_time) >=
        handle->current_cmd.led_blink_wait_ms) {
      // 否则是连续闪烁模式，继续闪烁
      handle->blink_code_phase = LED_BLINK_PHASE_BLINKING;
      handle->last_toggle_time = now;
    }
  }
  return LED_STATE_BLINK_CODE;
}

/**
 * @brief  FSM BREATHING 状态处理器
 */
static fsm_state_t led_fsm_breathing_handler(fsm_context_t* ctx) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = led_get_time_now();

  if (led_time_diff(now, handle->last_breath_time) >=
      handle->config.led_refresh_time_ms) {
    handle->last_breath_time = now;
    uint16_t led_breath_steps =
        LED_BREATH_MAX_RESOLUTION / (handle->config.led_refresh_cycle_ms /
                                     handle->config.led_refresh_time_ms);
    handle->breath_cycle += led_breath_steps;

    if (handle->breath_cycle >= LED_BREATH_MAX_RESOLUTION) {
      handle->breath_cycle = 0;
    }
    // 使用正弦函数实现非线性亮度变化，使呼吸效果更加自然
    // 计算呼吸周期的相位（0-2π）
    float phase = (float)handle->breath_cycle * 0.001f * 2.0f * M_PIf;

    // 计算亮度值，使用正弦函数使变化更加平滑
    float brightness_ratio = (sin_approx(phase) + 1.0f) * 0.5f;
    // 添加gamma校正，使亮度变化更加自然
    float gamma_corrected = powerf(brightness_ratio, LED_GAMMA_CORRECTION);
    // 计算最终的PWM值
    handle->breath_value =
        (uint16_t)(handle->config.led_refresh_min_duty +
                   (uint16_t)((handle->config.led_refresh_max_duty -
                               handle->config.led_refresh_min_duty) *
                              gamma_corrected));

    if (handle->pwm_init_flag) {
      hal_tim_pwm_set_duty_cycle(
          &tim_pwm_ctx, handle->config.pwm_cfg.timer_instance,
          handle->config.pwm_cfg.channel, handle->breath_value);
    }
  }
  return LED_STATE_BREATHING;
}

/**
 * @brief  FSM 进入状态回调
 */
static void led_fsm_on_entry(fsm_context_t* ctx, fsm_state_t state) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  uint32_t now = led_get_time_now();

  // 触发状态变化回调
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
      handle->current_led_blink_code_counts = 0;
      handle->last_blink_code_phase = handle->blink_code_phase;
      handle->blink_code_phase = LED_BLINK_PHASE_INTERVAL;
      handle->interval_start_time = now;
      led_phys_write(handle, false);
      break;
    case LED_STATE_BREATHING:
      handle->breath_value = handle->config.led_refresh_min_duty;
      handle->breath_cycle = 0;
      handle->last_breath_time = now;
      hal_tim_pwm_gpio_alternate(&tim_pwm_ctx, &handle->config.pwm_cfg.gpio);
      // 确保PWM已启动
      if (handle->pwm_init_flag) {
        hal_tim_pwm_start(&tim_pwm_ctx, handle->config.pwm_cfg.timer_instance,
                          handle->config.pwm_cfg.channel);
      }
      break;
    default:
      break;
  }
}

/**
 * @brief  FSM 退出状态回调
 */
static void led_fsm_on_exit(fsm_context_t* ctx, fsm_state_t state) {
  led_handle_t* handle = (led_handle_t*)ctx->user_data;
  switch (state) {
    case LED_STATE_BREATHING:
      if (handle->config.gpio_init_cb) {
        handle->config.gpio_init_cb();
      } else {
        // 如果没有配置 GPIO 初始化回调，默认配置为输出模式
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
    default:
      break;
  }
}

/**
 * @brief  处理单个 LED 的命令队列
 */
static void led_process_cmds(led_handle_t* handle) {
  led_cmd_t cmd;
  while (kfifo_get(handle->cmd_fifo, (unsigned char*)&cmd, sizeof(led_cmd_t)) ==
         sizeof(led_cmd_t)) {
    if (cmd.led_set_state != LED_STATE_NONE) {
      fsm_request_transition(&handle->fsm, cmd.led_set_state);
    }

    if (cmd.led_set_state == LED_STATE_BLINK_CODE) {
      // 如果有待处理的更新且 LED 正在闪烁，检查是否需要等待熄灭
      if (handle->pending_blink_update &&
          fsm_get_current_state(&handle->fsm) == LED_STATE_BLINK_CODE) {
        // 读取当前 LED 状态（true 表示亮，false 表示灭）
        bool is_led_on = led_phys_read(handle);

        if (is_led_on) {
          // LED 还亮着，将命令重新放回队列，等待下次处理
          kfifo_put(handle->cmd_fifo, (unsigned char*)&cmd, sizeof(led_cmd_t));
          break;  // 跳出循环，等待 LED 熄灭
        }

        // LED 已熄灭，清除等待标志
        handle->pending_blink_update = 0;
      }

      // 应用新参数
      if (cmd.led_blink_cycle_ms > 0)
        handle->current_cmd.led_blink_cycle_ms = cmd.led_blink_cycle_ms;
      if (cmd.led_blink_wait_ms > 0)
        handle->current_cmd.led_blink_wait_ms = cmd.led_blink_wait_ms;
      if (cmd.led_blink_code_counts > 0)
        handle->current_cmd.led_blink_code_counts = cmd.led_blink_code_counts;

      // 如果已经在闪烁模式，则强制进入间隔阶段，等待间隔结束后再开始闪烁
      if (fsm_get_current_state(&handle->fsm) == LED_STATE_BLINK_CODE) {
        handle->current_led_blink_code_counts = 0;
        handle->blink_code_phase = LED_BLINK_PHASE_INTERVAL;
        handle->interval_start_time = led_get_time_now();
        led_phys_write(handle, false);  // 确保保持熄灭状态
      }
    }
  }
}

/**
 * @brief  检测并触发闪烁阶段变化回调（轮询检测变化）
 */
static void led_check_blink_phase_change(led_handle_t* handle) {
  if (!handle || !handle->blink_phase_cb) return;

  if (handle->blink_code_phase != handle->last_blink_code_phase) {
    // 检测到阶段变化，触发回调
    ((led_blink_phase_callback_t)handle->blink_phase_cb)(
        handle, handle->blink_code_phase, handle->callback_user_data);
    handle->last_blink_code_phase = handle->blink_code_phase;
  }
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化 LED 子系统
 * @param  get_time_cb 毫秒级时间获取回调函数
 * @return 操作结果错误码
 */
led_error_t led_init(led_get_time_func_t get_time_cb) {
  // 检查参数有效性
  if (get_time_cb == NULL) {
    return LED_ERROR_INVALID_PARAM;
  }

  // 检查是否已初始化
  if (led_system_initialized) {
    return LED_OK_EXISTED;
  }

  led_get_time = get_time_cb;
  led_master = NULL;
  led_count = 0;
  led_system_initialized = 1;

  stm32_gpio_init_context(&gpio_ctx);
  stm32_tim_pwm_init_context(&tim_pwm_ctx);

  MessageCenterInit();  // 确保消息中心已就绪

  LED_PRINTF("LED system initialized\n");
  return LED_OK;
}

/**
 * @brief  反初始化 LED 子系统
 */
void led_deinit(void) {
  if (!led_system_initialized) return;

  led_handle_t* curr = led_master;
  while (curr) {
    led_handle_t* next = curr->next;
    led_unregister(curr->config.led_name);
    curr = next;
  }

  led_system_initialized = 0;
  led_master = NULL;
  led_count = 0;
}

/**
 * @brief  获取 LED 实例
 * @param  name 名称
 * @return LED 实例指针，未找到返回 NULL
 */
led_handle_t* led_get_instance(const char* name) {
  // 检查参数有效性
  if (name == NULL || !led_system_initialized) {
    return NULL;
  }

  led_handle_t* curr = led_master;
  while (curr) {
    if (__strcmp(curr->config.led_name, name) == 0) return curr;
    curr = curr->next;
  }
  return NULL;
}

/**
 * @brief  静态注册 LED 实例
 * @param  config 配置指针
 * @param  instance 用户提供的句柄空间
 * @return 操作结果错误码
 */
led_error_t led_register_static(const led_config_t* config,
                                led_handle_t* instance) {
  // 检查参数有效性
  if (config == NULL || instance == NULL || config->led_name == NULL) {
    return LED_ERROR_INVALID_PARAM;
  }

  if (!led_system_initialized) {
    return LED_ERROR_INTERNAL;
  }

  if (led_get_instance(config->led_name)) {
    return LED_ERROR_ALREADY_EXIST;
  }

  __memset(instance, 0, sizeof(led_handle_t));
  __memcpy(&instance->config, config, sizeof(led_config_t));
  instance->is_static = 1;
  instance->initialized = 1;

  // 初始化 FSM (大厂规范)
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

  // 允许任意状态切换 (LED 模块由外部指令驱动)
  for (uint8_t i = 0; i < LED_STATE_MAX; i++) {
    for (uint8_t j = 0; j < LED_STATE_MAX; j++) {
      if (i != j) {
        fsm_add_transition(&instance->fsm, i, j, NULL);
      }
    }
  }

  // 初始化命令队列
  instance->cmd_fifo = kfifo_alloc(LED_CMD_FIFO_SIZE * sizeof(led_cmd_t), NULL);
  if (!instance->cmd_fifo) return LED_ERROR_NO_MEMORY;

  // 如果是呼吸灯，且配置了PWM信息，则初始化PWM
  if (!instance->pwm_init_flag) {
    if (instance->config.gpio_pwm_init_cb) {
      instance->config.gpio_pwm_init_cb();
    } else if (instance->config.pwm_cfg.timer_instance != 0) {
      hal_tim_pwm_init(&tim_pwm_ctx, &instance->config.pwm_cfg);
      hal_tim_pwm_start(&tim_pwm_ctx, instance->config.pwm_cfg.timer_instance,
                        instance->config.pwm_cfg.channel);
      instance->pwm_init_flag = 1;
    }
  }

  // 硬件底层初始化
  // 如果初始状态是呼吸模式，GPIO需要保持PWM复用模式，不配置为普通输出
  if (config->init_state == LED_STATE_BREATHING) {
    // 呼吸模式：GPIO已在PWM初始化时配置为复用模式，无需再配置
    // 确保PWM已启动
    if (instance->pwm_init_flag) {
      hal_tim_pwm_start(&tim_pwm_ctx, instance->config.pwm_cfg.timer_instance,
                        instance->config.pwm_cfg.channel);
    }
  } else {
    if (instance->config.gpio_init_cb) {
      instance->config.gpio_init_cb();
    } else {
      // 如果没有配置 GPIO 初始化回调，默认配置为输出模式
      hal_gpio_config_t gpio_cfg = {.port = config->port,
                                    .pin = config->pin,
                                    .mode = HAL_GPIO_MODE_OUTPUT_PP,
                                    .pull = HAL_GPIO_PULL_NONE,
                                    .speed = HAL_GPIO_SPEED_FREQ_LOW,
                                    .default_state = !config->active_level};
      hal_gpio_init(&gpio_ctx, &gpio_cfg);
    }
  }

  // 加入链表 (头插法)
  instance->next = led_master;
  led_master = instance;
  led_count++;

  LED_PRINTF("LED static registered: %s\n", config->led_name);
  return LED_OK;
}

/**
 * @brief  动态注册 LED 实例
 * @param  config 配置指针
 * @return 实例指针，失败返回 NULL
 */
led_handle_t* led_register(const led_config_t* config) {
  // 检查参数有效性
  if (config == NULL || config->led_name == NULL) {
    return NULL;
  }

  led_handle_t* instance = (led_handle_t*)__malloc(sizeof(led_handle_t));
  if (!instance) return NULL;

  led_error_t err = led_register_static(config, instance);
  if (LED_IS_ERR(err)) {
    __free(instance);
    return NULL;
  }
  instance->is_static = 0;  // 修正标记
  return instance;
}

/**
 * @brief  注销 LED 实例
 * @param  name LED 名称
 * @return 操作结果错误码
 */
led_error_t led_unregister(const char* name) {
  // 检查参数有效性
  if (name == NULL || !led_system_initialized) {
    return LED_ERROR_INVALID_PARAM;
  }

  led_handle_t** ptr = &led_master;
  while (*ptr) {
    if (__strcmp((*ptr)->config.led_name, name) == 0) {
      led_handle_t* to_remove = *ptr;
      *ptr = to_remove->next;

      // 释放命令队列
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
  return LED_ERROR_NOT_FOUND;
}

/**
 * @brief  设置 LED 状态 (异步)
 * @param  instance 句柄
 * @param  state 目标状态
 */
void led_set_state(led_handle_t* instance, led_state_t state) {
  // 检查参数有效性
  if (instance == NULL) return;

  led_cmd_t cmd = {.led_set_state = state};
  kfifo_put((kfifo_t*)instance->cmd_fifo, (unsigned char*)&cmd,
            sizeof(led_cmd_t));
}

/**
 * @brief  设置 LED 闪烁参数 (通过事件/命令触发)
 * @param  instance 句柄
 * @param  cmd 闪烁参数结构体指针
 * @return 操作结果错误码
 */
led_error_t led_set_blink_interval(led_handle_t* instance,
                                   const led_cmd_t* cmd) {
  // 检查参数有效性
  if (instance == NULL || cmd == NULL) {
    return LED_ERROR_INVALID_PARAM;
  }

  // 检查参数是否真的变化
  bool param_changed =
      (instance->current_cmd.led_set_state != LED_STATE_BLINK_CODE) ||
      (instance->current_cmd.led_blink_cycle_ms != cmd->led_blink_cycle_ms) ||
      (instance->current_cmd.led_blink_wait_ms != cmd->led_blink_wait_ms) ||
      (instance->current_cmd.led_blink_code_counts !=
       cmd->led_blink_code_counts);

  if (!param_changed) {
    return LED_OK;  // 参数未变化，直接返回
  }

  // 如果 LED 正在编码闪烁模式
  if (fsm_get_current_state(&instance->fsm) == LED_STATE_BLINK_CODE) {
    // 读取当前 LED 状态（true 表示亮，false 表示灭）
    bool is_led_on = led_phys_read(instance);

    if (is_led_on) {
      // LED 亮着，标记需要等待熄灭后更新，将参数暂存到 cmd_fifo
      memcpy(&instance->current_cmd, cmd, sizeof(led_cmd_t));
      instance->current_cmd.led_set_state = LED_STATE_BLINK_CODE;
      return kfifo_put(instance->cmd_fifo,
                       (unsigned char*)&instance->current_cmd,
                       sizeof(led_cmd_t)) == sizeof(led_cmd_t)
                 ? LED_OK
                 : LED_ERROR_INTERNAL;
      instance->pending_blink_update = true;

    } else {
      // LED 已熄灭，立即切换参数
      memcpy(&instance->current_cmd, cmd, sizeof(led_cmd_t));
      instance->pending_blink_update = false;

      // 重置闪烁进度，强制进入间隔阶段，等待间隔结束后再开始闪烁
      instance->current_led_blink_code_counts = 0;
      instance->blink_code_phase = LED_BLINK_PHASE_INTERVAL;
      instance->interval_start_time = led_get_time_now();
      led_phys_write(instance, false);  // 确保保持熄灭状态
      return LED_OK;
    }
  } else {
    // 不在闪烁模式，直接更新参数
    memcpy(&instance->current_cmd, cmd, sizeof(led_cmd_t));
    instance->pending_blink_update = false;
    return LED_OK;
  }
}

/**
 * @brief  获取 LED 闪烁阶段
 * @param  instance 句柄
 * @return 闪烁阶段
 */
led_blink_phase_t led_get_blink_phase(led_handle_t* instance) {
  // 检查参数有效性
  if (instance == NULL) {
    return LED_BLINK_PHASE_INTERVAL;
  }
  return (led_blink_phase_t)instance->blink_code_phase;
}

/**
 * @brief  设置 LED 状态变化回调函数
 * @param  instance 句柄
 * @param  state_cb 状态变化回调
 * @param  blink_phase_cb 闪烁阶段变化回调
 * @param  gpio_edge_cb GPIO 边沿变化回调
 * @param  user_data 用户数据，将传递给回调函数
 */
void led_set_state_change_callback(led_handle_t* instance,
                                   led_state_change_callback_t state_cb,
                                   led_blink_phase_callback_t blink_phase_cb,
                                   led_gpio_edge_callback_t gpio_edge_cb,
                                   void* user_data) {
  // 检查参数有效性
  if (instance == NULL) return;

  instance->state_change_cb = (void*)state_cb;
  instance->blink_phase_cb = (void*)blink_phase_cb;
  instance->gpio_edge_cb = (void*)gpio_edge_cb;
  instance->callback_user_data = user_data;
}

/**
 * @brief  刷新任务，需在主循环周期调用
 */
void led_task_refresh(void) {
  // 检查系统是否已初始化
  if (!led_system_initialized || !led_master) return;

  led_handle_t* curr = led_master;

  while (curr) {
    if (!curr->initialized) {
      curr = curr->next;
      continue;
    }

    // 处理异步命令
    led_process_cmds(curr);

    // 执行 FSM 步进 (驱动状态机运行)
    fsm_step(&curr->fsm);

    // 检测闪烁阶段变化并触发回调
    led_check_blink_phase_change(curr);

    curr = curr->next;
  }
}
