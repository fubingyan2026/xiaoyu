//
// Created by fubingyan on 25-9-5.
//
/**
 * @brief: 按键功能实现 (重构版)
 * @FilePath: key_function.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-08 10:11:53
 * @LastEditTime: 2025-09-29 20:45:25
 * @version: V2.0.0
 * @note: 重构改进：命令表驱动、统一命名、消除重复代码
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "key_menu.h"

#include "CAN_Server.h"
#include "bsp_delay.h"
#include "easyflash.h"
#include "flash_task.h"
#include "foc_ctrl_q16.h"
#include "foc_sm.h"

/* ==================== 配置常量 ==================== */

#define KEY_FUNC_PRINTF(fmt, ...) DEBUG_LOGI("key_menu", fmt, ##__VA_ARGS__)

#define KEY_MULTI_CLICK_TIME_MS 500U   /**< 连击判定窗口 */
#define KEY_LONG_PRESS_TIME_MS 750U    /**< 长按判定时间 */
#define KEY_ENTRY_CMD_TIMEOUT_MS 1000U /**< 数值设置超时时间 */

#define LED_BLINK_INTERVAL_SLOW_MS 250U   /**< LED慢闪间隔 */
#define LED_BLINK_INTERVAL_FAST_MS 100U   /**< LED快闪间隔 */
#define LED_BLINK_INTERVAL_CONFIRM_MS 50U /**< LED确认闪烁间隔 */
#define LED_BLINK_WAIT_MS 1000U           /**< LED闪烁等待间隔 */
#define LED_BLINK_WAIT_SHORT_MS 500U      /**< LED短等待间隔 */

/* ==================== 命令定义 ==================== */

/**
 * @brief 命令类型枚举
 */
typedef enum {
  CMD_TYPE_NONE = 0,      /**< 无命令 */
  CMD_TYPE_MOTOR_CONTROL, /**< 电机控制命令组 */
  CMD_TYPE_SAVE_CAN_ID,   /**< 保存CAN ID命令 */
  CMD_TYPE_ERASE_FLASH,   /**< 擦除Flash命令 */
  CMD_TYPE_MAX,           /**< 最大命令类型 */
} cmd_type_e;

/**
 * @brief 电机控制子命令枚举
 */
typedef enum {
  MOTOR_SUBCMD_STOP = 1,      /**< 停止电机 */
  MOTOR_SUBCMD_CURRENT_25,    /**< 25%电流 */
  MOTOR_SUBCMD_ALIGN,         /**< 对齐模式 */
  MOTOR_SUBCMD_HALL,          /**< Hall模式 */
  MOTOR_SUBCMD_RESET_DEFAULT, /**< 恢复默认 */
} motor_subcmd_e;

/**
 * @brief 命令处理函数类型
 */
typedef void (*cmd_handler_t)(uint8_t value);

/**
 * @brief 命令表项结构
 */
typedef struct {
  uint8_t cmd_id;        /**< 命令ID */
  const char* name;      /**< 命令名称(调试使用) */
  cmd_handler_t handler; /**< 处理函数 */
} cmd_entry_t;

/* ==================== 全局变量 ==================== */

uint32_t can_save_id = 0; /**< 保存的CAN ID (flash_task.c引用) */

/* ==================== 静态变量 ==================== */

static key_fsm_ctx_t s_key_fsm_ctx;                /**< 全局按键 FSM 上下文 */
static led_handle_t* s_led_instance = NULL;        /**< LED实例 */
static key_base_context_t* s_key0_instance = NULL; /**< 按键0实例 */
static const char* s_key_event_names[KEY_BASE_EVENT_MAX] =
    KEY_BASE_EVENT_NAME_TABLE;
static hal_gpio_context_t gpio_ctx;

/* ==================== 前向声明 ==================== */

