/**
 * @brief:   Q16.16 定点数学库实现
 * @FilePath:  q16_16_math.c
 * @author:  fubingyan qq:3245784484
 * @date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#include "q16_16_math.h"
/* ============= Q16.16 三角函数（LUT + 线性插值实现） ============= */

#define SIN_TABLE_SIZE 512

static const q16_16_t sin_table[SIN_TABLE_SIZE] = {
    0,      804,    1608,   2412,   3216,   4019,   4821,   5623,   6424,   7224,   8022,   8820,   9616,   10411,
    11204,  11996,  12785,  13573,  14359,  15143,  15924,  16703,  17479,  18253,  19024,  19792,  20557,  21320,
    22078,  22834,  23586,  24335,  25080,  25821,  26558,  27291,  28020,  28745,  29466,  30182,  30893,  31600,
    32303,  33000,  33692,  34380,  35062,  35738,  36410,  37076,  37736,  38391,  39040,  39683,  40320,  40951,
    41576,  42194,  42806,  43412,  44011,  44604,  45190,  45769,  46341,  46906,  47464,  48015,  48559,  49095,
    49624,  50146,  50660,  51166,  51665,  52156,  52639,  53114,  53581,  54040,  54491,  54934,  55368,  55794,
    56212,  56621,  57022,  57414,  57798,  58172,  58538,  58896,  59244,  59583,  59914,  60235,  60547,  60851,
    61145,  61429,  61705,  61971,  62228,  62476,  62714,  62943,  63162,  63372,  63572,  63763,  63944,  64115,
    64277,  64429,  64571,  64704,  64827,  64940,  65043,  65137,  65220,  65294,  65358,  65413,  65457,  65492,
    65516,  65531,  65536,  65531,  65516,  65492,  65457,  65413,  65358,  65294,  65220,  65137,  65043,  64940,
    64827,  64704,  64571,  64429,  64277,  64115,  63944,  63763,  63572,  63372,  63162,  62943,  62714,  62476,
    62228,  61971,  61705,  61429,  61145,  60851,  60547,  60235,  59914,  59583,  59244,  58896,  58538,  58172,
    57798,  57414,  57022,  56621,  56212,  55794,  55368,  54934,  54491,  54040,  53581,  53114,  52639,  52156,
    51665,  51166,  50660,  50146,  49624,  49095,  48559,  48015,  47464,  46906,  46341,  45769,  45190,  44604,
    44011,  43412,  42806,  42194,  41576,  40951,  40320,  39683,  39040,  38391,  37736,  37076,  36410,  35738,
    35062,  34380,  33692,  33000,  32303,  31600,  30893,  30182,  29466,  28745,  28020,  27291,  26558,  25821,
    25080,  24335,  23586,  22834,  22078,  21320,  20557,  19792,  19024,  18253,  17479,  16703,  15924,  15143,
    14359,  13573,  12785,  11996,  11204,  10411,  9616,   8820,   8022,   7224,   6424,   5623,   4821,   4019,
    3216,   2412,   1608,   804,    0,      -804,   -1608,  -2412,  -3216,  -4019,  -4821,  -5623,  -6424,  -7224,
    -8022,  -8820,  -9616,  -10411, -11204, -11996, -12785, -13573, -14359, -15143, -15924, -16703, -17479, -18253,
    -19024, -19792, -20557, -21320, -22078, -22834, -23586, -24335, -25080, -25821, -26558, -27291, -28020, -28745,
    -29466, -30182, -30893, -31600, -32303, -33000, -33692, -34380, -35062, -35738, -36410, -37076, -37736, -38391,
    -39040, -39683, -40320, -40951, -41576, -42194, -42806, -43412, -44011, -44604, -45190, -45769, -46341, -46906,
    -47464, -48015, -48559, -49095, -49624, -50146, -50660, -51166, -51665, -52156, -52639, -53114, -53581, -54040,
    -54491, -54934, -55368, -55794, -56212, -56621, -57022, -57414, -57798, -58172, -58538, -58896, -59244, -59583,
    -59914, -60235, -60547, -60851, -61145, -61429, -61705, -61971, -62228, -62476, -62714, -62943, -63162, -63372,
    -63572, -63763, -63944, -64115, -64277, -64429, -64571, -64704, -64827, -64940, -65043, -65137, -65220, -65294,
    -65358, -65413, -65457, -65492, -65516, -65531, -65536, -65531, -65516, -65492, -65457, -65413, -65358, -65294,
    -65220, -65137, -65043, -64940, -64827, -64704, -64571, -64429, -64277, -64115, -63944, -63763, -63572, -63372,
    -63162, -62943, -62714, -62476, -62228, -61971, -61705, -61429, -61145, -60851, -60547, -60235, -59914, -59583,
    -59244, -58896, -58538, -58172, -57798, -57414, -57022, -56621, -56212, -55794, -55368, -54934, -54491, -54040,
    -53581, -53114, -52639, -52156, -51665, -51166, -50660, -50146, -49624, -49095, -48559, -48015, -47464, -46906,
    -46341, -45769, -45190, -44604, -44011, -43412, -42806, -42194, -41576, -40951, -40320, -39683, -39040, -38391,
    -37736, -37076, -36410, -35738, -35062, -34380, -33692, -33000, -32303, -31600, -30893, -30182, -29466, -28745,
    -28020, -27291, -26558, -25821, -25080, -24335, -23586, -22834, -22078, -21320, -20557, -19792, -19024, -18253,
    -17479, -16703, -15924, -15143, -14359, -13573, -12785, -11996, -11204, -10411, -9616,  -8820,  -8022,  -7224,
    -6424,  -5623,  -4821,  -4019,  -3216,  -2412,  -1608,  -804};

