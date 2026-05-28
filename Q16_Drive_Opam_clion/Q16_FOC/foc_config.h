/**
 * @brief:    FOC配置文件（Q16.16定点版本）
 * @FilePath: foc_config.h
 * @author: fubingyan qq:3245784484
 * @Date: 2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
#ifndef FOC_CONFIG_H
#define FOC_CONFIG_H

#include "foc_math.h"

/* ============= 电机参数 ============= */
#define MOTOR_POLES 11 ///< 电机极对数
#define MOVE_STEP_NUM ((uint32_t)(MOTOR_POLES * 4)) ///< 移动步数
#define CURRENT_SAMPLE_FACTOR 0.0096683555f ///< 电流采样转换因子
#define CURRENT_SAMPLE_FACTOR_Q FLOAT_TO_Q16_16(CURRENT_SAMPLE_FACTOR) ///< 电流采样转换因子（Q16.16格式）

#define MOTOR_PHASE_RESISTANCE 6.2f ///< 电机相电阻
#define MOTOR_PHASE_INDUCTANCE 0.001f ///< 电机相电感
#define V_BUS_MAX (48) ///< 母线最大电压
#define V_BUS (24.0f) ///< 母线电压

/* ============= 系统参数 ============= */
#define PWM_PERIOD (2500 - 1) ///< PWM周期计数器值
#define FOC_PWM_PERIOD (0.00005952381f) ///< FOC PWM周期（秒）
#define STATE_PERIOD (0.001f) ///< 状态机周期（秒）
#define FOC_PWM_PERIOD_Q FLOAT_TO_Q16_16(FOC_PWM_PERIOD) ///< FOC PWM周期（Q16.16格式）
#define STATE_PERIOD_Q FLOAT_TO_Q16_16(STATE_PERIOD) ///< 状态机周期（Q16.16格式）

/* ============= 数学常数 ============= */
#define M_2PI (6.2831853f) ///< 2π
#define SQRT3 1.732050807568877f ///< 根号3
#define INV_SQRT3 0.577350269f ///< 1/根号3

/* ============= PLL 参数 ============= */
#define PLL_ELE_KP 6000 ///< PLL比例增益
#define PLL_ELE_KI 12000 ///< PLL积分增益

/* ============= 对齐参数 ============= */
#define ALIGN_THETA_Q FLOAT_TO_Q16_16(0.25f * M_2PI) ///< 对齐角度（Q16.16格式）
#define ALIGN_CURRENT (0.50f) ///< 对齐电流
#define ALIGN_CURRENT_Q FLOAT_TO_Q16_16(0.50f) ///< 对齐电流（Q16.16格式）

/* ============= IF启动参数 ============= */
#define IF_STARTUP_IQ_Q FLOAT_TO_Q16_16(ALIGN_CURRENT) ///< 启动IQ电流（Q16.16格式）
#define IF_STARTUP_OMEGA ((25.0f / 60.0f) * MOTOR_POLES * M_2PI * STATE_PERIOD) ///< 启动角速度
#define IF_STARTUP_OMEGA_ACC (10.0f / 60.0f / MOTOR_POLES * M_2PI * STATE_PERIOD) ///< 启动角速度加速度
#define IF_STARTUP_OMEGA_Q FLOAT_TO_Q16_16(IF_STARTUP_OMEGA) ///< 启动角速度（Q16.16格式）
#define IF_STARTUP_OMEGA_ACC_Q FLOAT_TO_Q16_16(IF_STARTUP_OMEGA_ACC) ///< 启动角速度加速度（Q16.16格式）

/* ============= 电流环参数 ============= */
#define CURRENT_LOOP_WIDTH (M_2PI * 50.0f) ///< 电流环带宽
#define CURRENT_KP (CURRENT_LOOP_WIDTH * MOTOR_PHASE_INDUCTANCE) ///< 电流环比例增益
#define CURRENT_KI (CURRENT_LOOP_WIDTH * MOTOR_PHASE_RESISTANCE) ///< 电流环积分增益

#define CURRENT_IQ_KP (CURRENT_KP) ///< IQ电流环比例增益
#define CURRENT_IQ_KI (CURRENT_KI) ///< IQ电流环积分增益
#define CURRENT_IQ_OUT_MAX (INV_SQRT3 * V_BUS) ///< IQ电流环最大输出
#define CURRENT_IQ_OUT_MIN (-CURRENT_IQ_OUT_MAX) ///< IQ电流环最小输出
#define CURRENT_IQ_INTEG_SAT (CURRENT_IQ_OUT_MAX) ///< IQ电流环积分饱和限制

#define CURRENT_ID_KP (CURRENT_KP) ///< ID电流环比例增益
#define CURRENT_ID_KI (CURRENT_KI) ///< ID电流环积分增益
#define CURRENT_ID_OUT_MAX CURRENT_IQ_OUT_MAX ///< ID电流环最大输出
#define CURRENT_ID_OUT_MIN (-CURRENT_ID_OUT_MAX) ///< ID电流环最小输出
#define CURRENT_ID_INTEG_SAT CURRENT_IQ_INTEG_SAT ///< ID电流环积分饱和限制

/* ============= FOC 错误码枚举 ============= */

/**
 * @brief FOC模块错误码枚举
 */
typedef enum {
  FOC_OK = 0,                       /**< 操作成功 */
  FOC_ERROR_NULL_PTR,               /**< 空指针错误 */
  FOC_ERROR_UNINITIALIZED,          /**< 未初始化 */
  FOC_ERROR_PORT_INIT_FAILED,       /**< 端口初始化失败 */
  FOC_ERROR_FSM_INIT_FAILED,        /**< 状态机初始化失败 */
  FOC_ERROR_GENERIC,                /**< 通用错误 */
} foc_error_t;

#endif /* FOC_CONFIG_H */
