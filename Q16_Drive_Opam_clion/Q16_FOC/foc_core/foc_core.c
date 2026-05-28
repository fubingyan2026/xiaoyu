/**
 * @file    foc_core.c
 * @brief   FOC核心算法实现
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V1.0.0
 */

#include "foc_core.h"

#include "foc.h"

/* ==================== 导出函数 ==================== */

/**
 * @brief FOC电流环计算（核心算法）
 */
void foc_core_current_loop(foc_context_t* ctx, q16_16_t electrical_angle_q)
{
    if (!ctx || !ctx->initialized) {
        return;
    }

    // 计算电气角度的正弦和余弦值
    foc_sin_cos(electrical_angle_q, &ctx->svpwm.sin_cos[0], &ctx->svpwm.sin_cos[1]);

    // 读取ADC电流采样
    foc_port_adc_read(&ctx->port, &ctx->current_sample[0], &ctx->current_sample[1], &ctx->current_sample[2]);

    q16_16_t ia_q = ctx->current_sample[0];
    q16_16_t ib_q = ctx->current_sample[1];
    q16_16_t ic_q = ctx->current_sample[2];

    // Clarke变换
    q16_16_t i_alpha_q = 0;
    q16_16_t i_beta_q = 0;
    foc_clarke_transform(ia_q, ib_q, ic_q, &i_alpha_q, &i_beta_q);

    // Park变换
    q16_16_t id_raw_q = 0;
    q16_16_t iq_raw_q = 0;
    foc_park_transform(i_alpha_q, i_beta_q, ctx->svpwm.sin_cos[0], ctx->svpwm.sin_cos[1], &id_raw_q, &iq_raw_q);

    // 低通滤波
    const q16_16_t lpf_beta_q = FLOAT_TO_Q16_16(0.33f);
    ctx->lpf_id_q = foc_lpf_update(ctx->lpf_id_q, id_raw_q, lpf_beta_q);
    ctx->lpf_iq_q = foc_lpf_update(ctx->lpf_iq_q, iq_raw_q, lpf_beta_q);

    // PI电流控制器
    ctx->pi_id.target = ctx->target_id_q;
    ctx->pi_id.real = ctx->lpf_id_q;
    ctx->pi_iq.target = ctx->target_iq_q;
    ctx->pi_iq.real = ctx->lpf_iq_q;

    foc_pi_calc(&ctx->pi_iq);
    foc_pi_calc(&ctx->pi_id);

    // 设置SVPWM输入
    ctx->svpwm.vd = ctx->pi_id.out;
    ctx->svpwm.vq = ctx->pi_iq.out;

    // 逆Park变换
    foc_ipark_transform(ctx->svpwm.vd, ctx->svpwm.vq, ctx->svpwm.sin_cos[0], ctx->svpwm.sin_cos[1],
        &ctx->svpwm.v_alpha, &ctx->svpwm.v_beta);

    // SVPWM计算
    foc_svpwm_calculate(&ctx->svpwm);

    // 更新PWM输出
    foc_port_pwm_update(&ctx->port, ctx->svpwm.ta, ctx->svpwm.tb, ctx->svpwm.tc, ctx->svpwm.td);
}

/**
 * @brief FOC锁相环（PLL）运行函数
 */
void foc_core_pll_run(q16_16_t phase_q, q16_16_t dt_q, q16_16_t* phase_var_q, q16_16_t* speed_var_q, q16_16_t kp_q,
    q16_16_t ki_q, q16_16_t speed_limit_q)
{
    // 计算相位误差并归一化到 [-π, π)
    q16_16_t delta_theta_q = q16_16_sub(phase_q, *phase_var_q);
    delta_theta_q = foc_normalize_angle(delta_theta_q);

    // 使用64位中间结果避免溢出
    // 比例项：kp * delta_theta
    int64_t kp_term_64 = ((int64_t)kp_q * (int64_t)delta_theta_q) >> 16;
    q16_16_t kp_term = (q16_16_t)kp_term_64;

    // 速度项：speed + kp_term
    q16_16_t speed_term = q16_16_add(*speed_var_q, kp_term);

    // 相位增量：speed_term * dt
    int64_t phase_inc_64 = ((int64_t)speed_term * (int64_t)dt_q) >> 16;
    q16_16_t phase_inc = (q16_16_t)phase_inc_64;

    // 更新相位变量并归一化到 [-π, π)
    *phase_var_q = q16_16_add(*phase_var_q, phase_inc);
    *phase_var_q = foc_normalize_angle(*phase_var_q);

    // 积分项：ki * delta_theta * dt
    int64_t ki_delta_64 = ((int64_t)ki_q * (int64_t)delta_theta_q) >> 16;
    int64_t speed_inc_64 = (ki_delta_64 * (int64_t)dt_q) >> 16;
    q16_16_t speed_inc = (q16_16_t)speed_inc_64;

    // 更新速度变量，添加可配置的饱和限制
    *speed_var_q = q16_16_add(*speed_var_q, speed_inc);
    *speed_var_q = q16_16_clip(*speed_var_q, q16_16_sub(0, speed_limit_q), speed_limit_q);
}