#define SIN_LUT_INDEX_MULTIPLIER 5340351ULL // 预计算魔数：(512ULL << 32) / 411775 ≈ 5340351

void q16_16_sin_cos(q16_16_t angle_q, q16_16_t *sin_out, q16_16_t *cos_out)
{
    angle_q = q16_16_normalize_angle_0_2pi(angle_q);

    // scaled = angle_q * (512 / 2π)
    // 结果 64 位：[高32位=索引, 低32位=小数]
    uint64_t scaled = (uint64_t)angle_q * SIN_LUT_INDEX_MULTIPLIER;

    uint32_t index = (uint32_t)(scaled >> 32); // ✓ 整数部分
    uint32_t frac_full = (uint32_t)(scaled & 0xFFFFFFFFUL);
    uint16_t frac = (uint16_t)(frac_full >> 16); // ✓ 正确提取

    /* 安全取模（表大小为2的幂） */
    index &= (SIN_TABLE_SIZE - 1);
    uint32_t index_cos = (index + (SIN_TABLE_SIZE >> 2)) & (SIN_TABLE_SIZE - 1);

    q16_16_t sin1 = sin_table[index];
    q16_16_t sin2 = sin_table[(index + 1) & (SIN_TABLE_SIZE - 1)];
    q16_16_t cos1 = sin_table[index_cos];
    q16_16_t cos2 = sin_table[(index_cos + 1) & (SIN_TABLE_SIZE - 1)];

    int64_t diff_sin = (int64_t)sin2 - sin1;
    int64_t diff_cos = (int64_t)cos2 - cos1;

    q16_16_t interp_sin = (q16_16_t)((diff_sin * frac) >> 16);
    q16_16_t interp_cos = (q16_16_t)((diff_cos * frac) >> 16);

    *sin_out = sin1 + interp_sin;
    *cos_out = cos1 + interp_cos;
}

/**
 * @brief Q16.16 快速平方根
 */
q16_16_t q16_16_sqrt(q16_16_t x)
{
    if (x <= 0)
        return 0;
    if (x == Q16_16_ONE)
        return Q16_16_ONE;

    // sqrt(x) = x * (1/sqrt(x))
    q16_16_t inv = q16_16_inv_sqrt(x);
    return q16_16_mul(x, inv);
}

