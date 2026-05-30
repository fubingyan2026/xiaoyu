/**
 * @brief:     SVPWM 空间矢量PWM（Q16.16定点版本）
 * @FilePath:   foc_svpwm.h
 * @author:  fubingyan qq:  3245784484
 * @Date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef FOC_SVPWM_H
#define FOC_SVPWM_H

#include "foc_math.h"
#include <stdint.h>

/**
 * @brief SVPWM配置结构体
 */
typedef struct {
    q16_16_t v_bus; /**< 母线电压（Q16.16格式） */
    q16_16_t max_duty_ratio; /**< 最大占空比 */
} foc_svpwm_config_t;

/**
 * @brief SVPWM上下文结构体
 */
typedef struct {
    foc_svpwm_config_t config; /**< 配置参数 */
    /* 输入变量 */
    q16_16_t vd, vq;
    q16_16_t v_alpha, v_beta;
    q16_16_t sin_cos[2]; /**< [0]: sin(theta), [1]: cos(theta) */
    /* 输出变量 */
    q16_16_t ta, tb, tc; /**< 三相占空比 */
    q16_16_t td; /**< 最大占空比（用于ADC同步采样触发） */
} foc_svpwm_context_t;

/**
 * @brief 初始化SVPWM模块
 * @param ctx SVPWM上下文指针
 * @param config SVPWM配置指针
 */
void foc_svpwm_init(foc_svpwm_context_t* ctx, const foc_svpwm_config_t* config);

/**
 * @brief SVPWM计算
 * @param ctx SVPWM上下文指针
 * @note  在此函数执行前，应先调用 foc_ipark_transform() 计算v_alpha和v_beta
 */
void foc_svpwm_calculate(foc_svpwm_context_t* ctx);

#endif /* FOC_SVPWM_H */
