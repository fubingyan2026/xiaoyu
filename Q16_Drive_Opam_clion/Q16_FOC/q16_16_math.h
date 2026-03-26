/**
 * @brief:    Q16.16 定点数学库（32位MCU专用）
 *            实现高精度定点运算，避免浮点运算在中断中的开销
 *            Q16.16格式：高16位为整数部分，低16位为小数部分
 *            表示范围：-32768.0 ~ +32767.99998
 *            精度：约0.000015（1/65536）
 * @FilePath: q16_16_math.h
 * @author:  fubingyan qq: 3245784484
 * @Date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef Q16_16_MATH_H
#define Q16_16_MATH_H

#include <stdint.h>
/* 统一四舍五入，使用 copysign 避免分支 */
#include <math.h>

/* ============= Q16.16 定点数类型定义 ============= */
/**
 * @brief Q16.16定点数类型
 * 使用32位有符号整数表示定点数
 * 格式：IIIIIIIIIIIIIIII.FFFFFFFFFFFFFFFF
 * 其中I为整数位，F为小数位
 */
typedef int32_t q16_16_t;

/* ============= 缩放因子定义 ============= */
#define Q16_16_FRAC_BITS 16 ///< 小数位数：16位

#define Q16_16_SCALE 65536                   ///< 缩放因子：65536
#define Q16_16_SCALE_INV 0.0000152587890625f ///< 逆缩放因子：1/65536

/* ============= 浮点与Q16.16互转 ============= */
/**
 * @brief 浮点数转Q16.16定点数（带四舍五入）
 * @param x 浮点数输入
 * @return Q16.16定点数
 */
#define FLOAT_TO_Q16_16(x) ((q16_16_t)((x) * 65536.0f + ((x) >= 0.0f ? 0.5f : -0.5f)))

/**
 * @brief Q16.16定点数转浮点数
 * @param x Q16.16定点数输入
 * @return 浮点数
 */
#define Q16_16_TO_FLOAT(x) ((float)(x) * Q16_16_SCALE_INV)

/**
 * @brief 整数转Q16.16定点数
 * @param x 整数输入
 * @return Q16.16定点数
 */
#define INT_TO_Q16_16(x) ((q16_16_t)(x) << Q16_16_FRAC_BITS)

/**
 * @brief Q16.16定点数转整数（截断）
 * @param x Q16.16定点数输入
 * @return 整数（截断小数部分）
 */
#define Q16_16_TO_INT(x) ((int32_t)(x) >> Q16_16_FRAC_BITS)

/**
 * @brief Q16.16定点数转整数（四舍五入）
 * @param x Q16.16定点数输入
 * @return 整数（四舍五入）
 */
#define Q16_16_TO_INT_ROUND(x) (((x) + (1 << (Q16_16_FRAC_BITS - 1))) >> Q16_16_FRAC_BITS)

/* ============= 常数定义 (Q16.16格式，预计算整数常量) ============= */
#define Q16_16_PI (INT32_C(205887))       ///< π ≈ 3.141592653590 (0x3243f)
#define Q16_16_2PI (INT32_C(411775))      ///< 2π ≈ 6.283185307180 (0x6487f)
#define Q16_16_PI_2 (INT32_C(102944))     ///< π/2 ≈ 1.570796326795 (0x19220)
#define Q16_16_PI_4 (INT32_C(51472))      ///< π/4 ≈ 0.785398163397 (0xc910)
#define Q16_16_SQRT2 (INT32_C(92682))     ///< √2 ≈ 1.414213562373 (0x16a0a)
#define Q16_16_SQRT3 (INT32_C(113512))    ///< √3 ≈ 1.732050807569 (0x1bb68)
#define Q16_16_SQRT3_2 (INT32_C(56756))   ///< √3/2 ≈ 0.866025403784 (0xddb4)
#define Q16_16_INV_SQRT3 (INT32_C(37837)) ///< 1/√3 ≈ 0.577350269190 (0x93cd)
#define Q16_16_ONE (INT32_C(0x00010000))  ///< 1.0
#define Q16_16_HALF (INT32_C(0x00008000)) ///< 0.5

/* ============= 饱和限幅宏定义 ============= */

/**
 * @brief 通用饱和宏（推荐最常用）
 * @param x 输入值
 * @param min_val 最小值
 * @param max_val 最大值
 */
#define Q16_16_SAT(x, min_val, max_val) q16_16_clip((x), (min_val), (max_val))

/**
 * @brief 对称饱和宏（常用于电压、误差等）
 * @param x 输入值
 * @param limit 限制值（绝对值）
 */
#define Q16_16_SAT_SYM(x, limit) q16_16_clip((x), -(limit), (limit))

/**
 * @brief 非负饱和宏（常用于占空比、增益等）
 * @param x 输入值
 * @param max_val 最大值
 */
#define Q16_16_SAT_POS(x, max_val) q16_16_clip((x), 0, (max_val))

/**
 * @brief 单位范围饱和（归一化值）
 */