/**
 * @brief Q16.16格式的平方根倒数函数
 * @param x 输入值（Q16.16格式）
 * @return 平方根倒数（Q16.16格式）
 */
q16_16_t q16_16_inv_sqrt(q16_16_t x)
{
    if (x <= 0)
        return INT32_MAX;

    /* 使用浮点运算确保精度和可靠性 */

    // 将Q16.16转换为浮点数
    float x_float = Q16_16_TO_FLOAT(x);

    // 计算平方根倒数
    float inv_sqrt_float = 1.0f / sqrtf(x_float);

    // 将结果转换回Q16.16格式
    return FLOAT_TO_Q16_16(inv_sqrt_float);
}

/**
 * @brief Q16.16 反正切
 */
q16_16_t q16_16_atan2(q16_16_t y, q16_16_t x)
{
    if (x == 0 && y == 0)
        return 0;

    int quadrant = 0;
    q16_16_t abs_x = q16_16_abs(x);
    q16_16_t abs_y = q16_16_abs(y);

    if (x < 0)
        quadrant = (y < 0) ? 3 : 1;
    else
        quadrant = (y < 0) ? 2 : 0;

    q16_16_t ratio = q16_16_div(abs_y, abs_x);
    q16_16_t ratio_cubed = q16_16_mul(q16_16_mul(ratio, ratio), ratio);
    q16_16_t atan_val = q16_16_sub(ratio, q16_16_div(ratio_cubed, FLOAT_TO_Q16_16(3.0f)));

    switch (quadrant)
    {
    case 0:
        return atan_val;
    case 1:
        return q16_16_sub(Q16_16_PI, atan_val);
    case 2:
        return q16_16_sub(atan_val, Q16_16_PI);
    case 3:
        return q16_16_sub(0, atan_val);
    default:
        return 0;
    }
}

/* ============= FOC 特定运算实现 ============= */

/**
 * @brief Clarke变换（三相→两相静止坐标系）
 * @param ia A相电流
 * @param ib B相电流
 * @param ic C相电流（可由基尔霍夫定律计算）
 * @param[out] alpha 输出α轴电流
 * @param[out] beta 输出β轴电流
 */
void q16_16_clarke_transform(q16_16_t ia, q16_16_t ib, q16_16_t ic, q16_16_t *alpha, q16_16_t *beta)
{
    /* 等幅值 Clarke 变换
     * I_alpha = I_a
     * I_beta  = (I_b - I_c) / √3
     */
    *alpha = ia;

    /* 优化：使用移位代替乘法 */
    /* inv_sqrt3 * (ib - ic) >> 16 */
    q16_16_t beta_raw = q16_16_mul(Q16_16_INV_SQRT3, q16_16_sub(ib, ic));
    *beta = beta_raw;
}

/**
 * @brief Park变换（两相静止→两相旋转坐标系）
 * @param alpha α轴电流
 * @param beta β轴电流
 * @param sin_theta 电气角度正弦值
 * @param cos_theta 电气角度余弦值
 * @param[out] d 输出d轴电流
 * @param[out] q 输出q轴电流
 */
void q16_16_park_transform(q16_16_t alpha, q16_16_t beta, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t *d,
                           q16_16_t *q)
{
    /* 变换公式
     * I_d = I_alpha * cos(θ) + I_beta * sin(θ)
     * I_q = I_beta * cos(θ) - I_alpha * sin(θ)
     */
    *d = q16_16_add(q16_16_mul(cos_theta, alpha), q16_16_mul(sin_theta, beta));
    *q = q16_16_sub(q16_16_mul(cos_theta, beta), q16_16_mul(sin_theta, alpha));
}

