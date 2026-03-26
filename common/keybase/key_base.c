/**
 * @brief: 按键状态机
 * @FilePath: key_base.c
 * @author: fubingyan qq:3245784484
 * @Date: 2024-04-05 19:14:21
 * @LastEditTime: 2025-11-07 20:04:05
 * @version: V1.0.1
 * @note: add your note!
 * @copyright (c) 2024 by fubingyan, All Rights Reserved.
 */
#include "keybase/key_base.h"
#include "bsp_delay.h"
#include "debug/debug.h"
#include "memory_pool/memory_pool.h"
#include "stdbool.h"

#define KEY_INFO(...) BSP_Printf(__VA_ARGS__)

static const uint8_t public_param_debouncingTime = 50; // ms
static const uint32_t KEY_MIN_TIME_THRESHOLD = 10;     // ms，最小时间阈值
static const uint32_t KEY_MAX_TIME_THRESHOLD = 60000;  // ms，最大时间阈值

static inline uint32_t key_time_diff(uint32_t new_time, uint32_t old_time)
{
    return (new_time >= old_time) ? (new_time - old_time) : (UINT32_MAX - old_time + new_time + 1);
}

static key_base_t *key_master = NULL;
static uint16_t key_count = 0;
static uint8_t key_system_initialized = 0;

/**
 * @brief 获取当前时间函数
 * @return 当前时间，单位：毫秒
 */
static uint32_t get_times(void)
{
    return millis();
}

/**
 * @brief 初始化按键系统
 */
void KeyBaseInit(void)
{
    if (key_system_initialized)
        return;
    key_master = NULL;
    key_count = 0;
    key_system_initialized = 1;
    KEY_INFO("Key system initialized\n");
}

/**
 * @brief 反初始化按键系统，释放所有资源
 */
void KeyBaseDeinit(void)
{
    if (!key_system_initialized)
        return;

    key_base_t *current = key_master;
    while (current)
    {
        key_base_t *next = current->next_key_lists;

        if (current->key_combination)
        {
            current->key_combination->key_combination = NULL;
        }

        if (!current->is_static)
        {
            __free(current);
        }

        current = next;
    }

    key_master = NULL;
    key_count = 0;
    key_system_initialized = 0;
    KEY_INFO("Key system deinitialized\n");
}

/**
 * @brief 获取按键实例数量
 * @return uint16_t 按键实例数量
 */
uint16_t KeyBaseGetCount(void)
{
    return key_count;
}

/**
 * @brief 消抖处理函数
 * @param key_x 按键实例
 * @return None
 */
static void KeyDebounceProcess(key_base_t *key_x)
{
    uint32_t currentTime = get_times();
    key_x->data.timer = currentTime;

    key_x->data.diffTimer = key_time_diff(currentTime, key_x->data.lastTimer);
    if (key_x->data.diffTimer == 0)
        return;

    key_x->data.last_key_pin = key_x->data.key_pin;
    key_x->data.lastTimer = currentTime;

    const uint16_t debouncingCycle = (key_x->data.diffTimer >= public_param_debouncingTime)
                                         ? 1
                                         : (public_param_debouncingTime / key_x->data.diffTimer);

    if (key_x->config.ReadKeyPinLevel())
    {
        if (key_x->data.key_pin != KEY_PRESS_STATE)
        {
            if (key_x->data.pressDebouncingTime++ >= debouncingCycle)
            {
                key_x->data.pressDebouncingTime = 0;
                key_x->data.key_pin = KEY_PRESS_STATE;
                key_x->data.pressStartTime = currentTime;
            }
        }
        if (key_x->data.releaseDebouncingTime)
        {
            key_x->data.releaseDebouncingTime = 0;
        }
    }
    else
    {
        if (key_x->data.key_pin == KEY_PRESS_STATE)
        {
            if (key_x->data.releaseDebouncingTime++ >= debouncingCycle)
            {
                key_x->data.releaseDebouncingTime = 0;
                key_x->data.key_pin = KEY_RELEASE_STATE;
            }
        }
        if (key_x->data.pressDebouncingTime)
        {
            key_x->data.pressDebouncingTime = 0;
        }
    }
}

/**
 * @brief 组合按键处理函数
 * @param key_x 按键实例
 * @return None
 */