#define Q16_16_SAT_UNIT(x) Q16_16_SAT_SYM((x), Q16_16_ONE)     ///< 限制在[-1, 1]
#define Q16_16_SAT_UNIT_POS(x) Q16_16_SAT_POS((x), Q16_16_ONE) ///< 限制在[0, 1]

/**
 * @brief Q16.16安全表示范围饱和（防止int32_t溢出导致符号错误）
 * Q16.16有效表示范围约为[-32768, +32767.999]，对应int32_t的±0x7FFF0000
 */
#define Q16_16_SAT_SAFE(x) q16_16_clip((x), INT32_C(-0x7FFF0000), INT32_C(0x7FFF0000))

/**
 * @brief PI控制器输出饱和宏
 * @param x 输入值
 * @param max_out 最大输出值
 */
#define Q16_16_SAT_PI_OUT(x, max_out) Q16_16_SAT_SYM((x), (max_out))

/* ============= Q16.16 基本运算 ============= */

/**
 * @brief Q16.16定点数乘法
 * 使用64位中间结果避免溢出，带四舍五入处理
 * @param a 乘数1
 * @param b 乘数2
 * @return 积
 */
static inline q16_16_t q16_16_mul(q16_16_t a, q16_16_t b)
{
    int64_t result = (int64_t)a * b;
    // 四舍五入：加上0.5后右移（相当于加上1<<15再右移16位）
    return (q16_16_t)((result + (1 << 15)) >> 16);
}

/**
 * @brief Q16.16定点数减法
 * @param a 被减数
 * @param b 减数
 * @return 差
 */
static inline q16_16_t q16_16_sub(q16_16_t a, q16_16_t b)
{
    return (q16_16_t)(a - b);
}

/**
 * @brief 计算Q16.16定点数的绝对值
 * @param x 输入值
 * @return 绝对值
 */
static inline q16_16_t q16_16_abs(q16_16_t x)
{
    return (x < 0) ? -x : x;
}

/**
 * @brief Q16.16定点数限幅函数
 * @param x 输入值
 * @param min_val 最小值
 * @param max_val 最大值
 * @return 限幅后的值
 */
static inline q16_16_t q16_16_clip(q16_16_t x, q16_16_t min_val, q16_16_t max_val)
{
    if (x > max_val)
        return max_val;
    if (x < min_val)
        return min_val;
    return x;
}

/**
 * @brief Q16.16定点数加法（带溢出检测）
 * @param a 加数1
 * @param b 加数2
 * @return 和（溢出时返回饱和值）
 */
static inline q16_16_t q16_16_add(q16_16_t a, q16_16_t b)
{
    q16_16_t res = a + b;
    // 检查是否发生正向或负向溢出
    if (((a ^ res) & (b ^ res)) < 0)
    {
        return (a < 0) ? INT32_MIN : INT32_MAX;
    }
    return res;
}


/**
 * @brief Q16.16定点数除法
 * 使用64位中间结果避免精度损失
 * @param a 被除数
 * @param b 除数
 * @return 商（除零时返回饱和值）
 */
static inline q16_16_t q16_16_div(q16_16_t a, q16_16_t b)
{
    if (b == 0)
        return (a >= 0) ? INT32_MAX : INT32_MIN;

    // 使用64位中间结果进行除法，确保精度
    int64_t result = ((int64_t)a << 16) / b;
    return (q16_16_t)result;
}

/**
 * @brief 返回两个Q16.16定点数中的最大值
 * @param a 值1
 * @param b 值2
 * @return 最大值
 */
static inline q16_16_t q16_16_max(q16_16_t a, q16_16_t b)
{
    return (a > b) ? a : b;
}

/**
 * @brief 返回两个Q16.16定点数中的最小值
 * @param a 值1
 * @param b 值2
 * @return 最小值
 */
static inline q16_16_t q16_16_min(q16_16_t a, q16_16_t b)
{
    return (a < b) ? a : b;
}

/* ============= Q16.16 三角函数 ============= */

/**
 * @brief 同时计算Q16.16角度的正弦和余弦值
 * 使用查找表+线性插值实现，适合实时控制
 * @param angle_q 输入角度（弧度，Q16.16格式）
 * @param sin_out 输出正弦值指针
 * @param cos_out 输出余弦值指针
 */
void q16_16_sin_cos(q16_16_t angle_q, q16_16_t *sin_out, q16_16_t *cos_out);

/**
 * @brief Q16.16定点数平方根
 * 使用牛顿迭代法实现
 * @param x 输入值（非负）
 * @return 平方根
 */
q16_16_t q16_16_sqrt(q16_16_t x);

/**
 * @brief Q16.16格式的平方根倒数函数
 * @param x 输入值（Q16.16格式）
 * @return 平方根倒数（Q16.16格式）
 */
q16_16_t q16_16_inv_sqrt(q16_16_t x);

/**
 * @brief Q16.16定点数反正切函数（两参数版本）
 * 计算atan2(y, x)，返回值范围[-π, π]
 * @param y Y坐标
 * @param x X坐标
 * @return 角度（弧度）
 */
q16_16_t q16_16_atan2(q16_16_t y, q16_16_t x);

/* ============= FOC 特定运算 ============= */

