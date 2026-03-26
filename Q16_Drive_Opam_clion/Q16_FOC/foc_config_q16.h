/**
 * @brief:    FOC配置文件（Q16.16定点版本）
 * @FilePath: focconfig.h
 * @author: fubingyan qq:3245784484
 * @Date: 2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
#ifndef FOC_CONFIG_Q16_H
#define FOC_CONFIG_Q16_H

#include "q16_16_math.h"

#define MOTOR_POLES 11                              ///< 电机极对数
#define MOVE_STEP_NUM ((uint32_t)(MOTOR_POLES * 4)) ///< 移动步数
#define OPAMPS 8.333f                               ///< 运放增益
#define RESISTOR 0.01f                              ///< 采样电阻值
#define CURRENT_SAMPLE_FACTOR 0.0096683555f         // (3.30f / (4096.0f * OPAMPS * RESISTOR)) ///< 电流采样转换因子
#define CURRENT_SAMPLE_FACTOR_Q FLOAT_TO_Q16_16(CURRENT_SAMPLE_FACTOR) ///< 电流采样转换因子（Q16.16格式）

#define MOTOR_PHASE_RESISTANCE 6.2f   ///< 电机相电阻
#define MOTOR_PHASE_INDUCTANCE 0.001f ///< 电机相电感
#define MOTOR_KE (8.2f)               ///< 电机反电动势常数
#define V_BUS_MAX (48)                ///< 母线最大电压
#define V_BUS (24.0f)                 ///< 母线电压
#define MOTOR_FLUX (9.905f / 1000.0f) ///< 电机磁链

#define PWM_PERIOD (2500 - 1)                                 ///< PWM周期计数器值
#define FOC_PWM_PERIOD (0.00005952381f)                       ///< FOC PWM周期（秒）
#define STATE_PERIOD (0.001f)                                 ///< 状态机周期（秒）
#define FOC_PWM_PERIOD_Q FLOAT_TO_Q16_16(FOC_PWM_PERIOD) ///< FOC PWM周期（Q16.16格式）
#define STATE_PERIOD_Q FLOAT_TO_Q16_16(STATE_PERIOD)          ///< 状态机周期（Q16.16格式）

#define M_2PI (6.2831853f)       ///< 2π
#define SQRT3 1.732050807568877f ///< 根号3
#define SQRT3_2 (SQRT3 * 0.50f)  ///< 根号3/2
#define INV_SQRT3 0.577350269f   ///< 1/根号3

#define PLL_ELE_VEL (1500)                                     ///< PLL电气角速度
#define PLL_ELE_BW (PLL_ELE_VEL * MOTOR_POLES * M_2PI / 60.0f) ///< PLL电气带宽
#define PLL_ELE_KP 6000                                        ///< PLL比例增益
#define PLL_ELE_KI 12000                                       ///< PLL积分增益
#define PLL_ELE_KP_Q INT_TO_Q16_16(PLL_ELE_KP)                 ///< PLL比例增益（Q16.16格式）
#define PLL_ELE_KI_Q INT_TO_Q16_16(PLL_ELE_KI)                 ///< PLL积分增益（Q16.16格式）

#define OB_SMC_GAIN 0.35f                         ///< 滑膜观测器增益
#define OB_SMC_GAIN_Q FLOAT_TO_Q16_16(0.35f)      ///< 滑膜观测器增益（Q16.16格式）
#define OB_SMC_LPF_K 0.25f                        ///< 滑膜观测器低通滤波系数
#define OB_SMC_LPF_K_Q FLOAT_TO_Q16_16(0.25f)     ///< 滑膜观测器低通滤波系数（Q16.16格式）
#define OB_SMC_PLL_KP (4000)                      ///< 滑膜观测器PLL比例增益
#define OB_SMC_PLL_KI (30000)                     ///< 滑膜观测器PLL积分增益
#define OB_SMC_PLL_KP_Q FLOAT_TO_Q16_16(4000.0f)  ///< 滑膜观测器PLL比例增益（Q16.16格式）
#define OB_SMC_PLL_KI_Q FLOAT_TO_Q16_16(30000.0f) ///< 滑膜观测器PLL积分增益（Q16.16格式）

#define ALIGN_THETA (0.25f * M_2PI)                  ///< 对齐角度
#define ALIGN_THETA_Q FLOAT_TO_Q16_16(0.25f * M_2PI) ///< 对齐角度（Q16.16格式）
#define ALIGN_CURRENT (0.50f)                        ///< 对齐电流
#define ALIGN_CURRENT_Q FLOAT_TO_Q16_16(0.50f)       ///< 对齐电流（Q16.16格式）
#define ALIGN_TIME (500)                             ///< 对齐时间

#define IF_STARTUP_IQ_MIN (0.1f)                                                  ///< 启动IQ最小电流
#define IF_STARTUP_IQ ALIGN_CURRENT                                               ///< 启动IQ电流
#define IF_STARTUP_IQ_Q FLOAT_TO_Q16_16(ALIGN_CURRENT)                            ///< 启动IQ电流（Q16.16格式）
#define IF_STARTUP_IQ_KP (0.00001f)                                               ///< 启动IQ比例增益
#define IF_STARTUP_IQ_KP_Q FLOAT_TO_Q16_16(0.00001f)                              ///< 启动IQ比例增益（Q16.16格式）
#define IF_STARTUP_OMEGA ((25.0f / 60.0f) * MOTOR_POLES * M_2PI * STATE_PERIOD)   ///< 启动角速度
#define IF_STARTUP_OMEGA_ACC (10.0f / 60.0f / MOTOR_POLES * M_2PI * STATE_PERIOD) ///< 启动角速度加速度
#define IF_STARTUP_THETA_DIFF (0.15f * M_2PI)                                     ///< 启动角度差
#define IF_STARTUP_MAX_TIME (3.0f)                                                ///< 启动最大时间
#define IF_STARTUP_OMEGA_Q FLOAT_TO_Q16_16(IF_STARTUP_OMEGA)                      ///< 启动角速度（Q16.16格式）
#define IF_STARTUP_OMEGA_ACC_Q FLOAT_TO_Q16_16(IF_STARTUP_OMEGA_ACC)              ///< 启动角速度加速度（Q16.16格式）
#define IF_STARTUP_THETA_DIFF_Q FLOAT_TO_Q16_16(0.15f * M_2PI)                    ///< 启动角度差（Q16.16格式）

#define CURRENT_LOOP_WIDTH (M_2PI * 50.0f)                       ///< 电流环带宽
#define CURRENT_KP (CURRENT_LOOP_WIDTH * MOTOR_PHASE_INDUCTANCE) ///< 电流环比例增益
#define CURRENT_KI (CURRENT_LOOP_WIDTH * MOTOR_PHASE_RESISTANCE) ///< 电流环积分增益
#define CURRENT_KP_Q FLOAT_TO_Q16_16(CURRENT_KP)                 ///< 电流环比例增益（Q16.16格式）
#define CURRENT_KI_Q FLOAT_TO_Q16_16(CURRENT_KI)                 ///< 电流环积分增益（Q16.16格式）

#define CURRENT_IQ_KP (CURRENT_KP)                                   ///< IQ电流环比例增益
#define CURRENT_IQ_KI (CURRENT_KI)                                   ///< IQ电流环积分增益
#define CURRENT_IQ_OUT_MAX (INV_SQRT3 * V_BUS)                       ///< IQ电流环最大输出
#define CURRENT_IQ_OUT_MIN (-CURRENT_IQ_OUT_MAX)                     ///< IQ电流环最小输出
#define CURRENT_IQ_INTEG_SAT (CURRENT_IQ_OUT_MAX)                    ///< IQ电流环积分饱和限制
#define CURRENT_IQ_KP_Q FLOAT_TO_Q16_16(CURRENT_IQ_KP)               ///< IQ电流环比例增益（Q16.16格式）
#define CURRENT_IQ_KI_Q FLOAT_TO_Q16_16(CURRENT_IQ_KI)               ///< IQ电流环积分增益（Q16.16格式）
#define CURRENT_IQ_OUT_MAX_Q FLOAT_TO_Q16_16(CURRENT_IQ_OUT_MAX)     ///< IQ电流环最大输出（Q16.16格式）
#define CURRENT_IQ_OUT_MIN_Q FLOAT_TO_Q16_16(CURRENT_IQ_OUT_MIN)     ///< IQ电流环最小输出（Q16.16格式）
#define CURRENT_IQ_INTEG_SAT_Q FLOAT_TO_Q16_16(CURRENT_IQ_INTEG_SAT) ///< IQ电流环积分饱和限制（Q16.16格式）

#define CURRENT_ID_KP (CURRENT_KP)                                   ///< ID电流环比例增益
#define CURRENT_ID_KI (CURRENT_KI)                                   ///< ID电流环积分增益
#define CURRENT_ID_OUT_MAX CURRENT_IQ_OUT_MAX                        ///< ID电流环最大输出
#define CURRENT_ID_OUT_MIN (-CURRENT_ID_OUT_MAX)                     ///< ID电流环最小输出
#define CURRENT_ID_INTEG_SAT CURRENT_IQ_INTEG_SAT                    ///< ID电流环积分饱和限制
#define CURRENT_ID_KP_Q FLOAT_TO_Q16_16(CURRENT_ID_KP)               ///< ID电流环比例增益（Q16.16格式）
#define CURRENT_ID_KI_Q FLOAT_TO_Q16_16(CURRENT_ID_KI)               ///< ID电流环积分增益（Q16.16格式）
#define CURRENT_ID_OUT_MAX_Q FLOAT_TO_Q16_16(CURRENT_ID_OUT_MAX)     ///< ID电流环最大输出（Q16.16格式）
#define CURRENT_ID_OUT_MIN_Q FLOAT_TO_Q16_16(CURRENT_ID_OUT_MIN)     ///< ID电流环最小输出（Q16.16格式）
#define CURRENT_ID_INTEG_SAT_Q FLOAT_TO_Q16_16(CURRENT_ID_INTEG_SAT) ///< ID电流环积分饱和限制（Q16.16格式）

#endif
