/**
 * @brief:    FOC配置文件（Q16.16定点版本）
 *            集中管理所有电机参数、控制参数和系统常量，
 *            是 FOC 控制系统的单一参数配置入口。
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
/**
 * @def MOTOR_POLES
 * @brief 电机极对数
 * @note 对于 22 极电机（11 对极），MOTOR_POLES = 11。
 *       极对数决定了电气角度与机械角度的倍数关系：θ_elec = MOTOR_POLES * θ_mech
 */
#define MOTOR_POLES 11

/**
 * @def MOVE_STEP_NUM
 * @brief 编码器校准时的移动步数
 * @note 每步对应一个极对角度（2π / MOTOR_POLES），总步数为 MOTOR_POLES * 4
 *       用于在校准过程中将编码器 360° 电气周期等分为多个采样点
 */
#define MOVE_STEP_NUM ((uint32_t)(MOTOR_POLES * 4))

/**
 * @def CURRENT_SAMPLE_FACTOR
 * @brief 电流采样转换因子
 * @note 将 ADC 原始采样值转换为实际电流值（安培）的比例系数。
 *       计算公式取决于硬件：采样电阻 × 放大器增益 × ADC 参考电压 / ADC 分辨率。
 *       此处值为 0.0096683555，即 ADC 读数每增加 1 对应约 9.67mA
 */
#define CURRENT_SAMPLE_FACTOR 0.0096683555f

#define MOTOR_PHASE_RESISTANCE 6.2f ///< 电机相电阻 @note 单位为 Ω（欧姆）
#define MOTOR_PHASE_INDUCTANCE 0.001f ///< 电机相电感 @note 单位为 H（亨利）
#define V_BUS_MAX (48) ///< 母线最大电压 @note 单位为 V（伏特），用于硬件过压保护判断
#define V_BUS (24.0f) ///< 母线电压 @note 单位为 V（伏特），实际供电电压，用于 SVPWM 占空比归一化

/* ============= 系统参数 ============= */
/**
 * @def PWM_PERIOD
 * @brief PWM 定时器周期计数值
 * @note 等于定时器 ARR 寄存器值。PWM 频率 = TIM_CLOCK / (PWM_PERIOD + 1) / (prescaler + 1)。
 *       此处为 2500 - 1 = 2499，配合预分频器得到约 16.8kHz 的 PWM 开关频率
 */
#define PWM_PERIOD (2500 - 1)

/**
 * @def FOC_PWM_PERIOD
 * @brief FOC 控制周期（秒）
 * @note 等于 PWM 周期的时间长度。FOC 电流环在 PWM 定时器更新事件中断中执行，
 *       因此控制频率 = 1 / FOC_PWM_PERIOD ≈ 16.8kHz。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->pwm_period_q，避免中断中重复转换
 */
#define FOC_PWM_PERIOD (0.00005952381f)

/**
 * @def STATE_PERIOD
 * @brief 状态机运行周期（秒）
 * @note 状态机（IF 启动、编码器校准等）不需要在 PWM 中断中每个周期都执行，
 *       通常以 1ms（1000Hz）的频率在主循环中被调用。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->state_period_q
 */
#define STATE_PERIOD (0.001f)

/* ============= 数学常数 ============= */
#define M_2PI (6.2831853f) ///< 2π = 6.283185307179586
#define SQRT3 1.732050807568877f ///< √3 ≈ 1.732050807568877，用于 Clarke 变换中 Iβ 计算
#define INV_SQRT3 0.577350269f ///< 1/√3 ≈ 0.577350269，等幅值 Clarke 变换的缩放系数

/* ============= PLL 参数 ============= */
/**
 * @def PLL_ELE_KP
 * @brief PLL（锁相环）比例增益
 * @note 用于 foc_core_pll_run() 中的相位误差跟踪。
 *       Kp 越大，锁相环对相位误差的响应越快，但可能引入超调和噪声
 */
#define PLL_ELE_KP 6000

/**
 * @def PLL_ELE_KI
 * @brief PLL（锁相环）积分增益
 * @note Ki 决定了锁相环对稳态误差的消除能力。
 *       Ki 越大，稳态跟踪误差越小，但可能引入低频振荡
 */
#define PLL_ELE_KI 12000

/**
 * @def PLL_SPEED_LIMIT
 * @brief PLL 速度输出饱和限制（rad/s，绝对值）
 * @note 限制锁相环估计的速度最大值，防止在突变或噪声情况下输出异常高速度
 */
#define PLL_SPEED_LIMIT 1000.0f

/* ============= 对齐参数 ============= */
/**
 * @brief 对齐初始电气角度（弧度）
 * @note 用于转子预定位（对齐）阶段。0.25 * 2π = π/2，
 *       即给电机施加一个 90° 电气角度的定向磁场，将转子拉到已知位置。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->align_theta_q
 */
