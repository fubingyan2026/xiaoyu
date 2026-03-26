#ifndef KEY_BASE_H
#define KEY_BASE_H

#include "stdint.h"

// 按键错误码
typedef enum
{
    KEY_OK = 0,
    KEY_OK_EXISTED = 1,
    KEY_ERR_INVALID_PARAM = -1,
    KEY_ERR_NO_MEMORY = -2,
    KEY_ERR_NOT_FOUND = -3,
    KEY_ERR_ALREADY_EXIST = -4,
    KEY_ERR_INTERNAL = -5,
} key_error_e;

// 按键错误码检查宏
#define KEY_IS_OK(err) ((err) == KEY_OK)
#define KEY_IS_ERR(err) ((err) < 0)

// 按键事件枚举
typedef enum __attribute__((packed))
{
    DOWN = 0,               // 按下
    CLICK,                  // 点击
    ONE_CLICK,              // 单击
    DOUBLE_CLICK,           // 双击
    TRIPLE_CLICK,           // 三连击
    REPEAT_CLICK,           // 重复点击
    LONG_WAIT_PRESS,        // 长按等待
    LONG_HOLD,              // 长按保持
    LONG_HOLD_RELEASE,      // 长按释放
    COMBINATION_EVENT,      // 组合事件
    COMBINATION_LONG_EVENT, // 组合长按事件
    KEY_EVENT_MAX,          /* 守卫值，必须放在最后 */
} key_event_e;

/* 生成字符串数组的宏 */
#define KEY_EVENT_NAME_TABLE                                 \
    {                                                        \
        [DOWN] = "DOWN",                                     \
        [CLICK] = "CLICK",                                   \
        [ONE_CLICK] = "ONE_CLICK",                           \
        [DOUBLE_CLICK] = "DOUBLE_CLICK",                     \
        [TRIPLE_CLICK] = "TRIPLE_CLICK",                     \
        [REPEAT_CLICK] = "REPEAT_CLICK",                     \
        [LONG_WAIT_PRESS] = "LONG_WAIT_PRESS",               \
        [LONG_HOLD] = "LONG_HOLD",                           \
        [LONG_HOLD_RELEASE] = "LONG_HOLD_RELEASE",           \
        [COMBINATION_EVENT] = "COMBINATION_EVENT",           \
        [COMBINATION_LONG_EVENT] = "COMBINATION_LONG_EVENT", \
    }

// 按键引脚状态枚举
typedef enum
{
    KEY_PRESS_STATE = 0x01,   // 按键按下状态
    KEY_RELEASE_STATE = 0x00, // 按键释放状态
} key_pin_state_e;

// 按键应用状态机
typedef enum __attribute__((packed))
{
    KEY_IDLE = 0,   // 空闲状态
    KEY_BATTER = 1, // 等待按键点击
} key_batter_e;

// 按键配置结构体
typedef struct
{
    key_event_e (*OnBasicCallback)(const key_event_e, const void *);
    uint8_t (*ReadKeyPinLevel)(void);
    uint32_t longPressTimeMS;  /* 长按判定超时窗口(ms) */
    uint32_t multiClickTimeMS; /* 连击判定超时窗口(ms)；为0时退化为 longPressTimeMS */
    char *name;
} key_config_t;

// 按键基础结构体
typedef struct key
{
    struct
    {
        /* data */
        uint32_t timer;
        uint32_t lastTimer;
        uint32_t pressTime;
        uint32_t keyBatterResetTime;
        uint32_t pressStartTime; /* 按键按下时的绝对时间戳，用于时间差式长按判定 */
        uint32_t diffTimer;
        uint32_t postLongReleaseTime; /* 长按释放时间戳，用于冷却屏蔽 */

        uint8_t key_pin : 1;
        uint8_t last_key_pin : 1;
        uint8_t longHoldState : 1;
        uint8_t keyBatterEvent : 1; // 按键应用状态机
        // 新增：组合按键相关
        uint8_t combination_is_add : 1;       // 标记当前按键是否在组合按键中
        uint8_t combination_handled : 1;      // 标记组合事件已处理
        uint8_t combination_long_handled : 1; // 标记组合事件已处理
        uint8_t keyBatterCounts;              // 按键点击计数
        uint8_t pressDebouncingTime;          // 按键按下消抖计时
        uint8_t releaseDebouncingTime;        // 按键释放消抖计时

        uint8_t keyEvent : 4;
        uint8_t lastKeyEvent : 4;
    } data;

    key_config_t config;
    struct key *key_combination;
    struct key *next_key_lists;
    uint8_t is_static : 1; /* 标记是否为静态注册，静态注册实例不能被free */
} key_base_t;

// 函数声明
void KeyBaseInit(void);
void KeyBaseDeinit(void);
int8_t KeyBaseRegister(key_config_t *config, key_base_t **instance);
int8_t KeyBaseRegisterStatic(key_config_t *config, key_base_t *instance);
key_error_e KeyCombinationRegister(const char *control_key_name, const char *command_name);
key_base_t *GetKeyBaseInstance(const char *name);
void KeyBaseTask(void);
key_error_e KeyBaseUnregister(const char *name);
uint16_t KeyBaseGetCount(void);

#endif