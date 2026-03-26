//
// Created by fubingyan on 25-9-5.
//
/**
 * @brief:按键功能实现
 * @FilePath: key_function.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-08 10:11:53
 * @LastEditTime: 2025-09-29 20:45:25
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "key_function.h"
#include "CAN_Server.h"
#include "bsp_delay.h"
#include "debug/debug.h"
#include "easyflash.h"
#include "ledshow/led.h"

uint32_t can_save_id = 0;

#define KEY_FUNC_PRINTF(...) DEBUG_INFO(__VA_ARGS__)

static void key_func_init(void);

static void key_func_task(void);

static void key_comm_callback(uint8_t comm, uint8_t value);

key_func_t key_func = {
    .key_entry_command_times_ms = 1000, // ms
    .key_func_init = key_func_init,
    .key_func_task = key_func_task,
    .key_func_callback = key_comm_callback,
};

static led_handle_t *LedInstance;
static key_base_t *Key0Instance;
static const char *const key_event_name[KEY_EVENT_MAX] = KEY_EVENT_NAME_TABLE;

/**
 * @brief   处理按键通信回调函数
 * @details
 * 该函数根据传入的通信命令和值来执行不同的操作。当`comm`为1时，根据`value`的不同设置FOC机器的状态或调整对齐电流。
 *          当`comm`为2时，保存CAN-ID，并更新电机ID。对于其他`comm`值，目前未定义具体行为。
 * @param comm 通信命令编号，指示要执行的操作类型
 * @param value 与通信命令关联的值，用于进一步指定操作细节
 */
static void key_comm_callback(uint8_t comm, uint8_t value)
{
    switch (comm)
    {
    case 1: /* 电流/状态控制组 */
        if (value == 1)
        {
        }
        else if (value == 2)
        {
         
        }
        else if (value == 3)
        {
            
        }
        else if (value == 4)
        {
            
        }
        else
        {
            ef_env_set_default();
        }
        break;
    case 2: /* 保存 CAN ID */
        can_save_id = value;
        ef_set_env_blob("CAN-ID", &can_save_id, sizeof(can_save_id));
        break;
    default:
        break;
    }
}

/**
 * @brief 读取按键引脚的输入状态
 * @return 返回按键的状态，1表示按键被按下（低电平），0表示未按下。
 */
static uint8_t ReadKeyPinLevel(void)
{
    return !HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin); // 低电平为按下状态;
}

/**
 * @brief 按键事件回调函数，处理不同的按键事件并打印相关信息
 * @param _event 当前的按键事件类型
 * @param _key_event 指向按键实例的指针，用于访问按键的状态和配置
 * @return 返回处理后的按键事件类型
 */
static key_event_e OnBasicCallback(const key_event_e _event, const void *_key_event)
{
    const key_base_t *this_pointer = _key_event;
    KEY_FUNC_PRINTF("%s:%s,%d!\r\n", this_pointer->config.name, key_event_name[this_pointer->data.keyEvent],
                    this_pointer->data.keyBatterCounts);
    if (Key0Instance == this_pointer)
    {
        key_func.timesLast = key_func.times;
    }

    switch (_event)
    {
    case DOWN:
        break;
    case LONG_WAIT_PRESS:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_COMMAND)
            {
                key_func.key_values++;
            }
        }
        break;
    case LONG_HOLD:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_NONE)
            {
                key_func.key_command = 4;
            }
            if (key_func.key_func_state == FUNC_COMMAND)
            {
                key_func.key_func_state = FUNC_NONE;
                key_func.key_command = 0;
                KEY_FUNC_PRINTF("SET-COMMAND-CANCEL!\r\n");
            }
        }
        break;
    case LONG_HOLD_RELEASE:
        if (Key0Instance == this_pointer)
        {
            static uint32_t bootTimes = 0;
            ef_set_env_blob("BT", &bootTimes, sizeof(bootTimes));
        }
        break;
    case CLICK:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_COMMAND)
            {
                key_func.key_values++;
            }
        }
        break;
    case ONE_CLICK:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_NONE)
            {
                key_func.key_command = 1;
            }
        }
        break;
    case DOUBLE_CLICK:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_NONE)
            {
                key_func.key_command = 2;
            }
        }
        break;
    case TRIPLE_CLICK:
        if (Key0Instance == this_pointer)
        {
            if (key_func.key_func_state == FUNC_NONE)
            {
                key_func.key_command = 3;
            }
        }
        break;
    case REPEAT_CLICK:
        break;
    case COMBINATION_EVENT:
        break;
    default:
        break;
    }

    return this_pointer->data.keyEvent; // 获取上一次的按键事件。
}

