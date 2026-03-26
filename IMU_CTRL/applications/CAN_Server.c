/**
 * @brief: CAN通讯服务
 * @FilePath: CAN_Server.c
 * @author: fubingyan qq:3245784484
 * @Date: 2024-04-05 19:14:21
 * @LastEditTime: 2025-12-08 20:51:42
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2024 by fubingyan, All Rights Reserved.
 */

#include "CAN_Server.h"
#include "can_comm/can_comm.h"
#include "can_comm/can_nm.h"
#include "debug/debug.h"
#include "key_function.h"
#include "kfifo/kfifo.h"
#include "perf_counter.h"
#include "daemon/daemon_def.h"
#include "stm32g4xx_hal_fdcan.h"
#include "app.h"
#define CAN_SERVER_INFO(...) // DEBUG_WARN(__VA_ARGS__)

#define FIRMWARE_UPDATE_MAGIC_WORD 0xA5A5A5A5   /* 固件需要更新的特殊标记（不建议修改，一定要和 APP 一致） */
#define FIRMWARE_RECOVERY_MAGIC_WORD 0x5A5A5A5A /* 需要恢复出厂固件的特殊标记（不建议修改，一定要和 APP 一致） */
#define BOOTLOADER_RESET_MAGIC_WORD 0xAAAAAAAA  /* bootloader 复位的特殊标记（不建议修改，一定要和 APP 一致） */

#if defined(__IS_COMPILER_ARM_COMPILER_5__)
volatile uint32_t update_flag __attribute__((at(0x20000000), zero_init));
#elif defined(__IS_COMPILER_ARM_COMPILER_6__)
#define __INT_TO_STR(x) #x
#define INT_TO_STR(x) __INT_TO_STR(x)
volatile uint32_t update_flag __attribute__((section(".bss.ARM.__at_" INT_TO_STR(0x20000000))));
#else
__attribute__((section(".noinitram.system_boot_count"), used)) volatile uint32_t update_flag;
__attribute__((section(".noinitram.system_device_name"), used)) volatile char device_name[4];
#endif

static can_comm_rx_t *can_comm_rx_pitch_motor = NULL;
static can_comm_rx_t *can_comm_rx_roll_motor = NULL;
static can_comm_rx_t *can_comm_rx_yaw_motor = NULL;
static can_comm_tx_t *can_comm_tx_sliver_instance = NULL;

static can_send_sliver_data_t can_send_sliver_data;
static can_get_sliver_data_t can_get_sliver_data[CAN_SLIVER_COUNTS];

static void Set_Bootloader_Reboot(void)
{
    update_flag = FIRMWARE_UPDATE_MAGIC_WORD;
    HAL_NVIC_SystemReset();
}

/* 应用层回调函数 */
static void app_sleep_callback(void)
{
    // DEBUG_INFO("App: Entering sleep mode\n");
    /* 执行应用层睡眠操作 */
    // stop_application_tasks();
    // configure_low_power_mode();
}

static void app_wakeup_callback(void)
{
    // DEBUG_INFO("App: Waking up from sleep\n");
    /* 执行应用层唤醒操作 */
    // start_application_tasks();
    // configure_normal_power_mode();
}

static uint8_t app_sleep_condition_check(void)
{
    /* 检查应用层是否允许睡眠 */
    // return (get_pending_tasks() == 0 &&
    //         is_user_inactive() &&
    //         is_battery_low() == 0);
    return true;
}

static uint8_t app_wakeup_condition_check(void)
{
    /* 检查应用层唤醒条件 */
    // return (is_user_button_pressed() ||
    //         is_rtc_alarm_triggered() ||
    //         is_communication_requested());
    return true;
}

static void app_node_status_callback(uint8_t node_id, uint8_t is_online)
{
    if (is_online)
    {
        // DEBUG_INFO("App: Node %d came online\n", node_id);
    }
    else
    {
        // DEBUG_INFO("App: Node %d went offline\n", node_id);
    }
}

