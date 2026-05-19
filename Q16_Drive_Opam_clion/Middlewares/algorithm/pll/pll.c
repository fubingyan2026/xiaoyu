//
// Created by fubingyan on 26-5-19.
//

/**
 * @file    pll.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    26-5-19
 * @brief   正交锁相环（Quadrature PLL）通用模块实现
 * @attention
 *
 * Copyright (c) 2025 fubingyan.
 * All rights reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "pll.h"

#include "filter.h"
#include "maths.h"
#include "utils_math.h"
#include <math.h>

/* Private constants ---------------------------------------------------------*/

/** @brief 圆周率 π */
#define PLL_PI 3.14159265358979323846f

/** @brief 2π */
#define PLL_2PI 6.28318530717958647692f

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief 对角度进行归一化到 [-PI, PI] 范围
 * @param theta 角度指针
 */
static void pll_normalize_angle(float *theta);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化 PLL
 */
pll_error_t pll_init(pll_context_t *ctx, const pll_config_t *config)
{
    // 检查参数有效性
    if (!ctx || !config)
    {
        return PLL_ERROR_NULL_PTR;
    }

    // 检查配置有效性
    if (config->sample_time <= 0.0f)
    {
        return PLL_ERROR_INVALID_PARAM;
    }

    // 如果已初始化，先反初始化
    if (ctx->initialized)
    {
        pll_deinit(ctx);
    }

    // 保存配置
    ctx->config = *config;

    // 初始化运行时状态
    ctx->theta = 0.0f;
    ctx->omega = 0.0f;
    ctx->intg_pll = 0.0f;

    // 初始化滤波器（如果配置了截止频率）
    if (config->filter_freq_dq > 0.0f)
    {
        float k = pt1FilterGain((uint16_t)config->filter_freq_dq,
                                config->sample_time);
        pt1FilterInit(&ctx->filter_uq_a, k);
        pt1FilterInit(&ctx->filter_uq_b, k);
        ctx->filters_enabled = true;
    }
    else
    {
        ctx->filters_enabled = false;
    }

    if (config->filter_freq_omega > 0.0f)
    {
        float k = pt1FilterGain((uint16_t)config->filter_freq_omega,
                                config->sample_time);
        pt1FilterInit(&ctx->filter_omega, k);
    }

    // 标记初始化完成
    ctx->initialized = true;

    return PLL_OK;
}

/**
 * @brief 反初始化 PLL
 */
void pll_deinit(pll_context_t *ctx)
{
    if (!ctx)
    {
        return;
    }

    // 重置状态
    pll_reset(ctx);
    ctx->initialized = false;
}

/**
 * @brief 检查 PLL 是否已初始化
 */
bool pll_is_initialized(const pll_context_t *ctx)
{
    return (ctx && ctx->initialized);
}

/**
 * @brief 更新 PLL（输入两路正交信号）
 */
pll_error_t pll_update(pll_context_t *ctx, float signal_a, float signal_b)
{
    if (!ctx)
    {
        return PLL_ERROR_NULL_PTR;
    }

    if (!ctx->initialized)
    {
        return PLL_ERROR_UNINITIALIZED;
    }

    // 1. Park 变换（仅计算 q 轴分量用于锁相）
    const float sin_th = sin_approx(ctx->theta);
    const float cos_th = cos_approx(ctx->theta);

    const float uq_a = 2.0f * signal_a * cos_th;
    const float uq_b = -2.0f * signal_b * sin_th;

    // 2. 低通滤波（可选）
    float uq_a_filtered;
    float uq_b_filtered;

    if (ctx->filters_enabled)
    {
        uq_a_filtered = pt1FilterApply(&ctx->filter_uq_a, uq_a);
        uq_b_filtered = pt1FilterApply(&ctx->filter_uq_b, uq_b);
    }
    else
    {
        uq_a_filtered = uq_a;
        uq_b_filtered = uq_b;
    }

    // 3. 相位误差 = 平均 q 轴
    const float err_pll = 0.5f * (uq_a_filtered + uq_b_filtered);

    // 4. PI 计算频率
    ctx->intg_pll += err_pll * ctx->config.ki * ctx->config.sample_time;
    const float omega_raw = ctx->intg_pll + err_pll * ctx->config.kp;

    // 5. 角速度滤波（可选）
    if (ctx->config.filter_freq_omega > 0.0f)
    {
        ctx->omega = pt1FilterApply(&ctx->filter_omega, omega_raw);
    }
    else
    {
        ctx->omega = omega_raw;
    }

    // 6. 积分得角度
    ctx->theta += ctx->omega * ctx->config.sample_time;
    pll_normalize_angle(&ctx->theta);

    return PLL_OK;
}

/**
 * @brief 获取估计角度
 */
float pll_get_angle(const pll_context_t *ctx)
{
    if (!ctx || !ctx->initialized)
    {
        return 0.0f;
    }

    return ctx->theta;
}

/**
 * @brief 获取估计角速度
 */
float pll_get_speed(const pll_context_t *ctx)
{
    if (!ctx || !ctx->initialized)
    {
        return 0.0f;
    }

    return ctx->omega;
}

/**
 * @brief 重置 PLL 状态（保持配置不变）
 */
pll_error_t pll_reset(pll_context_t *ctx)
{
    if (!ctx)
    {
        return PLL_ERROR_NULL_PTR;
    }

    ctx->theta = 0.0f;
    ctx->omega = 0.0f;
    ctx->intg_pll = 0.0f;

    // 重新初始化滤波器
    if (ctx->config.filter_freq_dq > 0.0f)
    {
        float k = pt1FilterGain((uint16_t)ctx->config.filter_freq_dq,
                                ctx->config.sample_time);
        pt1FilterInit(&ctx->filter_uq_a, k);
        pt1FilterInit(&ctx->filter_uq_b, k);
    }

    if (ctx->config.filter_freq_omega > 0.0f)
    {
        float k = pt1FilterGain((uint16_t)ctx->config.filter_freq_omega,
                                ctx->config.sample_time);
        pt1FilterInit(&ctx->filter_omega, k);
    }

    return PLL_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 对角度进行归一化到 [-PI, PI] 范围（O(1) 整数除法，无库函数调用）
 */
static void pll_normalize_angle(float *theta)
{
    if (*theta > PLL_PI)
    {
        *theta -= PLL_2PI * (float)(int32_t)((*theta + PLL_PI) / PLL_2PI);
    }
    else if (*theta < -PLL_PI)
    {
        *theta += PLL_2PI * (float)(int32_t)((-*theta + PLL_PI) / PLL_2PI);
    }
}