/**
 * @brief   初始化按键功能相关的设置
 * @details 该函数负责初始化LED配置，注册LED实例，并从环境变量中获取CAN-ID。
 *          根据获取的CAN-ID设置电机ID，并配置LED的闪烁间隔。如果LED注册失败，则进入错误处理循环。
 * @note    该函数应在系统启动时调用以确保按键功能的正确初始化。
 */
static void key_func_init(void)
{
    /* 初始化 LED 子系统 (大厂规范) */
    LedInit(millis);

    key_config_t config = {
        .name = "key0",
        .ReadKeyPinLevel = ReadKeyPinLevel,
        .OnBasicCallback = OnBasicCallback,
        .longPressTimeMS = 500,
    };

    KeyBaseRegister(&config, &Key0Instance);
    ASSERT(Key0Instance);

    const led_config_t led_config = {
        .led_name = "LED0",
        .port = HAL_GPIO_PORT_B,              // GPIOB
        .pin = 9,                             // Pin 9
        .active_level = HAL_GPIO_PIN_RESET,   // 低电平有效
        .init_state = LED_STATE_BLINK_CODE,   // 初始状态为编码闪烁
        .blink_interval_ms = 250,             // 250ms闪烁间隔
        .blink_interval_wait_ms = 1000,       // 1000ms闪烁等待间隔
        .blink_counts = 0,                    // 默认由后续指令控制
        .breath_interval_ms = 5,              // 5ms呼吸更新间隔
        .breath_step = 50,                    // 呼吸步进值
        .breath_max = 10000,                  // 最大亮度
        .breath_min = 0,                      // 最小亮度
        .pwm = {
            .channel = HAL_TIM_PWM_CHANNEL_1, // PWM通道
        },
    };

    LedInstance = LedRegister(&led_config);
    ASSERT(LedInstance);

    ef_get_env_blob("CAN-ID", &can_save_id, sizeof(can_save_id), NULL);
    KEY_FUNC_PRINTF("CAN-ID is:%x\r\n", can_save_id);
    if (can_save_id)
        LedSetBlinkInterval(LedInstance, 250, 1000, can_save_id);
    else
        LedSetBlinkInterval(LedInstance, 100, 100, 0);
}

/**
 * @brief   用于跟踪LED编码闪烁状态变化的变量
 * @details `test_code_state` 和 `test_code_state_last` 用于存储当前和上一次的LED编码闪烁完成状态。
 *           这些变量在检测到LED编码闪烁完成时的状态变化时被更新，以便于触发特定的操作。
 * @note    这些变量主要用于内部状态管理，不应直接在外部修改。
 */
uint8_t test_code_state = BLINK_CODE_PHASE_INTERVAL, test_code_state_last = BLINK_CODE_PHASE_INTERVAL;

#define LED_BLINK_ONE_CODE(app)                                                                                        \
    test_code_state_last = test_code_state;                                                                              \
    test_code_state = LedGetBlinkPhase(LedInstance);                                                                   \
    if (test_code_state_last != test_code_state)                                                                         \
    {                                                                                                                  \
        if (test_code_state == BLINK_CODE_PHASE_INTERVAL)                                                               \
        {                                                                                                              \
            app;                                                                                                       \
        }                                                                                                              \
    }

/**
 * @brief   按键功能任务处理
 * @details 该函数负责处理按键状态机的各个阶段，包括命令检测、值保存和测试。
 *          根据不同的按键状态，执行相应的操作，并更新LED的状态以提供视觉反馈。
 * @note    该函数需要定期调用以确保按键状态机的正常运行。
 */