static void handle_motor_control(uint8_t value);
static void handle_save_can_id(uint8_t value);
static void handle_erase_flash(uint8_t value);
static uint8_t read_key_pin_level(void);
static key_base_event_t on_key_event_callback(key_base_event_t event,
                                              const void* context);
static inline void configure_led_blink(uint16_t interval_ms, uint16_t wait_ms,
                                       uint16_t counts);
/* ==================== 命令表 ==================== */

/**
 * @brief 命令处理表
 * @note 新增命令只需在此表中添加条目，无需修改switch-case
 */
static const cmd_entry_t s_cmd_table[CMD_TYPE_MAX] = {
    {CMD_TYPE_NONE, "none", NULL},
    {CMD_TYPE_MOTOR_CONTROL, "motor_control", handle_motor_control},
    {CMD_TYPE_SAVE_CAN_ID, "save_can_id", handle_save_can_id},
    {CMD_TYPE_ERASE_FLASH, "erase_flash", handle_erase_flash},
};
#define CMD_TABLE_SIZE (sizeof(s_cmd_table) / sizeof(s_cmd_table[0]))

/* ==================== 命令处理函数 ==================== */

/**
 * @brief 处理电机控制命令
 * @param value 子命令值
 */
static void handle_motor_control(uint8_t value) {
  switch (value) {
    case MOTOR_SUBCMD_STOP:
      foc_ctrl.target_iq_q = FLOAT_TO_Q16_16(0.0f);
      KEY_FUNC_PRINTF("Motor: STOP");
      break;
    case MOTOR_SUBCMD_CURRENT_25:
      foc_ctrl.target_iq_q = FLOAT_TO_Q16_16(0.25f);
      KEY_FUNC_PRINTF("Motor: 25%% current");
      break;
    case MOTOR_SUBCMD_ALIGN:
      foc_sm_request_state(foc_sm_get_instance(), FOC_SM_STATE_ALIGN);
      KEY_FUNC_PRINTF("Motor: ALIGN mode");
      break;
    case MOTOR_SUBCMD_HALL:
      foc_sm_request_state(foc_sm_get_instance(), FOC_SM_STATE_HALL);
      KEY_FUNC_PRINTF("Motor: HALL mode");
      break;
    case MOTOR_SUBCMD_RESET_DEFAULT:
    default:
      ef_env_set_default();
      KEY_FUNC_PRINTF("System: Reset to default");
      break;
  }
}

/**
 * @brief 处理保存CAN ID命令
 * @param value CAN ID值
 */
static void handle_save_can_id(uint8_t value) {
  can_save_id = value;
  flash_task_request(FLASH_TASK_WRITE_CAN, &can_save_id, sizeof(can_save_id));
  KEY_FUNC_PRINTF("CAN ID saved: %d", can_save_id);
}

/**
 * @brief 处理擦除Flash命令
 * @param value 无参数
 */
static void handle_erase_flash(uint8_t value) {
  flash_task_request(FLASH_TASK_ERASE_ALL, NULL, 0);
  KEY_FUNC_PRINTF("Flash erased");
}

/**
 * @brief 命令分发函数
 * @param cmd_id 命令ID
 * @param value 命令参数
 */
static void dispatch_command(cmd_type_e cmd_id, uint8_t value) {
  for (uint8_t i = CMD_TYPE_MOTOR_CONTROL; i < CMD_TYPE_MAX; i++) {
    if (s_cmd_table[i].cmd_id == cmd_id) {
      KEY_FUNC_PRINTF("Execute cmd[%s]: %d", s_cmd_table[i].name, value);
      if (s_cmd_table[i].handler) {
        s_cmd_table[i].handler(value);
        return;
      } else {
        KEY_FUNC_PRINTF("Unknown command: %d", cmd_id);
      }
    }
  }
  KEY_FUNC_PRINTF("Unknown command: %d\r\n", cmd_id);
}

/* ==================== 硬件抽象层 ==================== */

