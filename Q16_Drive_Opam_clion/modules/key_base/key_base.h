//
// Created by fubingyan on 25-4-12.
//

#ifndef __KEY_BASE_H
#define __KEY_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 按键模块错误码枚举
 */
typedef enum {
  KEY_BASE_OK = 0,                   /**< 操作成功 */
  KEY_BASE_OK_EXISTED,               /**< 已存在（返回已有实例） */
  KEY_BASE_ERROR_INVALID_PARAM = -1, /**< 无效参数 */
  KEY_BASE_ERROR_NO_MEMORY = -2,     /**< 内存不足 */
  KEY_BASE_ERROR_NOT_FOUND = -3,     /**< 未找到 */
  KEY_BASE_ERROR_ALREADY_EXIST = -4, /**< 已存在 */
  KEY_BASE_ERROR_INTERNAL = -5,      /**< 内部错误 */
} key_base_error_t;

/**
 * @brief 按键事件枚举
 */
typedef enum __attribute__((packed)) {
  KEY_BASE_EVENT_DOWN = 0,               /**< 按下 */
  KEY_BASE_EVENT_CLICK,                  /**< 点击 */
  KEY_BASE_EVENT_ONE_CLICK,              /**< 单击 */
  KEY_BASE_EVENT_DOUBLE_CLICK,           /**< 双击 */
  KEY_BASE_EVENT_TRIPLE_CLICK,           /**< 三连击 */
  KEY_BASE_EVENT_REPEAT_CLICK,           /**< 重复点击 */
  KEY_BASE_EVENT_LONG_WAIT_PRESS,        /**< 长按等待 */
  KEY_BASE_EVENT_LONG_HOLD,              /**< 长按保持 */
  KEY_BASE_EVENT_LONG_HOLD_RELEASE,      /**< 长按释放 */
  KEY_BASE_EVENT_COMBINATION,            /**< 组合事件 */
  KEY_BASE_EVENT_COMBINATION_LONG,       /**< 组合长按事件 */
  KEY_BASE_EVENT_MAX,                    /**< 守卫值，必须放在最后 */
} key_base_event_t;

/**
 * @brief 按键引脚状态枚举
 */
typedef enum {
  KEY_BASE_PIN_STATE_PRESS = 0x01,   /**< 按键按下状态 */
  KEY_BASE_PIN_STATE_RELEASE = 0x00, /**< 按键释放状态 */
} key_base_pin_state_t;

/**
 * @brief 按键连击状态机枚举
 */
typedef enum __attribute__((packed)) {
  KEY_BASE_BATTER_STATE_IDLE = 0,   /**< 空闲状态 */
  KEY_BASE_BATTER_STATE_WAIT = 1,   /**< 等待按键点击 */
} key_base_batter_state_t;

/**
 * @brief 按键事件回调函数类型
 * @param event 按键事件类型
 * @param context 按键上下文指针
 * @return 处理后的按键事件类型
 */
typedef key_base_event_t (*key_base_event_cb_t)(key_base_event_t event,
                                                 const void* context);

/**
 * @brief 读取按键引脚电平回调函数类型
 * @return 1表示按键被按下，0表示未按下
 */
typedef uint8_t (*key_base_read_pin_cb_t)(void);

/**
 * @brief 获取当前时间回调函数类型
 * @return 当前时间，单位：毫秒
 */
typedef uint32_t (*key_base_get_time_cb_t)(void);

/**
 * @brief 按键模块配置结构体
 */
typedef struct {
  const char* name;                     /**< 按键名称 */
  key_base_event_cb_t event_callback;   /**< 事件回调函数 */
  key_base_read_pin_cb_t read_pin_cb;   /**< 读取引脚电平回调 */
  key_base_get_time_cb_t get_time_cb;   /**< 获取当前时间回调 */
  uint32_t long_press_time_ms;          /**< 长按判定超时窗口(ms) */
  uint32_t multi_click_time_ms;         /**< 连击判定超时窗口(ms)；为0时退化为long_press_time_ms */
} key_base_config_t;

/**
 * @brief 按键模块上下文结构体前向声明
 */
typedef struct key_base_context key_base_context_t;

/**
 * @brief 按键模块上下文结构体
 */
struct key_base_context {
  key_base_config_t config;             /**< 配置参数 */

  struct {
    uint32_t timer;                     /**< 当前时间戳 */
    uint32_t last_timer;                /**< 上次时间戳 */
    uint32_t press_time;                /**< 按键释放时间戳 */
    uint32_t batter_reset_time;         /**< 连击重置时间戳 */
    uint32_t press_start_time;          /**< 按键按下时的绝对时间戳 */
    uint32_t diff_timer;                /**< 时间差值 */
    uint32_t post_long_release_time;    /**< 长按释放时间戳，用于冷却屏蔽 */

