/**
 * @brief:  线性霍尔PLL
 * @FilePath: line_hall_pll.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-10-20 15:12:47
 * @LastEditTime: 2025-10-20 16:37:38
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "line_hall_pll.h"
#include "public.h"
#include "foc_config_q16.h"

/* 用户可调参数 ---------------------------------------------------------- */
#define Ts FOC_PWM_PERIOD /* 100 µs */
#define KP_PLL 250.0f      /* 比例 = 2*pi*400 Hz */
#define KI_PLL 32000.0f   /* 积分 = (2*pi*400)^2 /4  */
#define KP_AMP 0.05f      /* 幅值环带宽 ~10 Hz */
#define KI_AMP 2.5f
#define KP_OFF 0.02f /* 偏置环带宽 ~5 Hz */
#define KI_OFF 0.5f
#define LPF_CF 500
/* ---------------------------------------------------------------------- */

/**
 * @brief   : 霍尔 PLL 初始化
 * @param hall  : 霍尔 PLL 句柄
 */
void hall_pll_init(hall_pll_handle_t *hall)
{
    hall->pll.theta = 0.0f;
    hall->pll.omega = 0.0f;
    hall->pll.amp = 1000.0f;   /* 初始猜测，根据实际幅值给 */
    hall->pll.off_a = 2048.0f; /* ADC 中值 */
    hall->pll.off_b = 2048.0f;
    hall->pll.intg_pll = 0.0f;
    hall->pll.intg_amp = 0.0f;
    hall->pll.intg_off_a = 0.0f;
    hall->pll.intg_off_b = 0.0f;
    pt1FilterInit(&hall->lp_filter_ud_a, pt1FilterGain(LPF_CF, Ts)); /* ~100 Hz */
    pt1FilterInit(&hall->lp_filter_uq_a, pt1FilterGain(LPF_CF, Ts));
    pt1FilterInit(&hall->lp_filter_ud_b, pt1FilterGain(LPF_CF, Ts));
    pt1FilterInit(&hall->lp_filter_uq_b, pt1FilterGain(LPF_CF, Ts));
    pt1FilterInit(&hall->lp_filter_omega, pt1FilterGain(20, Ts));
}

/**
 * @brief   : 霍尔 PLL 更新
 * @param hall  : 霍尔 PLL 句柄
 * @param adc_a : 通道 A 原始 ADC 值 (-1,1)
 * @param adc_b : 通道 B 原始 ADC 值 (-1,1)
 */
void hall_pll_update(hall_pll_handle_t *hall, const float adc_a, const float adc_b)
{
    float ualpha = adc_a;
    float ubeta = adc_b;

    /* 1.双 dq 变换，得到误差 */
    const float sin_th = sin_approx(hall->pll.theta);
    const float cos_th = cos_approx(hall->pll.theta);

    // 2.做park变换
    // [ud] = T(θ̂)[uα] = [uα cosθ̂ + uβ sinθ̂]
    // [uq][uβ][ −uα sinθ̂ + uβ cosθ̂]
    const float ud_a = -2.0f * ualpha * sin_th;
    const float uq_a = 2.0f * ualpha * cos_th;
    const float ud_b = -2.0f * ubeta * cos_th; /* beta 领先 90° */
    const float uq_b = -2.0f * ubeta * sin_th;

    /* 3. 低通（简单一阶 IIR，截止 ~100 Hz） */
    pt1FilterApply(&hall->lp_filter_ud_a, ud_a);
    pt1FilterApply(&hall->lp_filter_uq_a, uq_a);
    pt1FilterApply(&hall->lp_filter_ud_b, ud_b);
    pt1FilterApply(&hall->lp_filter_uq_b, uq_b);

    /* 4. 相位误差 = 平均 q 轴 */
    const float err_pll = 0.5f * (hall->lp_filter_uq_a.state + hall->lp_filter_uq_b.state);

    /* 5. PI 计算频率 */
    hall->pll.intg_pll += err_pll * KI_PLL * Ts;
    hall->pll.omega = hall->pll.intg_pll + err_pll * KP_PLL;
    pt1FilterApply(&hall->lp_filter_omega, hall->pll.omega );

    /* 6. 积分得角度 */
    hall->pll.theta += hall->pll.omega * Ts;
    while (hall->pll.theta > PI)
        hall->pll.theta -= 2.0f * PI;
    while (hall->pll.theta < -M_PI)
        hall->pll.theta += 2.0f * PI;

    // /* 7. 幅值环：uq 理论 = 1，误差 = 1 - |uq| */
    // const float err_amp = 1.0f - err_pll;
    // hall->pll.intg_amp += err_amp * KI_AMP * Ts;
    // hall->pll.amp += hall->pll.intg_amp + err_amp * KP_AMP;
    // /* 限幅，防止启动瞬态炸掉 */
    // if (hall->pll.amp < 100.0f)
    //     hall->pll.amp = 100.0f;
    // if (hall->pll.amp > 3200.0f)
    //     hall->pll.amp = 3200.0f;

    // /* 8. 偏置环：ud 理论 = 0，误差 = -ud */
    // float err_off_a = -hall->lp_filter_ud_a.state;
    // float err_off_b = -hall->lp_filter_ud_b.state;
    // hall->pll.intg_off_a += err_off_a * KI_OFF * Ts;
    // hall->pll.intg_off_b += err_off_b * KI_OFF * Ts;
    // hall->pll.off_a += hall->pll.intg_off_a + err_off_a * KP_OFF;
    // hall->pll.off_b += hall->pll.intg_off_b + err_off_b * KP_OFF;
}

/* 随时可读结果，线程安全（单中断写） */
float hall_pll_get_angle_rad(const hall_pll_handle_t *hall)
{
    return hall->pll.theta;
}

/* 随时可读结果，线程安全（单中断写） */
float hall_pll_get_speed_rads(const hall_pll_handle_t *hall)
{
    return hall->pll.omega;
}