/**
 * @brief 读取按键引脚的输入状态
 * @return 1表示按键被按下（低电平），0表示未按下
 */
static uint8_t read_key_pin_level(void) {
  hal_gpio_pin_state_t state;
  hal_gpio_read(&gpio_ctx, HAL_GPIO_PORT_A, HAL_GPIO_PIN_0, &state);
  return state == HAL_GPIO_PIN_RESET;
}

/* ==================== 按键事件处理 ==================== */

/**
 * @brief 检查当前是否在指定状态
 */
static inline bool is_in_state(key_fsm_state_e state) {
  return fsm_current_state(&s_key_fsm_ctx.fsm) == state;
}

/**
 * @brief 在数值设置状态下递增数值
 */
static inline void increment_value_in_set_state(void) {
  if (is_in_state(KEY_FSM_STATE_SET_VALUE)) {
    s_key_fsm_ctx.cmd_value++;
  }
}

/**
 * @brief 在空闲状态下设置命令
 */
static inline void set_command_in_none_state(uint8_t cmd) {
  if (is_in_state(KEY_FSM_STATE_NONE)) {
    s_key_fsm_ctx.cmd_id = cmd;
  }
}

/**
 * @brief 处理长按保持事件
 */
static void handle_long_hold(void) {
  if (is_in_state(KEY_FSM_STATE_NONE)) {
    set_command_in_none_state(CMD_TYPE_ERASE_FLASH);
  } else if (is_in_state(KEY_FSM_STATE_SET_VALUE)) {
    fsm_goto(&s_key_fsm_ctx.fsm, KEY_FSM_STATE_NONE);
    s_key_fsm_ctx.cmd_id = 0;
    KEY_FUNC_PRINTF("SET-COMMAND-CANCEL!");
  }
}

/**
 * @brief 按键事件回调函数
 * @param event 当前的按键事件类型
 * @param context 指向按键上下文的指针
 * @return 返回处理后的按键事件类型
 */
static key_base_event_t on_key_event_callback(key_base_event_t event,
                                              const void* context) {
  const key_base_context_t* key_ptr = context;

  KEY_FUNC_PRINTF("%s:%s,%d!", key_ptr->config.name,
                  s_key_event_names[key_ptr->data.key_event],
                  key_ptr->data.batter_counts);

  if (key_ptr != s_key0_instance) {
    return key_ptr->data.key_event;
  }

  switch (event) {
    case KEY_BASE_EVENT_LONG_WAIT_PRESS:
      increment_value_in_set_state();
      break;

    case KEY_BASE_EVENT_LONG_HOLD:
      handle_long_hold();
      break;

    case KEY_BASE_EVENT_CLICK:
      increment_value_in_set_state();
      break;

    case KEY_BASE_EVENT_ONE_CLICK:
      set_command_in_none_state(CMD_TYPE_MOTOR_CONTROL);
      break;

    case KEY_BASE_EVENT_DOUBLE_CLICK:
      set_command_in_none_state(CMD_TYPE_SAVE_CAN_ID);
      break;

    case KEY_BASE_EVENT_TRIPLE_CLICK:
      break;

    case KEY_BASE_EVENT_DOWN:
    case KEY_BASE_EVENT_LONG_HOLD_RELEASE:
    case KEY_BASE_EVENT_REPEAT_CLICK:
    case KEY_BASE_EVENT_COMBINATION:
    default:
      break;
  }

  return key_ptr->data.key_event;
}

/* ==================== LED回调函数 ==================== */

/**
 * @brief LED 闪烁阶段变化回调
 * @details 当 LED 编码闪烁状态从 BLINKING 切换到 INTERVAL 时，
 *          触发相应的状态机转换操作
 */
