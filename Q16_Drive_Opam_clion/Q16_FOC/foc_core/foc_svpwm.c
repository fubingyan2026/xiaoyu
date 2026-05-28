/**
 * @brief:    SVPWM 空间矢量PWM（Q16.16定点版本）
 * @FilePath:  foc_svpwm.c
 * @author:  fubingyan qq: 3245784484
 * @Date: 2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#include "foc_svpwm.h"
#include "foc_config.h"

/**
 * @brief 三次谐波注入法SVPWM实现
 * 注入三次谐波以扩展线性调制范围，适用于需要最大输出电压的应用。
 * 原理：
 * - 在基本波基础上注入最优三次谐波
 * - 使PWM波形从正弦变为"马鞍波"
 * - 增加基波幅值约15%
 * - 计算量更小，不需要扇区判断
 * @param ctx SVPWM上下文指针
 */
static void svpwm_calculate_harmonic(foc_svpwm_context_t* ctx)
{
    // 计算归一化电压（避免溢出，使用 64 位中间变量）
    q16_16_t v_bus_inv = q16_16_div(Q16_16_ONE, ctx->config.v_bus); // V_bus 倒数

    // 逆 Clark 变换：从 Vα/Vβ 计算归一化 Va、Vb、Vc
    q16_16_t va = q16_16_mul(ctx->v_alpha, v_bus_inv);

    // Vb = (-1/2)*Vα + (√3/2)*Vβ
    q16_16_t vb_temp = q16_16_add(q16_16_mul(-Q16_16_HALF, ctx->v_alpha), q16_16_mul(Q16_16_SQRT3_2, ctx->v_beta));
    q16_16_t vb = q16_16_mul(vb_temp, v_bus_inv);

    // Vc = (-1/2)*Vα + (-√3/2)*Vβ
    q16_16_t vc_temp = q16_16_add(q16_16_mul(-Q16_16_HALF, ctx->v_alpha), q16_16_mul(-Q16_16_SQRT3_2, ctx->v_beta));
    q16_16_t vc = q16_16_mul(vc_temp, v_bus_inv);

    // 找到最小值和最大值
    q16_16_t min_v = q16_16_min(q16_16_min(va, vb), vc);
    q16_16_t max_v = q16_16_max(q16_16_max(va, vb), vc);

    // 计算三次谐波注入偏移量（标准公式：v_offset = -(V_max + V_min)/2）
    q16_16_t v_offset = q16_16_mul(q16_16_add(min_v, max_v), -Q16_16_HALF);

    // 应用三次谐波注入并调整到 [0, 1] 范围
    // 标准公式：V' = V + v_offset，然后加上0.5中心化到[0,1]
    ctx->ta = q16_16_add(q16_16_add(va, v_offset), Q16_16_HALF);
    ctx->tb = q16_16_add(q16_16_add(vb, v_offset), Q16_16_HALF);
    ctx->tc = q16_16_add(q16_16_add(vc, v_offset), Q16_16_HALF);
}

/**
 * @brief 初始化SVPWM模块
 * @param ctx SVPWM上下文指针
 * @param config SVPWM配置指针
 */
void foc_svpwm_init(foc_svpwm_context_t* ctx, const foc_svpwm_config_t* config)
{
    if (!ctx || !config) {
        return;
    }

    ctx->config = *config;
    ctx->vd = 0;
    ctx->vq = 0;
    ctx->v_alpha = 0;
    ctx->v_beta = 0;
    ctx->sin_cos[0] = 0;
    ctx->sin_cos[1] = Q16_16_ONE;
    ctx->ta = 0;
    ctx->tb = 0;
    ctx->tc = 0;
    ctx->td = 0;
}

/**
 * @brief SVPWM计算
 *
 * 执行流程：
 * 1. 检查母线电压有效性
 * 2. 计算电压矢量幅度，进行过调制处理
 * 3. 使用三次谐波注入法计算占空比
 * 4. 限幅并输出三相占空比
 *
 * @param ctx SVPWM上下文指针
 *        - 输入：v_alpha, v_beta (需预先设置)
 *        - 输出：ta, tb, tc, td
 */
void foc_svpwm_calculate(foc_svpwm_context_t* ctx)
{
    // 输入合理性检查
    if (ctx->config.v_bus <= 0 || ctx->config.v_bus > FLOAT_TO_Q16_16(V_BUS_MAX)) {
        ctx->ta = ctx->tb = ctx->tc = ctx->td = 0;
        return;
    }

    // 计算电压矢量幅度的平方
    q16_16_t v_alpha_sq = q16_16_mul(ctx->v_alpha, ctx->v_alpha);
    q16_16_t v_beta_sq = q16_16_mul(ctx->v_beta, ctx->v_beta);
    q16_16_t v_mag_sq = q16_16_add(v_alpha_sq, v_beta_sq);

    // 最大允许电压（内切圆，保留余度）
    // max_v = Vbus / √3 · duty_ratio
    q16_16_t max_v_radius = q16_16_mul(ctx->config.v_bus, Q16_16_INV_SQRT3);
    q16_16_t max_v_limited = q16_16_mul(max_v_radius, ctx->config.max_duty_ratio);
    q16_16_t max_v_sq = q16_16_mul(max_v_limited, max_v_limited);

    // 过调制检查和幅度缩放
    if (v_mag_sq > max_v_sq) {
        q16_16_t inv_mag = foc_inv_sqrt(v_mag_sq);

        // 计算缩放因子
        q16_16_t scale = q16_16_mul(max_v_limited, inv_mag);

        // 应用缩放
        ctx->v_alpha = q16_16_mul(ctx->v_alpha, scale);
        ctx->v_beta = q16_16_mul(ctx->v_beta, scale);
    }

    /* 使用三次谐波注入法SVPWM */
    svpwm_calculate_harmonic(ctx);

    /* 限幅处理，确保占空比在[0, 1]之间 */
    ctx->ta = q16_16_clip(ctx->ta, 0, Q16_16_ONE);
    ctx->tb = q16_16_clip(ctx->tb, 0, Q16_16_ONE);
    ctx->tc = q16_16_clip(ctx->tc, 0, Q16_16_ONE);

    /* 计算最大占空比（用于ADC同步采样） */
    ctx->td = q16_16_max(ctx->ta, q16_16_max(ctx->tb, ctx->tc));
}