#define ALIGN_THETA (0.25f * M_2PI)

#define ALIGN_CURRENT (0.50f) ///< 对齐电流 @note 单位为 A（安培）

/* ============= IF启动参数 ============= */
/**
 * @brief IF（I-F 启动）Q 轴电流目标值（安培）
 * @note 在 IF 启动模式下，向电机施加恒定的 Iq 电流产生转矩，
 *       使转子跟随旋转磁场加速。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->if_startup_iq_q
 */
#define IF_STARTUP_IQ (ALIGN_CURRENT)

/**
 * @def IF_STARTUP_OMEGA
 * @brief IF 启动每步角速度增量（弧度）
 * @note 公式：(25 rpm / 60) * MOTOR_POLES * 2π * STATE_PERIOD
 *       物理含义：25 rpm 机械转速换算为每个状态机周期（1ms）的电气角度增量。
 *       用于 IF 启动阶段开环扫频时的角度步进。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->if_startup_target_omega_q
 */
#define IF_STARTUP_OMEGA ((25.0f / 60.0f) * MOTOR_POLES * M_2PI * STATE_PERIOD)

/**
 * @def IF_STARTUP_OMEGA_ACC
 * @brief IF 启动角速度加速度（弧度）
 * @note 公式：(10 rpm / 60 / MOTOR_POLES) * 2π * STATE_PERIOD
 *       物理含义：每个状态机周期增加的机械转速为 10 rpm/min（每分钟增加 10 转），
 *       换算为每个周期（1ms）的电气角度增量。用于 IF 启动的扫频加速度。
 *       Q16.16 转换在 foc_init() 中预计算后存入 ctx->if_startup_omega_acc_q
 */
#define IF_STARTUP_OMEGA_ACC (10.0f / 60.0f / MOTOR_POLES * M_2PI * STATE_PERIOD)

/* ============= 电流环参数 ============= */
/**
 * @def CURRENT_LOOP_WIDTH
 * @brief 电流环带宽
 * @note 公式：2π * 50 Hz = 314.159 rad/s
 *       电流环带宽决定了电流环的响应速度，50Hz 适用于大多数电机。
 *       带宽越高，响应越快，但对噪声和采样延迟更敏感
 */
#define CURRENT_LOOP_WIDTH (M_2PI * 50.0f)

/**
 * @def CURRENT_KP
 * @brief 电流环比例增益（基于带宽和电感计算）
 * @note 公式：Kp = bandwidth * L (L = 相电感)
 *       极点配置法：Kp = ω_bw * L，将电流环的闭环极点配置在 -ω_bw 处
 */
#define CURRENT_KP (CURRENT_LOOP_WIDTH * MOTOR_PHASE_INDUCTANCE)

/**
 * @def CURRENT_KI
 * @brief 电流环积分增益（基于带宽和电阻计算）
 * @note 公式：Ki = bandwidth * R (R = 相电阻)
 *       极点配置法：Ki = ω_bw * R，配合 Kp 将零极点对消
 */
#define CURRENT_KI (CURRENT_LOOP_WIDTH * MOTOR_PHASE_RESISTANCE)

/**
 * @def CURRENT_IQ_OUT_MAX
 * @brief IQ 电流环最大输出电压
 * @note 公式：V_bus / √3
 *       在 SVPWM 线性调制区（六边形内切圆），最大相电压峰值为 V_bus / √3。
 *       即电流环 PI 输出不能超过此值，否则进入过调制区
 */
#define CURRENT_IQ_OUT_MAX (INV_SQRT3 * V_BUS)

#define CURRENT_IQ_KP (CURRENT_KP) ///< IQ 电流环比例增益，与 D 轴共用相同值
#define CURRENT_IQ_KI (CURRENT_KI) ///< IQ 电流环积分增益，与 D 轴共用相同值
#define CURRENT_IQ_OUT_MIN (-CURRENT_IQ_OUT_MAX) ///< IQ 电流环最小输出电压（对称限幅）
#define CURRENT_IQ_INTEG_SAT (CURRENT_IQ_OUT_MAX) ///< IQ 电流环积分饱和限制，防止积分深度饱和

#define CURRENT_ID_KP (CURRENT_KP) ///< ID 电流环比例增益，与 Q 轴共用相同值
#define CURRENT_ID_KI (CURRENT_KI) ///< ID 电流环积分增益，与 Q 轴共用相同值
#define CURRENT_ID_OUT_MAX CURRENT_IQ_OUT_MAX ///< ID 电流环最大输出电压
#define CURRENT_ID_OUT_MIN (-CURRENT_ID_OUT_MAX) ///< ID 电流环最小输出电压
#define CURRENT_ID_INTEG_SAT CURRENT_IQ_INTEG_SAT ///< ID 电流环积分饱和限制

#endif /* FOC_CONFIG_H */