static void on_led_blink_phase_change(led_handle_t* instance,
                                      led_blink_phase_t phase,
                                      void* user_data) {
  (void)instance;
  (void)user_data;

  if (phase != LED_BLINK_PHASE_INTERVAL) {
    return;
  }

  key_fsm_state_e current_state = fsm_current_state(&s_key_fsm_ctx.fsm);

  if (current_state == KEY_FSM_STATE_SHOW_COMMAND &&
      s_key_fsm_ctx.set_cmd_flag) {
    fsm_goto(&s_key_fsm_ctx.fsm, KEY_FSM_STATE_SET_VALUE);
  }
}

/**
 * @brief LED GPIO 边沿变化回调
 */
static void on_led_gpio_edge_change(led_handle_t* instance,
                                    hal_gpio_pin_state_t edge,
                                    void* user_data) {
  (void)instance;
  (void)user_data;

  s_key_fsm_ctx.time_last = s_key_fsm_ctx.time_now;

  if (edge == HAL_GPIO_PIN_RESET) {
  } else if (edge == HAL_GPIO_PIN_SET) {
  }
}

/**
 * @brief LED 状态变化回调
 * @details 在 NONE 状态下循环显示 can_save_id
 */
static void on_led_state_change(led_handle_t* instance, led_state_t new_state,
                                void* user_data) {
  (void)instance;
  (void)user_data;

  if (is_in_state(KEY_FSM_STATE_NONE) && new_state == LED_STATE_OFF) {
    KEY_FUNC_PRINTF("[LED] GPIO Release (LED wait)");
    configure_led_blink(LED_BLINK_INTERVAL_SLOW_MS, LED_BLINK_WAIT_MS,
                        can_save_id);
    // led_set_state(s_led_instance, LED_STATE_BREATHING);
  }
}

/* ==================== FSM 状态处理器 ==================== */

/**
 * @brief 配置LED闪烁参数
 */
static inline void configure_led_blink(uint16_t interval_ms, uint16_t wait_ms,
                                       uint16_t counts) {
  led_set_blink_interval(s_led_instance, &(led_cmd_t){
                                             .led_blink_cycle_ms = interval_ms,
                                             .led_blink_wait_ms = wait_ms,
                                             .led_blink_code_counts = counts,
                                         });
  led_set_state(s_led_instance, LED_STATE_BLINK_CODE);
}

/**
 * @brief 状态机进入回调
 */
static void on_fsm_entry(fsm_t* ctx, fsm_state_t state) {
  KEY_FUNC_PRINTF("FSM Enter State: %s", fsm_name(ctx, state));
  key_fsm_ctx_t* fsm_ctx = fsm_user_data(ctx);

  switch (state) {
    case KEY_FSM_STATE_NONE:
      configure_led_blink(LED_BLINK_INTERVAL_SLOW_MS, LED_BLINK_WAIT_MS,
                          can_save_id);
      break;

    case KEY_FSM_STATE_SHOW_COMMAND:
      fsm_ctx->set_cmd_flag = true;
      configure_led_blink(LED_BLINK_INTERVAL_FAST_MS, LED_BLINK_WAIT_SHORT_MS,
                          s_key_fsm_ctx.cmd_id);
      break;

    case KEY_FSM_STATE_SET_VALUE:
      s_key_fsm_ctx.time_last = s_key_fsm_ctx.time_now;
      break;

    default:
      break;
  }
}

/**
 * @brief 状态机退出回调
 */
static void on_fsm_exit(fsm_t* ctx, fsm_state_t state) {
  (void)ctx;
  KEY_FUNC_PRINTF("FSM Exit State: %s", fsm_name(ctx, state));
  key_fsm_ctx_t* fsm_ctx = fsm_user_data(ctx);

  switch (state) {
    case KEY_FSM_STATE_NONE:
      fsm_ctx->cmd_value = 0;
      fsm_ctx->cmd_value_last = 0;
      fsm_ctx->set_cmd_flag = false;
      break;

    case KEY_FSM_STATE_SHOW_COMMAND:
    case KEY_FSM_STATE_SET_VALUE:
    default:
      break;
  }
}