/**
 * @brief Clarke变换（三相→两相静止坐标系）
 * 将三相电流Ia、Ib、Ic转换为两相静止坐标系下的Iα、Iβ
 * 变换公式：
 *   Iα = Ia
 *   Iβ = (2/√3)·Ib - (1/√3)·(Ia + Ic)
 * @param ia A相电流
 * @param ib B相电流
 * @param ic C相电流
 * @param alpha 输出α轴电流指针
 * @param beta 输出β轴电流指针
 */
void q16_16_clarke_transform(q16_16_t ia, q16_16_t ib, q16_16_t ic, q16_16_t *alpha, q16_16_t *beta);

/**
 * @brief Park变换（两相静止→两相旋转坐标系）
 * 将静止坐标系下的Iα、Iβ转换为旋转坐标系下的Id、Iq
 * 变换公式：
 *   Id = Iα·cos(θ) + Iβ·sin(θ)
 *   Iq = Iβ·cos(θ) - Iα·sin(θ)
 * @param alpha α轴分量
 * @param beta β轴分量
 * @param sin_theta 转子角度的正弦值
 * @param cos_theta 转子角度的余弦值
 * @param d 输出d轴分量指针
 * @param q 输出q轴分量指针
 */
void q16_16_park_transform(q16_16_t alpha, q16_16_t beta, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t *d,
                           q16_16_t *q);

/**
 * @brief 逆Park变换（两相旋转→两相静止坐标系）
 * 将旋转坐标系下的Vd、Vq转换为静止坐标系下的Vα、Vβ
 * 变换公式：
 *   Vα = Vd·cos(θ) - Vq·sin(θ)
 *   Vβ = Vd·sin(θ) + Vq·cos(θ)
 * @param d d轴分量
 * @param q q轴分量
 * @param sin_theta 转子角度的正弦值
 * @param cos_theta 转子角度的余弦值
 * @param alpha 输出α轴分量指针
 * @param beta 输出β轴分量指针
 */
void q16_16_ipark_transform(q16_16_t d, q16_16_t q, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t *alpha,
                            q16_16_t *beta);

/**
 * @brief 计算二维矢量的幅值
 * 计算√(α² + β²)
 * @param alpha α轴分量
 * @param beta β轴分量
 * @return 矢量幅值
 */
q16_16_t q16_16_vector_magnitude(q16_16_t alpha, q16_16_t beta);

/**
 * @brief 计算二维矢量幅值的平方
 * 计算α² + β²（避免开方运算）
 * @param alpha α轴分量
 * @param beta β轴分量
 * @return 矢量幅值的平方
 */
q16_16_t q16_16_vector_magnitude_sq(q16_16_t alpha, q16_16_t beta);

/* ============= Q16.16 PI 控制器 ============= */

/**
 * @brief Q16.16 PI控制器结构体
 * 实现比例积分控制算法，适用于电流环、速度环等控制
 */
typedef struct
{
    q16_16_t kp, ki;               ///< 比例增益和积分增益
    q16_16_t max_value, min_value; ///< 输出限幅值
    q16_16_t integ_sat;            ///< 积分饱和限制
    q16_16_t integral;             ///< 积分累积值
    q16_16_t target, real, err;    ///< 目标值、实际值、误差
    q16_16_t out;                  ///< 控制器输出
} q16_16_pi_t;

/**
 * @brief 初始化PI控制器
 * @param pi PI控制器指针
 * @param kp 比例增益
 * @param ki 积分增益
 * @param max_val 最大输出值
 * @param min_val 最小输出值
 * @param integ_sat 积分饱和限制
 */
void q16_16_pi_init(q16_16_pi_t *pi, q16_16_t kp, q16_16_t ki, q16_16_t max_val, q16_16_t min_val, q16_16_t integ_sat);

/**
 * @brief 执行PI控制器计算
 * 计算公式：out = Kp·err + Ki·∫err·dt
 * @param pi PI控制器指针
 * @param dt_q 采样周期（Q16.16格式）
 */
void q16_16_pi_calc(q16_16_pi_t *pi, q16_16_t dt_q);

void q16_16_pi_reset(q16_16_pi_t *pi);

/**
 * @brief 重置PI控制器状态
 * 清零积分项和误
 */

q16_16_t q16_16_lpf_update(q16_16_t old_val, q16_16_t new_val, q16_16_t lpf_k);

typedef struct
{
    q16_16_t *buffer;
    uint16_t length;
    uint16_t idx;
    q16_16_t sum;
} q16_16_ma_filter_t;

void q16_16_ma_filter_init(q16_16_ma_filter_t *filter, q16_16_t *buf, uint16_t len);
q16_16_t q16_16_ma_filter_update(q16_16_ma_filter_t *filter, q16_16_t new_val);

/* ============= Q16.16 角度处理 ============= */
q16_16_t q16_16_normalize_angle(q16_16_t angle_q);
q16_16_t q16_16_normalize_angle_0_2pi(q16_16_t angle_q);
q16_16_t q16_16_angle_diff(q16_16_t target, q16_16_t current);

#endif