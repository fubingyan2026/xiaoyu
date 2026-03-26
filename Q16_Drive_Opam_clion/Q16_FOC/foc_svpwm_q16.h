/**
 * @brief:     SVPWM 空间矢量PWM（Q16. 16定点版本）
 * @FilePath:   focsvpwm.h
 * @author:  fubingyan qq:  3245784484
 * @Date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef _SVPWM_H
#define _SVPWM_H

#include <stdint.h>
#include "q16_16_math.h"

typedef struct svpwm
{
    /* 输入变量 */
    q16_16_t vd, vq;
    q16_16_t v_alpha, v_beta;
    q16_16_t v_bus;
    q16_16_t sin_cos[2]; /* [0]:  sin(theta), [1]: cos(theta) */
    q16_16_t max_duty_ratio;

    /* 输出变量（占空比） */
    q16_16_t ta, tb, tc; /* 三相占空比 */
    q16_16_t td;         /* 最大占空比（用于ADC同步采样触发） */

    /* 状态变量 */
    int sector; /* 当前扇区（扇区法） */
} svpwm_t;

/**
 * @brief SVPWM计算
 * @param pHandle SVPWM结构体指针
 * @note  在此函数执行前，应先调用 q16_16_ipark_transform() 计算vAlpha和vBeta
 */
void svpwm_calculate(svpwm_t *this);

#endif