/**
 * @brief NONE 状态处理器 - 空闲状态
 */
static fsm_state_t handle_fsm_none(fsm_t* ctx) {
  (void)ctx;

  if (s_key_fsm_ctx.cmd_id > 0) {
    s_key_fsm_ctx.time_last = s_key_fsm_ctx.time_now;
    return KEY_FSM_STATE_SHOW_COMMAND;
  }
  return KEY_FSM_STATE_NONE;
}

/**
 * @brief SHOW_COMMAND 状态处理器 - 命令显示状态
 */
static fsm_state_t handle_fsm_show_command(fsm_t* ctx) {
  (void)ctx;
  return KEY_FSM_STATE_SHOW_COMMAND;
}

/**
 * @brief SET_VALUE 状态处理器 - 数值设置状态
 */
static fsm_state_t handle_fsm_set_value(fsm_t* ctx) {
  (void)ctx;
  key_fsm_ctx_t* fsm_ctx = &s_key_fsm_ctx;

  if (fsm_ctx->cmd_value_last != fsm_ctx->cmd_value) {
    /* 数值发生变化，更新并闪烁确认 */
    fsm_ctx->cmd_value_last = fsm_ctx->cmd_value;
    fsm_ctx->time_last = fsm_ctx->time_now;
    configure_led_blink(LED_BLINK_INTERVAL_CONFIRM_MS,
                        LED_BLINK_INTERVAL_CONFIRM_MS, 1);
  } else {
    /* 检查超时 */
    if (fsm_ctx->time_now - fsm_ctx->time_last >
        fsm_ctx->cmd_entry_timeout_ms) {
      if (fsm_ctx->cmd_value > 0) {
        dispatch_command(fsm_ctx->cmd_id, fsm_ctx->cmd_value);
      }
      fsm_ctx->cmd_id = 0;
      return KEY_FSM_STATE_NONE;
    }
  }

  return KEY_FSM_STATE_SET_VALUE;
}

/* ==================== 初始化配置 ==================== */

/**
 * @brief LED 默认配置
 */
static const led_config_t s_led_default_config = {
    .led_name = "LED0",
    .port = HAL_GPIO_PORT_B,
    .pin = HAL_GPIO_PIN_9,
    .active_level = HAL_GPIO_PIN_RESET,
    .init_state = LED_STATE_BLINK_CODE,
    .led_refresh_time_ms = 10,
    .led_refresh_cycle_ms = 1000,
    .led_refresh_max_duty = 10000,
    .led_refresh_min_duty = 100,
    .pwm_cfg = {.timer_frequency = 168000000,          // 定时器时钟频率：168MHz
                .pwm_frequency = 5000,                 // PWM输出频率：1kHz
                .duty_cycle = 10000,                   // 初始占空比：50%
                .polarity = HAL_TIM_PWM_POLARITY_LOW,  // 极性：低电平有效
                .timer_instance = HAL_TIM_PWM_INSTANCE_17,  // 定时器实例：TIM17
                .channel = HAL_TIM_PWM_CHANNEL_1,           // PWM通道：通道1
                .gpio =
                    {
                        .port = HAL_GPIO_PORT_B,     // GPIO端口：GPIOB
                        .pin = HAL_GPIO_PIN_9,       // 引脚号：PB9
                        .alternate = GPIO_AF1_TIM17  // 复用功能：TIM17_CH1
                    }},
};

/* 如果没有配置 GPIO 初始化回调，默认配置为输出模式 */
hal_gpio_config_t key0_gpio_cfg = {
    .port = HAL_GPIO_PORT_A,
    .pin = HAL_GPIO_PIN_0,
    .mode = HAL_GPIO_MODE_INPUT,
    .default_state = HAL_GPIO_PIN_RESET,
    .pull = HAL_GPIO_PULL_UP,
    .speed = HAL_GPIO_SPEED_FREQ_LOW,
};