/**
 * @brief 逆Park变换（两相旋转→两相静止坐标系）
 * @param d d轴电压
 * @param q q轴电压
 * @param sin_theta 电气角度正弦值
 * @param cos_theta 电气角度余弦值
 * @param[out] alpha 输出α轴电压
 * @param[out] beta 输出β轴电压
 */
void q16_16_ipark_transform(q16_16_t d, q16_16_t q, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t *alpha,
                            q16_16_t *beta)
{
    /* 逆变换公式
     * V_alpha = V_d * cos(θ) - V_q * sin(θ)
     * V_beta  = V_d * sin(θ) + V_q * cos(θ)
     */
    *alpha = q16_16_sub(q16_16_mul(cos_theta, d), q16_16_mul(sin_theta, q));
    *beta = q16_16_add(q16_16_mul(sin_theta, d), q16_16_mul(cos_theta, q));
}

q16_16_t q16_16_vector_magnitude(q16_16_t alpha, q16_16_t beta)
{
    q16_16_t alpha_sq = q16_16_mul(alpha, alpha);
    q16_16_t beta_sq = q16_16_mul(beta, beta);
    q16_16_t sum_sq = q16_16_add(alpha_sq, beta_sq);

    return q16_16_sqrt(sum_sq);
}

q16_16_t q16_16_vector_magnitude_sq(q16_16_t alpha, q16_16_t beta)
{
    return q16_16_add(q16_16_mul(alpha, alpha), q16_16_mul(beta, beta));
}

void q16_16_pi_init(q16_16_pi_t *pi, q16_16_t kp, q16_16_t ki, q16_16_t max_val, q16_16_t min_val, q16_16_t integ_sat)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->max_value = max_val;
    pi->min_value = min_val;
    pi->integ_sat = integ_sat;
    pi->integral = 0;
    pi->target = 0;
    pi->real = 0;
    pi->err = 0;
    pi->out = 0;
}

void q16_16_pi_calc(q16_16_pi_t *pi, q16_16_t dt_q)
{
    // 步骤1: 计算误差
    pi->err = q16_16_sub(pi->target, pi->real);

    // 步骤2: 更新积分项（含Ts·Ki）
    q16_16_t ki_term = q16_16_mul(q16_16_mul(pi->ki, pi->err), dt_q);
    pi->integral = q16_16_add(pi->integral, ki_term);

    // 对积分项单一饱和（防止windup）
    pi->integral = q16_16_clip(pi->integral, -pi->integ_sat, pi->integ_sat);

    // 步骤3: 计算比例项
    q16_16_t kp_term = q16_16_mul(pi->kp, pi->err);

    // 步骤4: 合并P和I，进行单一输出饱和
    pi->out = q16_16_add(pi->integral, kp_term);
    pi->out = q16_16_clip(pi->out, pi->min_value, pi->max_value);
}

void q16_16_pi_reset(q16_16_pi_t *pi)
{
    pi->integral = 0;
    pi->err = 0;
    pi->out = 0;
}

/* ============= 低通滤波实现 ============= */

q16_16_t q16_16_lpf_update(q16_16_t old_val, q16_16_t new_val, q16_16_t lpf_k)
{
    q16_16_t delta = q16_16_sub(new_val, old_val);
    return q16_16_add(old_val, q16_16_mul(lpf_k, delta));
}

void q16_16_ma_filter_init(q16_16_ma_filter_t *filter, q16_16_t *buf, uint16_t len)
{
    filter->buffer = buf;
    filter->length = len;
    filter->idx = 0;
    filter->sum = 0;
}

q16_16_t q16_16_ma_filter_update(q16_16_ma_filter_t *filter, q16_16_t new_val)
{
    filter->sum = q16_16_sub(filter->sum, filter->buffer[filter->idx]);
    filter->sum = q16_16_add(filter->sum, new_val);
    filter->buffer[filter->idx] = new_val;

    filter->idx = (filter->idx + 1) % filter->length;

    return filter->sum / filter->length;
}

/* ============= 角度处理实现 ============= */

