//
// Created by fubingyan on 25-9-20.
//

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fsm.h"
#include "hal_gpio.h"
#include "hal_tim_pwm.h"
#include "kfifo.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief LED 操作错误码枚举
 */
typedef enum {
  LED_OK = 0,                   /**< 操作成功 */
  LED_OK_EXISTED = 1,           /**< 已存在 */
  LED_ERROR_INVALID_PARAM = -1, /**< 无效参数 */
  LED_ERROR_NO_MEMORY = -2,     /**< 内存不足 */
  LED_ERROR_NOT_FOUND = -3,     /**< 未找到 */
  LED_ERROR_ALREADY_EXIST = -4, /**< 已存在 */
  LED_ERROR_INTERNAL = -5,      /**< 内部错误 */
} led_error_t;

/**
 * @brief LED 状态枚举
 */
typedef enum __attribute__((packed)) {
  LED_STATE_NONE = 0,   /**< 无状态 */
  LED_STATE_OFF,        /**< 关闭 */
  LED_STATE_ON,         /**< 开启 */
  LED_STATE_BLINK_CODE, /**< 编码闪烁 (异步更新) */
  LED_STATE_BREATHING,  /**< 呼吸灯 (PWM更新) */
  LED_STATE_MAX,        /**< 最大状态 */
} led_state_t;

/**
 * @brief LED 闪烁阶段枚举
 */
typedef enum __attribute__((packed)) {
  LED_BLINK_PHASE_BLINKING = 0, /**< 闪烁中 */
  LED_BLINK_PHASE_INTERVAL,     /**< 间隔中 */
} led_blink_phase_t;

/**
 * @brief LED 控制句柄结构体前向声明
 */
typedef struct led_handle led_handle_t;

/**
 * @brief 时间获取函数指针类型
 */
typedef uint32_t (*led_get_time_func_t)(void);

/**
 * @brief LED 状态变化回调函数类型
 */
typedef void (*led_state_change_callback_t)(led_handle_t* instance,
                                            led_state_t new_state,
                                            void* user_data);

/**
 * @brief LED 闪烁阶段变化回调函数类型
 */
typedef void (*led_blink_phase_callback_t)(led_handle_t* instance,
                                           led_blink_phase_t phase,
                                           void* user_data);

/**
 * @brief LED 引脚电平变化回调函数类型
 */
typedef void (*led_gpio_edge_callback_t)(led_handle_t* instance,
                                         hal_gpio_pin_state_t edge,
                                         void* user_data);

/**
 * @brief LED 初始化配置结构体
 */
typedef struct {
  const char* led_name; /**< LED名称 (唯一标识) */
  hal_gpio_port_t port; /**< GPIO端口 */
  hal_gpio_pin_t pin;   /**< GPIO引脚 */
  hal_gpio_pin_state_t
      active_level; /**< 有效电平 (HAL_GPIO_PIN_SET 或 HAL_GPIO_PIN_RESET) */
  led_state_t init_state; /**< 初始状态 */

  /* 呼吸灯参数 */
  uint16_t led_refresh_time_ms;  /**< 呼吸步进更新间隔 (ms) */
  uint16_t led_refresh_cycle_ms; /**< 呼吸周期 (ms) */
  uint16_t led_refresh_max_duty; /**< 最大亮度 (PWM占空比) */
  uint16_t led_refresh_min_duty; /**< 最小亮度 (PWM占空比) */

  hal_tim_pwm_config_t pwm_cfg; /**< PWM配置 (仅呼吸灯模式需要) */
} led_config_t;

/**
 * @brief LED 异步命令结构体
 */
typedef struct {
  led_state_t led_set_state;      /**< 目标状态 */
  uint16_t led_blink_cycle_ms;    /**< 新闪烁间隔 */
  uint16_t led_blink_wait_ms;     /**< 新等待间隔 */
  uint16_t led_blink_code_counts; /**< 新闪烁次数 */
} led_cmd_t;

