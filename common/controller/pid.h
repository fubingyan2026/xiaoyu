#ifndef PID_H
#define PID_H

#include "stdint.h"
#include "stdlib.h"

#define LimitMax(input, max)                                                                                           \
    {                                                                                                                  \
        if (input > max)                                                                                               \
        {                                                                                                              \
            input = max;                                                                                               \
        }                                                                                                              \
        else if (input < -max)                                                                                         \
        {                                                                                                              \
            input = -max;                                                                                              \
        }                                                                                                              \
    }

enum PID_MODE
{
    PID_POSITION = 0, // 位置PID
    PID_DELTA         // 增量PID
};

typedef struct
{
    uint8_t mode;
    // PID 三参数
    float Kp; // 比例系数
    float Ki; // 积分系数
    float Kd; // 阻尼系数
    float Kf; // 前馈系数

    float max_out;  // 最大输出
    float max_iout; // 最大积分输出

    float set;
    float fdb;

    float out;
    float Pout;
    float Iout;
    float Dout;
    float Dbuf[3];  // 微分项 0最新 1上一次 2上上次
    float error[3]; // 误差项 0最新 1上一次 2上上次
} pid_type_def;

/**
 * @brief          pid struct data init
 * @param[out]     pid: PID结构数据指针
 * @param[in]      mode: PID_POSITION:普通PID
 *                 PID_DELTA: 差分PID
 * @param[in]      PID: 0: kp, 1: ki, 2:kd
 * @param[in]      max_out: pid最大输出
 * @param[in]      max_iout: pid最大积分输出
 * @retval         none
 */
void PID_init(pid_type_def *pid, uint8_t mode, const float PID[4], float max_out, float max_iout);

/**
 * @brief          pid计算
 * @param[out]     pid: PID结构数据指针
 * @param[in]      ref: 反馈数据
 * @param[in]      set: 设定值
 * @retval         pid输出
 */
float PID_calc(pid_type_def *pid, float ref, float set);

/**
 * @brief          pid 输出清除
 * @param[out]     pid: PID结构数据指针
 * @retval         none
 */
void PID_clear(pid_type_def *pid);
#endif