/**
 * @brief : CAN初始化
 * @retval: None
 */
void FDCAN1_Config(void)
{
    update_flag = 0xFFFFFFFF;
    /* 打开所有错误中断源，掩码即可 */

    FDCAN_FilterTypeDef sFilterConfig;
    // 先禁用所有滤波器
    hfdcan1.Instance->RXGFC = 0x00000000;
    // 方法一：使用多个掩码滤波器（最灵活）
    // 配置滤波器0：接收ID 0x01
    sFilterConfig.IdType = FDCAN_STANDARD_ID;             // 配置为过滤标准帧
    sFilterConfig.FilterIndex = 0;                        // 过滤器的索引号
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;         // 掩码模式
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1; // 关联到FIFO0
    sFilterConfig.FilterID1 = 0x01 << 18;                 // 要匹配的ID：0x01
    sFilterConfig.FilterID2 = 0x7FF << 18;                // 掩码：所有11位必须匹配
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // 配置滤波器1：接收ID 0x02
    sFilterConfig.FilterIndex = 1;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1; // 也可以分配到FIFO1分担负载
    sFilterConfig.FilterID1 = 0x02 << 18;
    sFilterConfig.FilterID2 = 0x7FF << 18;
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // 配置滤波器2：接收ID 0x03，分配到FIFO1以平衡负载
    sFilterConfig.FilterIndex = 2;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1; // 关联到FIFO1
    sFilterConfig.FilterID1 = 0x03 << 18;
    sFilterConfig.FilterID2 = 0x7FF << 18;
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_FDCAN_Start(&hfdcan1); // 开启FDCAN
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO1_NEW_MESSAGE,
                                   FDCAN_IT_RX_FIFO1_FULL | FDCAN_IT_RX_FIFO1_MESSAGE_LOST);
    /* 开启 FDCAN 的 TX FIFO Empty 中断 */
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_TX_FIFO_EMPTY, 0);
    /* 打开所有错误中断源，掩码即可 */
    HAL_FDCAN_ActivateNotification(&hfdcan1,
                                   FDCAN_IT_BUS_OFF | FDCAN_IT_ERROR_WARNING | FDCAN_IT_ERROR_PASSIVE |
                                       FDCAN_IT_ARB_PROTOCOL_ERROR | FDCAN_IT_DATA_PROTOCOL_ERROR |
                                       FDCAN_IT_RX_FIFO0_FULL | FDCAN_IT_RX_FIFO1_FULL,
                                   0);
    CANCommInit();
}

/**
 * @brief 守护进程回调函数，当CAN接收节点离线时被调用
 * @param rx 触发此回调的CAN接收节点指针
 * @note 当节点离线时记录错误信息，在线时记录状态信息
 */
static void daemon_can_rx_offline_callback(void *rx)
{
    const can_comm_rx_t *this_ptr = (can_comm_rx_t *)rx;
    /* 检查指针有效性 */
    if (this_ptr == NULL || this_ptr->daemon_can_rx_ptr == NULL)
    {
        return;
    }
    /* 根据在线状态记录相应信息 */
    if (!this_ptr->daemon_can_rx_ptr->data.online)
    {
        DEBUG_ERROR("%s: offline,frequent:%d\n", this_ptr->config.name,
                    (int)this_ptr->daemon_can_rx_ptr->data.daemon_frequent);
        g_can_stats.timeout_errors++; /* 增加超时错误计数 */
    }
    else
    {
        DEBUG_INFO("%s: online,frequent:%d\n", this_ptr->config.name,
                   (int)this_ptr->daemon_can_rx_ptr->data.daemon_frequent);
    }
}

/**
 * @brief 初始化CAN通信模块
 * @note 注册所有预定义的CAN接收和发送实例
 */