/**
 * @brief LED 控制句柄结构体
 */
struct led_handle {
  led_config_t config; /**< 配置副本 */
  fsm_context_t fsm;   /**< FSM 上下文 */

  led_cmd_t current_cmd;        /**< 当前命令 */
  uint32_t last_toggle_time;    /**< 上次翻转时间 */
  uint32_t last_breath_time;    /**< 上次呼吸更新时间 */
  uint32_t interval_start_time; /**< 间隔开始时间 */

  uint16_t current_led_blink_code_counts; /**< 当前轮次闪烁计数 */
  led_blink_phase_t blink_code_phase; /**< 当前闪烁阶段 (led_blink_phase_t) */
  led_blink_phase_t blink_code_phase_last; /**< 上次闪烁阶段，用于检测变化 */

  uint32_t breath_cycle; /**< 呼吸周期计数器 */
  uint16_t breath_value; /**< 当前 PWM 值 */

  bool is_static;            /**< 是否为静态分配 */
  bool initialized;          /**< 是否已初始化 */
  bool pending_blink_update; /**< 是否有待处理的闪烁参数更新（等待 LED 熄灭） */
  bool pwm_init_flag;        /**< PWM 初始化标志位 */

  kfifo_t* cmd_fifo; /**< 异步命令队列句柄 (kfifo_t*) */

  /* 回调函数 */
  void* state_change_cb; /**< 状态变化回调 (led_state_change_callback_t) */
  void* blink_phase_cb;  /**< 闪烁阶段变化回调 (led_blink_phase_callback_t) */
  void* gpio_edge_cb;    /**< GPIO 边沿变化回调 (led_gpio_edge_callback_t) */
  void* callback_user_data; /**< 回调用户数据 */

  struct led_handle* next; /**< 链表指针 */
};

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/**
 * @brief 检查错误码是否为成功
 */
#define LED_IS_OK(err) ((err) >= 0)

/**
 * @brief 检查错误码是否为错误
 */
#define LED_IS_ERR(err) ((err) < 0)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  初始化 LED 子系统
 * @param  get_time_cb 毫秒级时间获取回调函数
 * @return 操作结果错误码
 */
led_error_t led_init(led_get_time_func_t get_time_cb);

/**
 * @brief  反初始化 LED 子系统
 */
void led_deinit(void);

/**
 * @brief  动态注册 LED 实例
 * @param  config 配置指针
 * @return 实例指针，失败返回 NULL
 */
led_handle_t* led_register(const led_config_t* config);

/**
 * @brief  静态注册 LED 实例
 * @param  config 配置指针
 * @param  instance 用户提供的句柄空间
 * @return 操作结果错误码
 */
led_error_t led_register_static(const led_config_t* config,
                                led_handle_t* instance);

/**
 * @brief  注销 LED 实例
 * @param  name LED 名称
 * @return 操作结果错误码
 */
led_error_t led_unregister(const char* name);

/**
 * @brief  设置 LED 状态 (异步)
 * @param  instance 句柄
 * @param  state 目标状态
 */
void led_set_state(led_handle_t* instance, led_state_t state);

/**
 * @brief  设置 LED 闪烁参数 (通过事件/命令触发)
 * @param  instance 句柄
 * @param  cmd 闪烁参数结构体指针
 * @return 操作结果错误码
 */
led_error_t led_set_blink_interval(led_handle_t* instance,
                                   const led_cmd_t* cmd);

/**
 * @brief  获取 LED 实例
 * @param  name 名称
 * @return LED 实例指针，未找到返回 NULL
 */
led_handle_t* led_get_instance(const char* name);

/**
 * @brief  获取 LED 闪烁阶段
 * @param  instance 句柄
 * @return 闪烁阶段
 */
led_blink_phase_t led_get_blink_phase(led_handle_t* instance);

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
                                   void* user_data);

/**
 * @brief  刷新任务，需在主循环周期调用
 */
void led_task_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