    uint8_t pin_state : 1;              /**< 当前引脚状态 */
    uint8_t last_pin_state : 1;         /**< 上次引脚状态 */
    uint8_t long_hold_state : 1;        /**< 长按保持状态 */
    uint8_t batter_event : 1;           /**< 连击状态机状态 */
    uint8_t combination_active : 1;     /**< 标记当前按键是否在组合按键中 */
    uint8_t combination_handled : 1;    /**< 标记组合事件已处理 */
    uint8_t combination_long_handled : 1; /**< 标记组合长按事件已处理 */
    uint8_t batter_counts;              /**< 按键点击计数 */
    uint16_t press_debounce_count;       /**< 按键按下消抖计数 */
    uint16_t release_debounce_count;     /**< 按键释放消抖计数 */

    uint8_t key_event : 4;              /**< 当前按键事件 */
    uint8_t last_key_event : 4;         /**< 上次按键事件 */
  } data;

  key_base_context_t* combination_partner; /**< 组合按键伙伴 */
  key_base_context_t* next;                /**< 链表下一个节点 */
  uint8_t is_static : 1;                   /**< 标记是否为静态注册 */
};

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/** @brief 按键错误码检查宏 */
#define KEY_BASE_IS_OK(err) ((err) == KEY_BASE_OK)
#define KEY_BASE_IS_ERR(err) ((err) < 0)

/** @brief 按键事件名称表 */
#define KEY_BASE_EVENT_NAME_TABLE                                \
  {                                                              \
    [KEY_BASE_EVENT_DOWN] = "DOWN",                              \
    [KEY_BASE_EVENT_CLICK] = "CLICK",                            \
    [KEY_BASE_EVENT_ONE_CLICK] = "ONE_CLICK",                    \
    [KEY_BASE_EVENT_DOUBLE_CLICK] = "DOUBLE_CLICK",              \
    [KEY_BASE_EVENT_TRIPLE_CLICK] = "TRIPLE_CLICK",              \
    [KEY_BASE_EVENT_REPEAT_CLICK] = "REPEAT_CLICK",              \
    [KEY_BASE_EVENT_LONG_WAIT_PRESS] = "LONG_WAIT_PRESS",        \
    [KEY_BASE_EVENT_LONG_HOLD] = "LONG_HOLD",                    \
    [KEY_BASE_EVENT_LONG_HOLD_RELEASE] = "LONG_HOLD_RELEASE",    \
    [KEY_BASE_EVENT_COMBINATION] = "COMBINATION",                \
    [KEY_BASE_EVENT_COMBINATION_LONG] = "COMBINATION_LONG",      \
  }

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化按键系统
 */
void key_base_init(void);

/**
 * @brief 反初始化按键系统，释放所有资源
 */
void key_base_deinit(void);

/**
 * @brief 注册按键实例（动态内存版本）
 * @param config 按键配置指针
 * @param instance 输出参数，返回注册的按键实例指针（可为NULL）
 * @return 错误码
 * @note 使用动态内存分配，适用于支持内存管理的场景
 */
key_base_error_t key_base_register(const key_base_config_t* config,
                                    key_base_context_t** instance);

/**
 * @brief 注册按键实例（静态内存版本）
 * @param config 按键配置指针
 * @param instance 静态内存指针，指向预分配的key_base_context_t内存
 * @return 错误码
 * @note 使用预分配的静态内存，适用于不支持动态内存的场景
 */
key_base_error_t key_base_register_static(const key_base_config_t* config,
                                           key_base_context_t* instance);

/**
 * @brief 删除按键实例
 * @param name 按键名称
 * @return 错误码
 */
key_base_error_t key_base_unregister(const char* name);

/**
 * @brief 注册组合按键
 * @param control_key_name 控制按键名称
 * @param command_key_name 命令按键名称
 * @return 错误码
 */
key_base_error_t key_base_combination_register(const char* control_key_name,
                                                const char* command_key_name);

/**
 * @brief 获取按键实例
 * @param name 按键名称
 * @return 按键实例指针，未找到返回NULL
 */
key_base_context_t* key_base_get_instance(const char* name);

/**
 * @brief 获取按键实例数量
 * @return 按键实例数量
 */
uint16_t key_base_get_count(void);

/**
 * @brief 按键任务处理函数
 * @note 该函数需在主循环中周期调用
 */
void key_base_task(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_BASE_H */
