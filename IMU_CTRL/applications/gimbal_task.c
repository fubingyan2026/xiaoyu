/**
 * @brief: 云台控制任务
 * @FilePath: gimbal_task.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-12-13 19:14:21
 * @LastEditTime: 2025-12-14 20:51:42
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "gimbal_task.h"

#include "CAN_Server.h"
#include "PS4_Controller.h"
#include "algorithm/user_lib.h"
#include "app.h"
#include "can_comm/can_comm.h"
#include "ins_task.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"
#include "algorithm/utils_math.h"
#include "algorithm/maths.h"

gimbal_PID_t *gimbal_pid_pitch_instance;
gimbal_PID_t *gimbal_pid_roll_instance;
gimbal_PID_t *gimbal_pid_yaw_instance;

pid_type_def pid_type_velocity_pitch;
pid_type_def pid_type_velocity_roll;
pid_type_def pid_type_velocity_yaw;

static can_comm_rx_t *_can_comm_rx_pitch_motor = NULL;
static can_comm_rx_t *_can_comm_rx_roll_motor = NULL;
static can_comm_rx_t *_can_comm_rx_yaw_motor = NULL;

typedef struct
{
    /* data */
    float pitch;
    float roll;
    float yaw;
} gimbal_pos_target_t;

gimbal_pos_target_t gimbal_pos_target = {0};

void Gimbal_Task_Init(void)
{
    _can_comm_rx_pitch_motor = CANGetRxInstance("gimbal_pitch_motor");
    _can_comm_rx_roll_motor = CANGetRxInstance("gimbal_roll_motor");
    _can_comm_rx_yaw_motor = CANGetRxInstance("gimbal_yaw_motor");

    while (_can_comm_rx_pitch_motor == NULL || _can_comm_rx_roll_motor == NULL || _can_comm_rx_yaw_motor == NULL)
    {
        vTaskDelay(10);
        /* code */
    }

    gimbal_pid_config_t config_pitch = {
        .name = "pitch",
        .kp = 1600,
        .ki = 0.1f,
        .kd = 12.15f,
        .control_cycle = 1,
        .max_iout = 32,
        .max_out = 1023,
    };
    gimbal_pid_pitch_instance = GimbalPidRegister(&config_pitch);
    ASSERT(gimbal_pid_pitch_instance);
    gimbal_pid_config_t config_roll = {
        .name = "roll",
        .kp = 450,
        .ki = 0.125001f,
        .kd = 2.25f,
        .control_cycle = 1,
        .max_iout = 32,
        .max_out = 1023,
    };
    gimbal_pid_roll_instance = GimbalPidRegister(&config_roll);
    ASSERT(gimbal_pid_roll_instance);

    gimbal_pid_config_t config_yaw = {
        .name = "yaw",
        .kp = 450,
        .ki = 0.025f,
        .kd = 2.25f,
        .control_cycle = 1,
        .max_iout = 16,
        .max_out = 500,
    };
    gimbal_pid_yaw_instance = GimbalPidRegister(&config_yaw);
    ASSERT(gimbal_pid_yaw_instance);
}

void Gimbal_Task(void)
{
    gimbal_pos_target.pitch += ps4_rx_pointer->analog.stick.lx / 128.0f * 0.001f;
    gimbal_pos_target.roll += ps4_rx_pointer->analog.stick.ly / 128.0f * 0.001f;
    gimbal_pos_target.yaw += ps4_rx_pointer->analog.stick.rx / 128.0f * 0.001f;

    static uint8_t print_counts = 0;
    if (print_counts++ > 250)
    {
        print_counts = 0;
        BSP_Printf("usart1rxps4P:%d\r\n", (int)gimbal_pos_target.pitch);
        BSP_Printf("usart1rxps4R:%d\r\n", (int)gimbal_pos_target.roll);
        BSP_Printf("usart1rxps4Y:%d\r\n", (int)gimbal_pos_target.yaw);
    }
    utils_norm_angle_rad(&gimbal_pos_target.pitch);
    utils_norm_angle_rad(&gimbal_pos_target.roll);
    utils_norm_angle_rad(&gimbal_pos_target.yaw);

    // if (gimbal_pos_target.pitch > M_PI_2 * 0.5f)
    // {
    //     gimbal_pos_target.pitch = M_PI_2 * 0.5f;
    // }
    // if (gimbal_pos_target.pitch < -M_PI_2 * 0.5f)
    // {
    //     gimbal_pos_target.pitch = -M_PI_2 * 0.5f;
    // }

    // if (gimbal_pos_target.roll > M_PI_2 * 0.5f)
    // {
    //     gimbal_pos_target.roll = M_PI_2 * 0.5f;
    // }
    // if (gimbal_pos_target.roll < -M_PI_2 * 0.5f)
    // {
    //     gimbal_pos_target.roll = -M_PI_2 * 0.5f;
    // }
    // if ((_can_comm_rx_pitch_motor->daemon_can_rx_ptr->data.daemon_frequent > 0) &&
    //     (_can_comm_rx_roll_motor->daemon_can_rx_ptr->data.daemon_frequent > 0))
    {
        /* code */

        gimbal_pid_update_t update_pitch;
        update_pitch.get = get_INS_angle_point()->pit * RAD;
        update_pitch.set = gimbal_pos_target.pitch;
        update_pitch.error_delta = -GetSliverFeedbackMessage(0)->velocity_rpm * M_PIf * 2 / 60.0f; // rpm->rad/s
        update_pitch.gyro = -get_INS_angle_point()->wx;
        GimbalPidUpdateFeedBack(gimbal_pid_pitch_instance, &update_pitch);

        static gimbal_pid_update_t update_roll;
        update_roll.get = get_INS_angle_point()->rol * RAD;
        update_roll.set = M_PI + gimbal_pos_target.roll;
        update_roll.error_delta = GetSliverFeedbackMessage(1)->velocity_rpm * M_PIf * 2 / 60.0f; // rpm->rad/s
        update_roll.gyro = 0; // get_INS_angle_point()->wy;
        GimbalPidUpdateFeedBack(gimbal_pid_roll_instance, &update_roll);

        static gimbal_pid_update_t update_yaw;
        update_yaw.get = get_INS_angle_point()->yaw * RAD;
        update_yaw.set = 0 + gimbal_pos_target.yaw;
        update_yaw.error_delta = -GetSliverFeedbackMessage(2)->velocity_rpm * M_PIf * 2 / 60.0f; // rpm->rad/s
        update_yaw.gyro = get_INS_angle_point()->wz;
        GimbalPidUpdateFeedBack(gimbal_pid_yaw_instance, &update_yaw);
    }

    SetSliverTargetCurrent(0, (int16_t)(gimbal_pid_pitch_instance->data.out));
    SetSliverTargetCurrent(1, -(int16_t)(gimbal_pid_roll_instance->data.out));
    SetSliverTargetCurrent(2, (int16_t)(gimbal_pid_yaw_instance->data.out));
}