/**
 * @brief 按键默认配置
 */
static const key_base_config_t s_key0_default_config = {
    .name = "key0",
    .read_pin_cb = read_key_pin_level,
    .event_callback = on_key_event_callback,
    .get_time_cb = millis,
    .long_press_time_ms = KEY_LONG_PRESS_TIME_MS,
    .multi_click_time_ms = KEY_MULTI_CLICK_TIME_MS,
};

/* ==================== 公共接口 ==================== */

/**
 * @brief 初始化按键功能
 */
void key_func_init(void) {
  stm32_gpio_init_context(&gpio_ctx);
  hal_gpio_init(&gpio_ctx, &key0_gpio_cfg);
  /* 注册按键 */
  key_base_register(&s_key0_default_config, &s_key0_instance);
  DEBUG_ASSERT(s_key0_instance);

  /* 初始化 LED 子系统 */
  led_init(millis);

  /* 注册 LED */
  s_led_instance = led_register(&s_led_default_config);
  DEBUG_ASSERT(s_led_instance);

  /* 注册 LED 回调 */
  led_set_state_change_callback(s_led_instance, on_led_state_change,
                                on_led_blink_phase_change,
                                on_led_gpio_edge_change, NULL);

  /* 初始化按键 FSM */
  __memset(&s_key_fsm_ctx, 0, sizeof(key_fsm_ctx_t));
  s_key_fsm_ctx.cmd_entry_timeout_ms = KEY_ENTRY_CMD_TIMEOUT_MS;

  static const char* key_state_names[] = {"NONE", "COMMAND", "SET_VALUE"};
  static fsm_handler_t handlers[KEY_FSM_STATE_MAX];
  static fsm_guard_t transitions[KEY_FSM_STATE_MAX * KEY_FSM_STATE_MAX];
  memset(handlers, 0, sizeof(handlers));
  memset(transitions, 0, sizeof(transitions));

  handlers[KEY_FSM_STATE_NONE] = handle_fsm_none;
  handlers[KEY_FSM_STATE_SHOW_COMMAND] = handle_fsm_show_command;
  handlers[KEY_FSM_STATE_SET_VALUE] = handle_fsm_set_value;

  fsm_config_t fsm_cfg = {
      .handlers = handlers,
      .transitions = transitions,
      .state_count = KEY_FSM_STATE_MAX,
      .entry_cb = on_fsm_entry,
      .exit_cb = on_fsm_exit,
      .state_names = key_state_names,
      .user_data = &s_key_fsm_ctx,
  };
  fsm_fill(&fsm_cfg, fsm_always_true);
  fsm_init(&s_key_fsm_ctx.fsm, KEY_FSM_STATE_NONE, &fsm_cfg);

  /* 从 Flash 读取 CAN ID */
  ef_get_env_blob(FLASH_MAGIC_CAN, &can_save_id, sizeof(can_save_id), NULL);
  KEY_FUNC_PRINTF("CAN-ID is:%d", can_save_id);
  configure_led_blink(LED_BLINK_INTERVAL_SLOW_MS, LED_BLINK_WAIT_MS,
      can_save_id);
}

/**
 * @brief 按键功能任务处理
 */
void key_func_task(void) {
  s_key_fsm_ctx.time_sys_last = s_key_fsm_ctx.time_now;
  s_key_fsm_ctx.time_now = millis();
  s_key_fsm_ctx.time_diff =
      s_key_fsm_ctx.time_now - s_key_fsm_ctx.time_sys_last;

  fsm_step(&s_key_fsm_ctx.fsm);
}

/**
 * @brief 获取当前按键功能状态
 */
key_fsm_state_e key_func_get_state(void) {
  return (key_fsm_state_e)fsm_current_state(&s_key_fsm_ctx.fsm);
}

/**
 * @brief 获取当前保存的 CAN ID
 */
uint32_t key_menu_get_can_id(void) { return can_save_id; }
