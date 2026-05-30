/**
 * @file    foc_core.h
 * @brief   FOC核心算法模块 - 电流环与PLL
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V1.0.0
 *
 * @description
 * 纯算法层，包含FOC电流环计算的完整流程和锁相环(PLL)算法。
 * 通过端口抽象层(foc_port)与硬件交互，不依赖任何平台特定代码。
 */

#ifndef FOC_CORE_H
#define FOC_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "foc_hal.h"
#include "foc_math.h"
#include "foc_svpwm.h"

/* ==================== 前置声明 ==================== */

// foc_context_t 完整定义在 foc.h 中
typedef struct foc_context foc_context_t;

/* ==================== 核心算法 API ==================== */

/**
 * @brief FOC电流环计算（核心算法）
 *
 * 执行FOC电流环计算：sin/cos → ADC读取 → Clarke/Park变换 →
 * PI控制 → 逆Park → SVPWM → PWM输出。
 * 所有硬件访问通过 ctx->port 的回调完成。
 *
 * @param ctx FOC上下文指针
 * @param electrical_angle_q 电气角度（Q16.16格式，弧度）
 */
void foc_core_current_loop(foc_context_t* ctx, q16_16_t electrical_angle_q);

/**
 * @brief FOC锁相环（PLL）运行函数
 *
 * 使用PI型PLL跟踪输入相位，同时估计速度和相位。
 *
 * @param phase_q 输入相位（Q16.16格式，弧度）
 * @param dt_q 采样时间间隔（Q16.16格式，秒）
 * @param phase_var_q 输出相位变量指针（Q16.16格式）
 * @param speed_var_q 输出速度变量指针（Q16.16格式）
 * @param kp_q 比例增益（Q16.16格式）
 * @param ki_q 积分增益（Q16.16格式）
 * @param speed_limit_q 速度输出饱和限制（Q16.16格式，绝对值）
 */
void foc_core_pll_run(q16_16_t phase_q, q16_16_t dt_q, q16_16_t* phase_var_q, q16_16_t* speed_var_q, q16_16_t kp_q,
    q16_16_t ki_q, q16_16_t speed_limit_q);

#endif /* FOC_CORE_H */