static void key_func_task(void)
{
    key_func.timesSysLast = key_func.times;
    key_func.times = millis();
    key_func.diffTimes = key_func.times - key_func.timesSysLast;
    static uint8_t last_command = 0;

    switch (key_func.key_func_state)
    {
    case FUNC_NONE:
        if (last_command != (key_func.key_command ? 1 : 0))
        {
            last_command = key_func.key_command ? 1 : 0;
        }
        if (last_command)
        {
            key_func.key_values = 0;
            key_func.key_valuesLast = 0;
            static uint8_t set_mode_flag = 0;
            if (set_mode_flag == 0)
            {
                set_mode_flag = 1;
                LedSetState(LedInstance, LED_STATE_BLINK_CODE);
                LedSetBlinkInterval(LedInstance, 100, 1000, key_func.key_command);
            }
            else
            {
                static uint16_t wait_blink_counts = 0;
                if (wait_blink_counts++ >= 50 / key_func.diffTimes)
                {
                    LED_BLINK_ONE_CODE(LedSetState(LedInstance, LED_STATE_OFF); //
                                       key_func.key_func_state = FUNC_COMMAND;     //
                                       key_func.timesLast = key_func.times;        //
                                       set_mode_flag = 0;                          //
                                       wait_blink_counts = 0;);
                }
            }
        }
        break;
    case FUNC_COMMAND:

        if (key_func.key_valuesLast != key_func.key_values)
        {
            static uint8_t set_led_mode_flag = 0;
            if (set_led_mode_flag == 0)
            {
                set_led_mode_flag = 1;
                LedSetState(LedInstance, LED_STATE_BLINK_CODE);
                LedSetBlinkInterval(LedInstance, 50, 50, 1);
            }
            LED_BLINK_ONE_CODE(key_func.key_valuesLast = key_func.key_values; //
                               LedSetState(LedInstance, LED_STATE_OFF);    //
                               set_led_mode_flag = 0;                         //
            )
        }
        else
        {
            if (key_func.times - key_func.timesLast > key_func.key_entry_command_times_ms)
            {
                if (key_func.key_values)
                {
                    key_func.key_func_state = FUNC_SAVE_VALUE;
                    key_func.key_func_callback(key_func.key_command, key_func.key_values);
                    KEY_FUNC_PRINTF("VALUE IS:[%x].\r\n", key_func.key_values);
                }
                else
                {
                    key_func.key_func_state = FUNC_NONE;
                    KEY_FUNC_PRINTF("SET-COMMAND-TIMEOUT!\r\n");
                    LedSetState(LedInstance, LED_STATE_BLINK_CODE);
                    LedSetBlinkInterval(LedInstance, 250, 1000, can_save_id);
                }
                key_func.key_command = 0;
            }
        }
        break;
    case FUNC_SAVE_VALUE:
        key_func.key_func_state = FUNC_TEST_VALUE;
        KEY_FUNC_PRINTF("SET-COMMAND-SUCCESS!\r\n");
        LedSetState(LedInstance, LED_STATE_BLINK_CODE);
        LedSetBlinkInterval(LedInstance, 250, 1000, can_save_id);
        break;
    case FUNC_TEST_VALUE:
        LED_BLINK_ONE_CODE(key_func.key_func_state = FUNC_NONE;)
        break;
    default:
        break;
    }
}

/**
 * @brief   获取当前按键功能状态
 * @details 该函数返回表示当前按键功能状态的枚举值。状态包括无功能、命令模式、保存值模式和测试值模式。
 * @return 返回当前按键功能状态，类型为key_func_state_e
 */
key_func_state_e key_func_get_state(void)
{
    return key_func.key_func_state;
}

/**
 * @brief   获取当前保存的CAN ID
 * @details 该函数返回当前保存的CAN ID值，用于标识设备在CAN网络中的唯一身份。
 * @return 返回当前保存的CAN ID，类型为uint32_t
 */
uint32_t Get_KeyFunc_CAN_ID(void)
{
    return can_save_id;
}