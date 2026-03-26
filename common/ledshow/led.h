/**
 * @file led.h
 * @author fubingyan qq:3245784484
 * @brief LED控制模块头文件 (大厂重构版)
 * @details 支持静态/动态注册，PWM/GPIO双模式，时间解耦，异步命令队列控制。
 * @version 2.0.0
 * @date 2025-03-25
 *
 * @copyright Copyright (c) 2025 fubingyan, All Rights Reserved.
 */

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal_gpio.h"
#include "hal_tim_pwm.h"
#include "fsm/fsm.h"
#include <stdbool.h>
#include <stdint.h>
#include "kfifo/kfifo.h"
    /* ==================== 错误码定义 ==================== */
    typedef enum
    {
        LED_OK = 0,                 /**< 成功 */
        LED_OK_EXISTED = 1,         /**< 已存在 */
        LED_ERR_INVALID_PARAM = -1, /**< 无效参数 */
        LED_ERR_NO_MEMORY = -2,     /**< 内存不足 */
        LED_ERR_NOT_FOUND = -3,     /**< 未找到 */
        LED_ERR_ALREADY_EXIST = -4, /**< 已存在 */
        LED_ERR_INTERNAL = -5,      /**< 内部错误 */
    } led_error_e;

#define LED_IS_OK(err) ((err) >= 0)
#define LED_IS_ERR(err) ((err) < 0)

    /* ==================== 枚举与宏定义 ==================== */

    /** @brief LED状态枚举 */
    typedef enum __attribute__((packed))
    {
        LED_STATE_NONE = 0,   /**< 无状态 */
        LED_STATE_OFF,        /**< 关闭 */
        LED_STATE_ON,         /**< 开启 */
        LED_STATE_BLINK_CODE, /**< 编码闪烁 (异步更新) */
        LED_STATE_BREATHING   /**< 呼吸灯 (PWM更新) */
    } led_state_t;

    /** @brief 编码闪烁阶段枚举 */
    typedef enum
    {
        BLINK_CODE_PHASE_BLINKING = 0, /**< 闪烁中 */
        BLINK_CODE_PHASE_INTERVAL,     /**< 间隔中 */
    } blink_code_phase_t;

    /** @brief 时间获取函数指针 */
    typedef uint32_t (*led_get_time_func)(void);

    /* ==================== 配置与句柄结构体 ==================== */

    /** @brief PWM硬件配置 */
    typedef struct
    {
        uint8_t timer_instance;        /**< 定时器实例 (如 17) */
        uint8_t channel;               /**< PWM通道 (如 HAL_TIM_PWM_CHANNEL_1) */
        hal_tim_pwm_config_t *raw_cfg; /**< 指向外部完整的PWM配置 (可选，用于高级控制) */
    } led_pwm_config_t;

    /** @brief LED初始化配置 */
    typedef struct
    {
        const char *led_name;              /**< LED名称 (唯一标识) */
        uint8_t port;                      /**< GPIO端口 */
        uint8_t pin;                       /**< GPIO引脚 */
        hal_gpio_pin_state_e active_level; /**< 有效电平 (HAL_GPIO_PIN_SET 或 HAL_GPIO_PIN_RESET) */
        led_state_t init_state;            /**< 初始状态 */

        /* 闪烁参数 */
        uint16_t blink_interval_ms;      /**< 闪烁半周期间隔 (ms) */
        uint16_t blink_interval_wait_ms; /**< 一轮闪烁后的等待间隔 (ms) */
        uint16_t blink_counts;           /**< 默认一轮闪烁次数 */

        /* 呼吸灯参数 */
        uint16_t breath_interval_ms; /**< 呼吸步进更新间隔 (ms) */
        uint16_t breath_step;        /**< 亮度步进值 */
        uint16_t breath_max;         /**< 最大亮度 (PWM占空比) */
        uint16_t breath_min;         /**< 最小亮度 (PWM占空比) */

        led_pwm_config_t pwm; /**< PWM配置 (仅呼吸灯模式需要) */
    } led_config_t;

    /** @brief LED 异步命令结构体 */
    typedef struct
    {
        led_state_t target_state;        /**< 目标状态 */
        uint16_t blink_interval_ms;      /**< 新闪烁间隔 */
        uint16_t blink_interval_wait_ms; /**< 新等待间隔 */
        uint16_t blink_counts;           /**< 新闪烁次数 */
    } led_cmd_t;

    /** @brief LED 控制句柄 */
    typedef struct led_handle
    {
        led_config_t config; /**< 配置副本 */
        fsm_context_t fsm;   /**< FSM 上下文 */

        uint32_t last_toggle_time;    /**< 上次翻转时间 */
        uint32_t last_breath_time;    /**< 上次呼吸更新时间 */
        uint32_t interval_start_time; /**< 间隔开始时间 */

        uint16_t current_blink_counts;     /**< 当前轮次闪烁计数 */
        uint16_t breath_value;             /**< 当前 PWM 值 */
        uint8_t breath_direction : 1;      /**< 呼吸方向：1-渐亮，0-渐暗 */
        uint8_t blink_code_phase : 1;      /**< 当前闪烁阶段 (blink_code_phase_t) */
        uint8_t last_blink_code_phase : 1; /**< 上次闪烁阶段，用于检测变化 */
        uint8_t is_static : 1;             /**< 是否为静态分配 */
        uint8_t initialized : 1;           /**< 是否已初始化 */
        uint8_t pending_blink_update : 1;  /**< 是否有待处理的闪烁参数更新（等待 LED 熄灭） */

        kfifo_t *cmd_fifo;       /**< 异步命令队列句柄 (kfifo_t*) */
        struct led_handle *next; /**< 链表指针 */

        /* 回调函数 */
        void *state_change_cb;    /**< 状态变化回调 (led_state_change_callback_t) */
        void *blink_phase_cb;     /**< 闪烁阶段变化回调 (led_blink_phase_callback_t) */
        void *gpio_edge_cb;       /**< GPIO 边沿变化回调 (led_gpio_edge_callback_t) */
        void *callback_user_data; /**< 回调用户数据 */
    } led_handle_t;

    /** @brief LED 状态变化回调函数类型 */
    typedef void (*led_state_change_callback_t)(led_handle_t *instance, led_state_t new_state, void *user_data);

    /** @brief LED 闪烁阶段变化回调函数类型 */
    typedef void (*led_blink_phase_callback_t)(led_handle_t *instance, blink_code_phase_t phase, void *user_data);

    /** @brief LED 引脚电平变化回调函数类型 */
    typedef void (*led_gpio_edge_callback_t)(led_handle_t *instance, hal_gpio_pin_state_e edge, void *user_data);

    /* ==================== API 声明 ==================== */

    /**
     * @brief 初始化LED子系统
     * @param get_time_cb 毫秒级时间获取回调
     * @return led_error_e
     */
    led_error_e LedInit(led_get_time_func get_time_cb);

    /**
     * @brief 反初始化LED子系统
     */
    void LedDeinit(void);

    /**
     * @brief 动态注册LED实例
     * @param config 配置指针
     * @return led_handle_t* 实例指针，失败返回NULL
     */
    led_handle_t *LedRegister(const led_config_t *config);

    /**
     * @brief 静态注册LED实例
     * @param config 配置指针
     * @param instance 用户提供的句柄空间
     * @return led_error_e
     */
    led_error_e LedRegisterStatic(const led_config_t *config, led_handle_t *instance);

    /**
     * @brief 注销LED实例
     * @param name LED名称
     * @return led_error_e
     */
    led_error_e LedUnregister(const char *name);

    /**
     * @brief 设置LED状态 (异步)
     * @param instance 句柄
     * @param state 目标状态
     */
    void LedSetState(led_handle_t *instance, led_state_t state);

    /**
     * @brief 设置LED闪烁参数 (通过事件/命令触发)
     * @param instance 句柄
     * @param interval_ms 闪烁间隔
     * @param interval_wait_ms 等待间隔
     * @param counts 闪烁次数
     * @return led_error_e
     */
    led_error_e LedSetBlinkInterval(led_handle_t *instance, uint16_t interval_ms, uint16_t interval_wait_ms, uint16_t counts);

    /**
     * @brief 获取LED实例
     * @param name 名称
     * @return led_handle_t*
     */
    led_handle_t *LedGetInstance(const char *name);

    /**
     * @brief 获取 LED 闪烁阶段
     * @param instance 句柄
     * @return blink_code_phase_t
     */
    blink_code_phase_t LedGetBlinkPhase(led_handle_t *instance);

    /**
     * @brief 设置 LED 状态变化回调函数
     * @param instance 句柄
     * @param state_cb 状态变化回调
     * @param blink_phase_cb 闪烁阶段变化回调
     * @param user_data 用户数据，将传递给回调函数
     */
    void LedSetStateChangeCallback(led_handle_t *instance,
                                   led_state_change_callback_t state_cb,
                                   led_blink_phase_callback_t blink_phase_cb,
                                   led_gpio_edge_callback_t gpio_edge_cb,
                                   void *user_data);

    /**
     * @brief 刷新任务，需在主循环周期调用
     */
    void LedTaskRefresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