static void KeyCombinationProcess(key_base_t *key_x)
{
    // 检查组合按键条件
    if (key_x->key_combination)
    {
        // 检查组合按键伙伴是否也按下
        key_base_t *combo_partner = key_x->key_combination;
        if (combo_partner->data.key_pin == KEY_PRESS_STATE)
        {
            if (key_x->data.key_pin == KEY_PRESS_STATE && !key_x->data.last_key_pin)
            {
                // 标记为组合按键
                key_x->data.combination_is_add = true;
                combo_partner->data.combination_is_add = true;
                // 触发组合按键事件
                key_x->data.keyEvent = COMBINATION_EVENT;
                key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
                // 标记组合事件已处理
                key_x->data.combination_handled = true;
                combo_partner->data.combination_handled = true;
                key_x->data.combination_long_handled = 0;
            }
            else if (key_x->data.key_pin == KEY_PRESS_STATE && key_x->data.last_key_pin == KEY_PRESS_STATE)
            {
                if (!key_x->data.combination_long_handled)
                {
                    const uint32_t effectiveLongPressTime = (key_x->config.longPressTimeMS < KEY_MIN_TIME_THRESHOLD)
                                                                ? KEY_MIN_TIME_THRESHOLD
                                                                : key_x->config.longPressTimeMS;
                    if (key_time_diff(key_x->data.timer, key_x->data.pressStartTime) >= effectiveLongPressTime)
                    {
                        key_x->data.combination_long_handled = 1;
                        key_x->data.keyEvent = COMBINATION_LONG_EVENT;
                        key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
                    }
                }
            }
        }
    }

    // 重置已释放的组合按键状态
    if (key_x->data.combination_is_add && key_x->data.key_pin == KEY_RELEASE_STATE && key_x->data.last_key_pin == KEY_PRESS_STATE)
    {
        key_x->data.combination_is_add = false;
        key_x->data.combination_handled = false;
        key_x->data.combination_long_handled = false;
        if (key_x->key_combination)
        {
            key_x->key_combination->data.combination_is_add = false;
            key_x->key_combination->data.combination_handled = false;
            key_x->key_combination->data.combination_long_handled = false;
        }
    }
}

/**
 * @brief 状态机处理函数
 * @param key_x 按键实例
 * @return None
 */
static void KeyStateMachineProcess(key_base_t *key_x)
{
    if (key_x->data.combination_is_add && key_x->data.combination_handled)
    {
        return;
    }

    const uint32_t effectiveLongPressTime = (key_x->config.longPressTimeMS < KEY_MIN_TIME_THRESHOLD)
                                                ? KEY_MIN_TIME_THRESHOLD
                                                : key_x->config.longPressTimeMS;
    const uint32_t coolingWindow = effectiveLongPressTime / 2;

    if (key_x->data.postLongReleaseTime &&
        key_time_diff(key_x->data.timer, key_x->data.postLongReleaseTime) < coolingWindow)
    {
        if (key_x->data.key_pin == KEY_PRESS_STATE)
            return;
    }

    const uint32_t clickWindow = (key_x->config.multiClickTimeMS > 0)
                                     ? key_x->config.multiClickTimeMS
                                     : effectiveLongPressTime;

    if (key_x->data.last_key_pin != key_x->data.key_pin)
    {
        key_x->data.last_key_pin = key_x->data.key_pin;
        if (key_x->data.key_pin == KEY_PRESS_STATE)
        {
            if (key_time_diff(key_x->data.timer, key_x->data.pressTime) >= effectiveLongPressTime)
            {
                key_x->data.keyEvent = LONG_WAIT_PRESS;
                key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
            }
            else
            {
                key_x->data.keyEvent = CLICK;
                key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
            }
        }
        else
        {
            key_x->data.pressTime = key_x->data.timer;
            if (key_x->data.longHoldState)
            {
                key_x->data.longHoldState = false;
                key_x->data.keyEvent = LONG_HOLD_RELEASE;
                key_x->data.keyBatterEvent = KEY_IDLE;
                key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
            }
            else
            {
                key_x->data.keyEvent = DOWN;
                key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
            }
        }
    }
    else
    {
        if (key_x->data.key_pin == KEY_PRESS_STATE)
        {
            if (!key_x->data.longHoldState)
            {
                if (key_time_diff(key_x->data.timer, key_x->data.pressStartTime) >= effectiveLongPressTime)
                {
                    key_x->data.longHoldState = true;
                    key_x->data.keyEvent = LONG_HOLD;
                    key_x->data.postLongReleaseTime = key_x->data.timer;
                    key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
                }
            }
        }
    }

    switch (key_x->data.keyBatterEvent)
    {
    case KEY_IDLE:
        if (key_x->data.lastKeyEvent != key_x->data.keyEvent)
        {
            if (key_x->data.key_pin == KEY_PRESS_STATE)
            {
                key_x->data.keyBatterCounts = 0;
                key_x->data.keyBatterEvent = KEY_BATTER;
                key_x->data.keyBatterResetTime = key_x->data.timer;
            }
        }
        break;

    case KEY_BATTER:
        if (key_time_diff(key_x->data.timer, key_x->data.keyBatterResetTime) <= clickWindow)
        {
            if (key_x->data.lastKeyEvent != key_x->data.keyEvent)
            {
                key_x->data.keyBatterResetTime = key_x->data.timer;
                if (key_x->data.keyEvent == DOWN)
                {
                    key_x->data.keyBatterCounts++;
                }
            }
        }
        else
        {
            if (key_x->data.key_pin == KEY_RELEASE_STATE)
            {
                if (key_x->data.keyBatterCounts > 0)
                {
                    static const uint8_t click_map[] = {
                        [0] = ONE_CLICK, [1] = DOUBLE_CLICK, [2] = TRIPLE_CLICK, [3] = REPEAT_CLICK};
                    const uint8_t cnt = key_x->data.keyBatterCounts - 1 > sizeof(click_map) - 1
                                            ? sizeof(click_map) - 1
                                            : key_x->data.keyBatterCounts - 1;
                    key_x->data.keyEvent = click_map[cnt];
                    key_x->config.OnBasicCallback(key_x->data.keyEvent, key_x);
                }

                key_x->data.keyBatterCounts = 0;
                key_x->data.keyBatterEvent = KEY_IDLE;
            }
        }
        break;

    default:
        break;
    }

    key_x->data.lastKeyEvent = key_x->data.keyEvent;
}

