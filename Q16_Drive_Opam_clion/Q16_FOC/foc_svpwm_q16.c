/**
 * @brief:    SVPWM 空间矢量PWM（Q16.16定点版本）
 * @FilePath:  focsvpwm.c
 * @author:  fubingyan qq: 3245784484
 * @Date: 2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#include "foc_svpwm_q16.h"  // 包含SVPWM头文件
#include "foc_config_q16.h" // 包含FOC配置头文件

/**
 * @brief 三次谐波注入法SVPWM实现
 * 注入三次谐波以扩展线性调制范围，适用于需要最大输出电压的应用。
 * 原理：
 * - 在基本波基础上注入最优三次谐波
 * - 使PWM波形从正弦变为"马鞍波"
 * - 增加基波幅值约15%
 * - 计算量更小，不需要扇区判断
 * @param this SVPWM结构体指针
 */
static void svpwm_calculate_harmonic(svpwm_t *this)
{
    // 计算归一化电压（避免溢出，使用 64 位中间变量）
    q16_16_t v_bus_inv = q16_16_div(Q16_16_ONE, this->v_bus); // V_bus 倒数

    // 逆 Clark 变换：从 Vα/Vβ 计算归一化 Va、Vb、Vc
    q16_16_t va = q16_16_mul(this->v_alpha, v_bus_inv);

    // Vb = (-1/2)*Vα + (√3/2)*Vβ
    q16_16_t vb_temp = q16_16_add(q16_16_mul(-Q16_16_HALF, this->v_alpha), q16_16_mul(Q16_16_SQRT3_2, this->v_beta));
    q16_16_t vb = q16_16_mul(vb_temp, v_bus_inv);

    // Vc = (-1/2)*Vα + (-√3/2)*Vβ
    q16_16_t vc_temp = q16_16_add(q16_16_mul(-Q16_16_HALF, this->v_alpha), q16_16_mul(-Q16_16_SQRT3_2, this->v_beta));
    q16_16_t vc = q16_16_mul(vc_temp, v_bus_inv);

    // 找到最小值和最大值
    q16_16_t min_v = q16_16_min(q16_16_min(va, vb), vc);
    q16_16_t max_v = q16_16_max(q16_16_max(va, vb), vc);

    // 计算三次谐波注入偏移量（标准公式：v_offset = -(V_max + V_min)/2）
    q16_16_t v_offset = q16_16_mul(q16_16_add(min_v, max_v), -Q16_16_HALF);

    // 应用三次谐波注入并调整到 [0, 1] 范围
    // 标准公式：V' = V + v_offset，然后加上0.5中心化到[0,1]
    this->ta = q16_16_add(q16_16_add(va, v_offset), Q16_16_HALF);
    this->tb = q16_16_add(q16_16_add(vb, v_offset), Q16_16_HALF);
    this->tc = q16_16_add(q16_16_add(vc, v_offset), Q16_16_HALF);
}

/**
 * @brief SVPWM计算（支持两种方法切换）
 *
 * 执行流程：
 * 1. 进行逆Park变换：d/q → alpha/beta
 * 2. 选择计算方法：扇区法或三次谐波注入法
 * 3. 输出三相占空比：ta, tb, tc
 *
 * @param this SVPWM结构体指针
 *        - 输入：vd, vq, v_bus, sin_cos
 *        - 输出：ta, tb, tc, td
 */
volatile float ratio = 0;
q16_16_t v_alpha_sq, v_beta_sq, v_mag_sq, max_v_sq, inv_mag, max_v_limited;
void svpwm_calculate(svpwm_t *this)
{
    // 输入合理性检查
    if (this->v_bus <= 0 || this->v_bus > FLOAT_TO_Q16_16(V_BUS_MAX)) // 母线电压范围检查
    {
        this->ta = this->tb = this->tc = this->td = 0; // 占空比清零
        return;
    }

    // 计算电压矢量幅度的平方
    v_alpha_sq = q16_16_mul(this->v_alpha, this->v_alpha); // α轴电压平方
    v_beta_sq = q16_16_mul(this->v_beta, this->v_beta);    // β轴电压平方
    v_mag_sq = q16_16_add(v_alpha_sq, v_beta_sq);          // 电压矢量幅度的平方

    // 最大允许电压（内切圆，保留余度）
    // max_v = Vbus / √3 · duty_ratio
    q16_16_t max_v_radius = q16_16_mul(this->v_bus, Q16_16_INV_SQRT3); // 最大电压半径
    max_v_limited = q16_16_mul(max_v_radius, this->max_duty_ratio);    // 限制后的最大电压
    max_v_sq = q16_16_mul(max_v_limited, max_v_limited);               // 限制后的最大电压平方

    // 过调制检查和幅度缩放
    if (v_mag_sq > max_v_sq) // 如果电压矢量幅度平方超过最大允许值
    {
        inv_mag = q16_16_inv_sqrt(v_mag_sq);

        // 2. 计算缩放因子
        q16_16_t scale = q16_16_mul(max_v_limited, inv_mag);
        ratio = Q16_16_TO_FLOAT(scale);
        // 3. 应用缩放
        this->v_alpha = q16_16_mul(this->v_alpha, scale);
        this->v_beta = q16_16_mul(this->v_beta, scale);
    }

    /* 使用三次谐波注入法SVPWM */
    svpwm_calculate_harmonic(this);

    /* 限幅处理，确保占空比在[0, 1]之间 */
    this->ta = q16_16_clip(this->ta, 0, Q16_16_ONE);
    this->tb = q16_16_clip(this->tb, 0, Q16_16_ONE);
    this->tc = q16_16_clip(this->tc, 0, Q16_16_ONE);

    /* 计算最大占空比（用于ADC同步采样） */
    this->td = q16_16_max(this->ta, q16_16_max(this->tb, this->tc));
}