/**
 * @brief Q16.16定点数角度归一化
 * @brief 将角度归一化到 [-π, π) 范围
 * @brief 使用快速模运算代替循环，大幅提升性能
 * @param angle_q 输入角度（Q16.16格式，弧度）
 * @return 归一化后的角度（Q16.16格式）
 * @note 优化后使用魔数法，时间复杂度 O(1)，仅需 ~1us
 */
q16_16_t q16_16_normalize_angle(q16_16_t angle_q)
{
    // 【第一步】快速模运算：将 angle 转换到 (-2π, 2π) 范围
    // 原理：angle % 2π = angle - k*(2π)，其中 k = round(angle / 2π)
    // 使用预计算的魔数 INV_2PI_Q = 1/(2π) ≈ 10430 (Q16.16)
    const q16_16_t TWO_PI_Q = Q16_16_2PI;      // 2π ≈ 411775 (Q16.16)
    const q16_16_t INV_2PI_Q = INT32_C(10430); // 预计算的 1/(2π) = 0.159155...

    // 计算 k = angle / (2π)，通过乘法近似除法
    q16_16_t k_float = q16_16_mul(angle_q, INV_2PI_Q);
    int32_t k = Q16_16_TO_INT_ROUND(k_float); // 四舍五入取整

    // result = angle - k * 2π，将角度转换到 (-2π, 2π)
    q16_16_t result = q16_16_sub(angle_q, q16_16_mul(INT_TO_Q16_16(k), TWO_PI_Q));

    // 【第二步】平移到 [-π, π) 范围
    // 如果 result >= π，说明角度在 [π, 2π)，减去 2π 平移到 (-π, 0]
    if (result >= Q16_16_PI)
    {
        return q16_16_sub(result, TWO_PI_Q);
    }
    // 如果 result < -π，说明角度在 (-2π, -π)，加上 2π 平移到 (0, π)
    else if (result < q16_16_sub(0, Q16_16_PI))
    {
        return q16_16_add(result, TWO_PI_Q);
    }

    // result 已在 [-π, π) 范围内，直接返回
    return result;
}

/**
 * @brief Q16.16定点数角度归一化（0 ~ 2π）
 * @brief 将角度归一化到 [0, 2π) 范围
 * @brief 利用 q16_16_normalize_angle 实现，避免代码冗余
 * @param angle_q 输入角度（Q16.16格式，弧度）
 * @return 归一化后的角度（Q16.16格式，范围 [0, 2π)）
 */
q16_16_t q16_16_normalize_angle_0_2pi(q16_16_t angle_q)
{
    // 先归一化到 [-π, π)，然后平移到 [0, 2π)
    q16_16_t result = q16_16_normalize_angle(angle_q);

    // 如果结果在 [-π, 0)，加上 2π 转换到 [π, 2π)
    if (result < 0)
    {
        result = q16_16_add(result, Q16_16_2PI);
    }

    return result;
}

/**
 * @brief 计算两个Q16.16角度之间的最小差值
 *
 * 该函数用于计算两个角度之间的最小角度差，确保结果在[-π, π)范围内。
 * 这在电机控制、机器人导航等应用中非常有用，可以避免角度环绕问题。
 *
 * @param target 目标角度（Q16.16格式，弧度）
 * @param current 当前角度（Q16.16格式，弧度）
 *
 * @return 最小角度差（Q16.16格式，弧度），范围[-π, π)
 *
 * @note 示例：
 * - target = 350°, current = 10° → 返回 -20°（而不是340°）
 * - target = 10°, current = 350° → 返回 20°（而不是-340°）
 *
 * @see q16_16_normalize_angle()
 */
q16_16_t q16_16_angle_diff(q16_16_t target, q16_16_t current)
{
    // 计算原始角度差
    q16_16_t diff = q16_16_sub(target, current);

    // 将差值归一化到[-π, π)范围，确保是最小角度差
    return q16_16_normalize_angle(diff);
}