/**
 * @brief 注册按键实例（动态内存版本）
 * @param config 按键配置指针
 * @param instance 输出参数，返回注册的按键实例指针（可为NULL）
 * @return 错误码
 * @note 使用动态内存分配，适用于支持内存管理的场景
 */
int8_t KeyBaseRegister(key_config_t *config, key_base_t **instance)
{
    if (!config)
    {
        KEY_INFO("KeyBaseRegister failed: config is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!config->name)
    {
        KEY_INFO("KeyBaseRegister failed: config->name is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!key_system_initialized)
    {
        KeyBaseInit();
    }

    key_base_t *existing = GetKeyBaseInstance(config->name);
    if (existing)
    {
        KEY_INFO("KeyBaseRegister warning: key %s already exists, return existing instance\n", config->name);
        if (instance)
            *instance = existing;
        return KEY_OK_EXISTED;
    }

    if (config->ReadKeyPinLevel == NULL)
    {
        KEY_INFO("KeyBaseRegister failed: ReadKeyPinLevel is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (config->OnBasicCallback == NULL)
    {
        KEY_INFO("KeyBaseRegister failed: OnBasicCallback is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    key_base_t *new_key = (key_base_t *)__malloc(sizeof(key_base_t));
    if (!new_key)
    {
        KEY_INFO("KeyBaseRegister failed: memory allocation failed\n");
        return KEY_ERR_NO_MEMORY;
    }

    __memset(new_key, 0, sizeof(key_base_t));
    __memcpy(&new_key->config, config, sizeof(key_config_t));

    new_key->next_key_lists = key_master;
    key_master = new_key;
    key_count++;

    KEY_INFO("KeyBaseRegister success: %s (total: %u)\n", new_key->config.name, key_count);

    if (instance)
        *instance = new_key;

    return KEY_OK;
}

/**
 * @brief 初始化按键实例（静态内存版本）
 * @param config 按键配置指针
 * @param instance 静态内存指针，指向预分配的 key_base_t 内存
 * @return 错误码
 * @note 使用预分配的静态内存，适用于不支持动态内存的场景
 */
int8_t KeyBaseRegisterStatic(key_config_t *config, key_base_t *instance)
{
    if (!config)
    {
        KEY_INFO("KeyBaseRegisterStatic failed: config is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!config->name)
    {
        KEY_INFO("KeyBaseRegisterStatic failed: config->name is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!instance)
    {
        KEY_INFO("KeyBaseRegisterStatic failed: instance is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!key_system_initialized)
    {
        KeyBaseInit();
    }

    key_base_t *existing = GetKeyBaseInstance(config->name);
    if (existing)
    {
        KEY_INFO("KeyBaseRegisterStatic warning: key %s already exists\n", config->name);
        return KEY_ERR_ALREADY_EXIST;
    }

    if (config->ReadKeyPinLevel == NULL || config->OnBasicCallback == NULL)
    {
        KEY_INFO("KeyBaseRegisterStatic failed: callback is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    __memset(instance, 0, sizeof(key_base_t));
    __memcpy(&instance->config, config, sizeof(key_config_t));
    instance->is_static = 1;

    instance->next_key_lists = key_master;
    key_master = instance;
    key_count++;

    KEY_INFO("KeyBaseRegisterStatic success: %s (total: %u)\n", instance->config.name, key_count);

    return KEY_OK;
}

/**
 * @brief 删除按键实例
 * @param name 按键名称
 * @return 错误码
 */
key_error_e KeyBaseUnregister(const char *name)
{
    if (!name)
    {
        KEY_INFO("KeyBaseUnregister failed: name is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    if (!key_system_initialized)
    {
        KEY_INFO("KeyBaseUnregister failed: system not initialized\n");
        return KEY_ERR_INVALID_PARAM;
    }

    key_base_t **ptr = &key_master;
    while (*ptr)
    {
        if (__strcmp((*ptr)->config.name, name) == 0)
        {
            key_base_t *to_unregister = *ptr;
            key_base_t *partner = to_unregister->key_combination;
            *ptr = (*ptr)->next_key_lists;

            if (partner)
            {
                partner->key_combination = NULL;
                partner->data.combination_is_add = 0;
                partner->data.combination_handled = 0;
                partner->data.combination_long_handled = 0;
            }

            to_unregister->key_combination = NULL;
            to_unregister->data.combination_is_add = 0;
            to_unregister->data.combination_handled = 0;
            to_unregister->data.combination_long_handled = 0;

            if (to_unregister->is_static)
            {
                KEY_INFO("KeyBaseUnregister: static key %s skip free\n", name);
            }
            else
            {
                __free(to_unregister);
            }

            key_count--;
            KEY_INFO("KeyBaseUnregister success: %s (remaining: %u)\n", name, key_count);
            return KEY_OK;
        }
        ptr = &(*ptr)->next_key_lists;
    }

    KEY_INFO("KeyBaseUnregister failed: key %s not found\n", name);
    return KEY_ERR_NOT_FOUND;
}

/**
 * @brief 注册组合按键
 * @param control_key_name 控制按键名称
 * @param command_name 命令按键名称
 * @return 错误码
 */
key_error_e KeyCombinationRegister(const char *control_key_name, const char *command_name)
{
    if (command_name == NULL || control_key_name == NULL)
    {
        KEY_INFO("KeyCombinationRegister failed: param is NULL\n");
        return KEY_ERR_INVALID_PARAM;
    }

    key_base_t *command_key = GetKeyBaseInstance(command_name);
    key_base_t *ctrl_key = GetKeyBaseInstance(control_key_name);

    if (command_key == NULL || ctrl_key == NULL)
    {
        KEY_INFO("KeyCombinationRegister failed: key not found\n");
        return KEY_ERR_NOT_FOUND;
    }

    if (command_key == ctrl_key)
    {
        KEY_INFO("KeyCombinationRegister failed: cannot combine key with itself\n");
        return KEY_ERR_INVALID_PARAM;
    }

    command_key->key_combination = ctrl_key;
    ctrl_key->key_combination = command_key;

    KEY_INFO("KeyCombinationRegister success: %s + %s\n", control_key_name, command_name);
    return KEY_OK;
}

/**
 * @brief 按键任务处理函数
 * @note 该函数需在主循环中周期调用
 * @return None
 */
void KeyBaseTask(void)
{
    if (!key_system_initialized || !key_master)
        return;

    key_base_t *key_x = key_master;
    key_base_t *key_next = NULL;

    while (key_x)
    {
        key_next = key_x->next_key_lists;
        if (key_x->config.ReadKeyPinLevel == NULL || key_x->config.OnBasicCallback == NULL)
        {
            key_x = key_next;
            continue;
        }
        KeyDebounceProcess(key_x);
        key_x = key_next;
    }

    key_x = key_master;
    while (key_x)
    {
        key_next = key_x->next_key_lists;
        KeyCombinationProcess(key_x);
        key_x = key_next;
    }

    key_x = key_master;
    while (key_x)
    {
        key_next = key_x->next_key_lists;
        if (key_x->config.ReadKeyPinLevel == NULL || key_x->config.OnBasicCallback == NULL)
        {
            key_x = key_next;
            continue;
        }
        KeyStateMachineProcess(key_x);
        key_x = key_next;
    }
}

/**
 * @brief 获取按键实例
 * @param name 按键名称
 * @return 按键实例指针，未找到返回NULL
 */
key_base_t *GetKeyBaseInstance(const char *name)
{
    if (!name || !key_system_initialized)
        return NULL;
    key_base_t *key_x = key_master;
    while (key_x)
    {
        if (__strcmp(key_x->config.name, name) == 0)
        {
            return key_x;
        }
        key_x = key_x->next_key_lists;
    }
    return NULL;
}

// End of file