void CANCommInit(void)
{
    /* 初始化INS角度和陀螺仪接收配置 */
    can_rx_config_t can_rx_config_pitch = {
        .instance = HAL_FDCAN_INSTANCE_1,
        .can_rx_identify = DAEMON_CAN_PITCH,
        .callback = daemon_can_rx_offline_callback,
        .name = "gimbal_pitch_motor",
        .rx_data_len = sizeof(can_get_sliver_data_t),
        .offline_ms = 250,
        .daemon_error_code = DAEMON_CAN_PITCH,
        .priority = 2,
    };
    can_comm_rx_pitch_motor = CANRxRegister(&can_rx_config_pitch);
    ASSERT(can_comm_rx_pitch_motor);

    can_rx_config_t can_rx_config_roll = {
        .instance = HAL_FDCAN_INSTANCE_1,
        .can_rx_identify = DAEMON_CAN_ROLL,
        .callback = daemon_can_rx_offline_callback,
        .name = "gimbal_roll_motor",
        .rx_data_len = sizeof(can_get_sliver_data_t),
        .offline_ms = 250,
        .daemon_error_code = DAEMON_CAN_ROLL,
        .priority = 2,
    };
    can_comm_rx_roll_motor = CANRxRegister(&can_rx_config_roll);
    ASSERT(can_comm_rx_roll_motor);

    can_rx_config_t can_rx_config_yaw = {
        .instance = HAL_FDCAN_INSTANCE_1,
        .can_rx_identify = DAEMON_CAN_YAW,
        .callback = daemon_can_rx_offline_callback,
        .name = "gimbal_yaw_motor",
        .rx_data_len = sizeof(can_get_sliver_data_t),
        .offline_ms = 250,
        .daemon_error_code = DAEMON_CAN_YAW,
        .priority = 2,
    };
    can_comm_rx_yaw_motor = CANRxRegister(&can_rx_config_yaw);
    ASSERT(can_comm_rx_yaw_motor);

    /* 初始化电机发送配置 */
    const can_tx_config_t can_tx_config = {
        .instance = HAL_FDCAN_INSTANCE_1,
        .can_tx_identify = 0x100,
        .tx_data_len = sizeof(can_send_sliver_data_t),
        .name = "master_controller",
        .priority = 2,
    };
    can_comm_tx_sliver_instance = CANTxRegister(&can_tx_config);
    ASSERT(can_comm_tx_sliver_instance);

    /* 网络管理初始化（节点ID为1） */
    // CANNmInit(0xFF, app_sleep_callback, app_wakeup_callback, app_sleep_condition_check, app_wakeup_condition_check,
    //           app_node_status_callback);
}

void FDCAN_Server_Task(void)
{
    can_get_sliver_data[0].velocity_rpm =
        ((can_get_sliver_data_t *)CANRxBindData(can_comm_rx_pitch_motor))->velocity_rpm;
    can_get_sliver_data[1].velocity_rpm =
        ((can_get_sliver_data_t *)CANRxBindData(can_comm_rx_roll_motor))->velocity_rpm;
    can_get_sliver_data[2].velocity_rpm = ((can_get_sliver_data_t *)CANRxBindData(can_comm_rx_yaw_motor))->velocity_rpm;
}

/**
 * @brief 获取电机的反馈数据
 * @param index (0-CAN_SLIVER_COUNTS-1)
 * @return 电机的数据指针
 */
can_get_sliver_data_t *GetSliverFeedbackMessage(uint8_t index)
{
    if (index >= (CAN_SLIVER_COUNTS))
        return NULL;
    return &can_get_sliver_data[index];
}

void SetSliverTargetCurrent(uint8_t index, int16_t current)
{
    if (index >= (CAN_SLIVER_COUNTS))
        return;
    ((can_send_sliver_data_t *)CANTxBindData(can_comm_tx_sliver_instance))->velocity_rpm[index] = (int16_t)(current << 4);
}
// end of the file !
