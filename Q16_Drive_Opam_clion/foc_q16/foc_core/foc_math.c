/**
 * @brief:   Q16.16 定点数学库实现
 * @FilePath:  foc_math.c
 * @author:  fubingyan qq:3245784484
 * @date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#include "foc_math.h"

/* ============= Q16.16 三角函数（LUT + 线性插值实现） ============= */

#define SIN_TABLE_SIZE 512

static const q16_16_t sin_table[SIN_TABLE_SIZE] = {
    0, 804, 1608, 2412, 3216, 4019, 4821, 5623, 6424, 7224, 8022, 8820, 9616, 10411,
    11204, 11996, 12785, 13573, 14359, 15143, 15924, 16703, 17479, 18253, 19024, 19792, 20557, 21320,
    22078, 22834, 23586, 24335, 25080, 25821, 26558, 27291, 28020, 28745, 29466, 30182, 30893, 31600,
    32303, 33000, 33692, 34380, 35062, 35738, 36410, 37076, 37736, 38391, 39040, 39683, 40320, 40951,
    41576, 42194, 42806, 43412, 44011, 44604, 45190, 45769, 46341, 46906, 47464, 48015, 48559, 49095,
    49624, 50146, 50660, 51166, 51665, 52156, 52639, 53114, 53581, 54040, 54491, 54934, 55368, 55794,
    56212, 56621, 57022, 57414, 57798, 58172, 58538, 58896, 59244, 59583, 59914, 60235, 60547, 60851,
    61145, 61429, 61705, 61971, 62228, 62476, 62714, 62943, 63162, 63372, 63572, 63763, 63944, 64115,
    64277, 64429, 64571, 64704, 64827, 64940, 65043, 65137, 65220, 65294, 65358, 65413, 65457, 65492,
    65516, 65531, 65536, 65531, 65516, 65492, 65457, 65413, 65358, 65294, 65220, 65137, 65043, 64940,
    64827, 64704, 64571, 64429, 64277, 64115, 63944, 63763, 63572, 63372, 63162, 62943, 62714, 62476,
    62228, 61971, 61705, 61429, 61145, 60851, 60547, 60235, 59914, 59583, 59244, 58896, 58538, 58172,
    57798, 57414, 57022, 56621, 56212, 55794, 55368, 54934, 54491, 54040, 53581, 53114, 52639, 52156,
    51665, 51166, 50660, 50146, 49624, 49095, 48559, 48015, 47464, 46906, 46341, 45769, 45190, 44604,
    44011, 43412, 42806, 42194, 41576, 40951, 40320, 39683, 39040, 38391, 37736, 37076, 36410, 35738,
    35062, 34380, 33692, 33000, 32303, 31600, 30893, 30182, 29466, 28745, 28020, 27291, 26558, 25821,
    25080, 24335, 23586, 22834, 22078, 21320, 20557, 19792, 19024, 18253, 17479, 16703, 15924, 15143,
    14359, 13573, 12785, 11996, 11204, 10411, 9616, 8820, 8022, 7224, 6424, 5623, 4821, 4019,
    3216, 2412, 1608, 804, 0, -804, -1608, -2412, -3216, -4019, -4821, -5623, -6424, -7224,
    -8022, -8820, -9616, -10411, -11204, -11996, -12785, -13573, -14359, -15143, -15924, -16703, -17479, -18253,
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
    -17479, -16703, -15924, -15143, -14359, -13573, -12785, -11996, -11204, -10411, -9616, -8820, -8022, -7224,
    -6424, -5623, -4821, -4019, -3216, -2412, -1608, -804
};

#define SIN_LUT_INDEX_MULTIPLIER 5340352ULL // (512ULL << 32) / 411775 = 5340351.78, 向上取整确保 2π 映射到索引 512

/**
 * @brief 同时计算 Q16.16 角度的正弦和余弦值
 *
 * 实现原理（查找表 + 线性插值）：
 *   1. 将输入角度归一化到 [0, 2π) 范围，使用 64 位除法避免精度损失
 *   2. 通过乘法定理将 [0, 2π) 映射到 [0, SIN_TABLE_SIZE) 索引空间：
 *      索引乘数 = (SIN_TABLE_SIZE << 32) / (2π) ≈ 5340352
 *      利用 64 位乘法直接分离整数索引和小数部分，无需除法
 *   3. 查表得到相邻两个索引的 sin 值，cos 通过相位偏移 SIN_TABLE_SIZE/4 获得
 *   4. 对小数部分做 16 位线性插值，平衡精度和计算速度
 *
 * 性能特性：
 *   - 查表：4 次内存读取
 *   - 运算：1 次 64 位除法（角度归一化）+ 1 次 64 位乘法（索引计算）+ 少量整数运算
 *   - 精度：约 0.01°（线性插值将 512 点查表的精度从 ~0.7° 提升至 ~0.01°）
 *
 * @param angle_q  输入角度（Q16.16格式，弧度）
 * @param sin_out  输出正弦值指针（Q16.16格式）
 * @param cos_out  输出余弦值指针（Q16.16格式）
 */
void foc_sin_cos(q16_16_t angle_q, q16_16_t* sin_out, q16_16_t* cos_out)
{
    /* 步骤1：角度归一化到 [0, 2π)，使用 64 位除法避免取模运算的溢出风险 */
    int32_t k = (int32_t)((int64_t)angle_q / Q16_16_2PI);
    angle_q = (q16_16_t)((int64_t)angle_q - (int64_t)k * Q16_16_2PI);
    if (angle_q < 0)
        angle_q = q16_16_add(angle_q, Q16_16_2PI);

    /* 步骤2：将角度映射到 LUT 索引空间，64 位乘积同时得到整数索引和小数部分 */
    uint64_t scaled = (uint64_t)angle_q * SIN_LUT_INDEX_MULTIPLIER;

    uint32_t index = (uint32_t)(scaled >> 32); // 高32位：整数索引（0~511）
    uint32_t frac_full = (uint32_t)(scaled & 0xFFFFFFFFUL);
    uint16_t frac = (uint16_t)(frac_full >> 16); // 高16位小数：插值权重

    /* 步骤3：安全取模（表大小为2的幂，与运算代替除法），计算cos索引（相位偏移π/2） */
    index &= (SIN_TABLE_SIZE - 1);
    uint32_t index_cos = (index + (SIN_TABLE_SIZE >> 2)) & (SIN_TABLE_SIZE - 1);

    /* 步骤4：查表获取相邻两个索引的正弦值和余弦值 */
    q16_16_t sin1 = sin_table[index];
    q16_16_t sin2 = sin_table[(index + 1) & (SIN_TABLE_SIZE - 1)];
    q16_16_t cos1 = sin_table[index_cos];
    q16_16_t cos2 = sin_table[(index_cos + 1) & (SIN_TABLE_SIZE - 1)];

    /* 步骤5：线性插值，使用 64 位中间结果避免溢出 */
    int64_t diff_sin = (int64_t)sin2 - sin1;
    int64_t diff_cos = (int64_t)cos2 - cos1;

    q16_16_t interp_sin = (q16_16_t)((diff_sin * frac) >> 16);
    q16_16_t interp_cos = (q16_16_t)((diff_cos * frac) >> 16);

    /* 步骤6：输出最终插值结果 */
    *sin_out = sin1 + interp_sin;
    *cos_out = cos1 + interp_cos;
}

/**
 * @brief Q16.16 快速平方根
 */
q16_16_t foc_sqrt(q16_16_t x)
{
    if (x <= 0)
        return 0;
    if (x == Q16_16_ONE)
        return Q16_16_ONE;

    int clz = __builtin_clz((uint32_t)x);
    q16_16_t y = (q16_16_t)(1u << ((17 + clz) >> 1));

    for (int i = 0; i < 3; i++) {
        y = (q16_16_t)((((int64_t)y + ((int64_t)x << 16) / y)) >> 1);
    }

    return y;
}

/**
 * @brief Q16.16格式的平方根倒数函数（纯整数实现）
 * 使用幂二等初始估值 + 三次牛顿迭代，约 0.01% 精度
 * @param x 输入值（Q16.16格式，必须 > 0）
 * @return 平方根倒数（Q16.16格式）
 */
q16_16_t foc_inv_sqrt(q16_16_t x)
{
    if (x <= 0)
        return INT32_MAX;
    if (x == Q16_16_ONE)
        return Q16_16_ONE;

    int clz = __builtin_clz((uint32_t)x);
    int exp = (17 + clz) >> 1;
    q16_16_t y = (q16_16_t)(1u << exp);

    int iterations = (clz > 24) ? 4 : 3;
    for (int i = 0; i < iterations; i++) {
        int64_t y_sq = ((int64_t)y * y) >> 16;
        int64_t xy_sq = ((int64_t)x * y_sq) >> 16;
        int64_t three = (int64_t)3 << 16;
        int64_t half_diff = (three - xy_sq) >> 1;
        y = (q16_16_t)(((int64_t)y * half_diff) >> 16);
    }

    return y;
}

/**
 * @brief Q16.16 反正切
 * 使用 2阶 Minimax 多项式逼近替代泰勒级数，极大提升运行速度并减少除法开销
 * 逼近公式：atan(r) ≈ (π/4)*r + 0.273*r*(1 - r),  r ∈ [0, 1]
 * 最大角度误差 < 0.22°，完全去除级数中的高阶定点除法操作
 */
q16_16_t foc_atan2(q16_16_t y, q16_16_t x)
{
    if (x == 0 && y == 0)
        return 0; // 工程约定：atan2(0,0) = 0，数学上未定义

    if (x == 0) {
        return (y >= 0) ? Q16_16_PI_2 : q16_16_sub(0, Q16_16_PI_2);
    }

    q16_16_t abs_x = q16_16_abs(x);
    q16_16_t abs_y = q16_16_abs(y);
    q16_16_t angle;

    // 确保比率 ratio 在 [0, 1] 范围内
    if (abs_x >= abs_y) {
        q16_16_t ratio = q16_16_div(abs_y, abs_x);

        // angle ≈ (π/4)*ratio + 0.273*ratio*(1 - ratio)
        q16_16_t one_minus_ratio = q16_16_sub(Q16_16_ONE, ratio);
        q16_16_t term1 = q16_16_mul(Q16_16_PI_4, ratio);
        q16_16_t ratio_one_minus = q16_16_mul(ratio, one_minus_ratio);
        q16_16_t term2 = q16_16_mul(INT32_C(17891), ratio_one_minus); // 0.273 在 Q16.16 格式下近似为 17891 (0.273 * 65536)

        angle = q16_16_add(term1, term2);
    } else {
        q16_16_t ratio = q16_16_div(abs_x, abs_y);

        q16_16_t one_minus_ratio = q16_16_sub(Q16_16_ONE, ratio);
        q16_16_t term1 = q16_16_mul(Q16_16_PI_4, ratio);
        q16_16_t ratio_one_minus = q16_16_mul(ratio, one_minus_ratio);
        q16_16_t term2 = q16_16_mul(INT32_C(17891), ratio_one_minus);

        angle = q16_16_add(term1, term2);
        // 使用恒等式：atan(y/x) = π/2 - atan(x/y)
        angle = q16_16_sub(Q16_16_PI_2, angle);
    }

    // 四象限映射
    if (x < 0) {
        if (y >= 0)
            return q16_16_sub(Q16_16_PI, angle);
        else
            return q16_16_sub(angle, Q16_16_PI);
    } else {
        if (y >= 0)
            return angle;
        else
            return q16_16_sub(0, angle);
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
void foc_clarke_transform(q16_16_t ia, q16_16_t ib, q16_16_t ic, q16_16_t* alpha, q16_16_t* beta)
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
void foc_park_transform(q16_16_t alpha, q16_16_t beta, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t* d,
    q16_16_t* q)
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
void foc_ipark_transform(q16_16_t d, q16_16_t q, q16_16_t sin_theta, q16_16_t cos_theta, q16_16_t* alpha,
    q16_16_t* beta)
{
    /* 逆变换公式
     * V_alpha = V_d * cos(θ) - V_q * sin(θ)
     * V_beta  = V_d * sin(θ) + V_q * cos(θ)
     */
    *alpha = q16_16_sub(q16_16_mul(cos_theta, d), q16_16_mul(sin_theta, q));
    *beta = q16_16_add(q16_16_mul(sin_theta, d), q16_16_mul(cos_theta, q));
}

/**
 * @brief 计算二维矢量的幅值
 *
 * 计算 sqrt(alpha² + beta²)，内部使用 foc_sqrt 实现定点开方。
 * 使用场景：电压矢量幅度归一化、过调制检测等。
 * 若仅需比较幅度大小，建议使用 foc_vector_magnitude_sq 以避免开方开销。
 *
 * @param alpha α 轴分量（Q16.16格式）
 * @param beta  β 轴分量（Q16.16格式）
 * @return 矢量幅值（Q16.16格式）
 */
q16_16_t foc_vector_magnitude(q16_16_t alpha, q16_16_t beta)
{
    q16_16_t alpha_sq = q16_16_mul(alpha, alpha);
    q16_16_t beta_sq = q16_16_mul(beta, beta);
    q16_16_t sum_sq = q16_16_add(alpha_sq, beta_sq);

    return foc_sqrt(sum_sq);
}

/**
 * @brief 计算二维矢量幅值的平方
 *
 * 直接计算 alpha² + beta²，避免了开方运算。
 * 适用场景：
 *   - 仅需比较矢量幅度大小时（如过调制判断）
 *   - 不需要精确幅值的场合
 *   相比 foc_vector_magnitude，计算量小且无精度损失。
 *
 * @param alpha α 轴分量（Q16.16格式）
 * @param beta  β 轴分量（Q16.16格式）
 * @return 矢量幅值的平方（Q16.16格式）
 */
q16_16_t foc_vector_magnitude_sq(q16_16_t alpha, q16_16_t beta)
{
    return q16_16_add(q16_16_mul(alpha, alpha), q16_16_mul(beta, beta));
}

/**
 * @brief 初始化PI控制器
 *
 * 将 Ki 预先乘以 dt（采样周期），使得 foc_pi_calc() 中无需重复乘 dt，
 * 减少一次乘法运算。所有状态量（integral、err、out）清零。
 *
 * @param pi        PI控制器结构体指针
 * @param kp        比例增益（Q16.16格式）
 * @param ki        积分增益原始值（Q16.16格式，内部会预乘 dt）
 * @param max_val   输出最大值（Q16.16格式）
 * @param min_val   输出最小值（Q16.16格式）
 * @param integ_sat 积分饱和限制（Q16.16格式，绝对值），防止积分深度饱和
 * @param dt_q      采样周期（Q16.16格式，秒）
 */
void foc_pi_init(foc_pi_t* pi, q16_16_t kp, q16_16_t ki, q16_16_t max_val, q16_16_t min_val, q16_16_t integ_sat,
    q16_16_t dt_q)
{
    pi->kp = kp;
    pi->ki = q16_16_mul(ki, dt_q); // 预乘 dt，calc() 中省去一次乘法
    pi->max_value = max_val;
    pi->min_value = min_val;
    pi->integ_sat = integ_sat;
    pi->integral = 0;
    pi->target = 0;
    pi->real = 0;
    pi->err = 0;
    pi->out = 0;
}

/**
 * @brief 执行PI控制器计算
 *
 * 计算公式：out = integral + Kp * err
 * 其中 integral += Ki_premultiplied * err  (Ki_premultiplied = Ki * dt)
 *
 * 抗积分饱和（Anti-Windup）策略：
 *   - 当输出达到限幅值且误差方向使饱和加深时，停止积分累积
 *   - 当输出未饱和或误差方向使饱和减弱时，允许正常积分
 *   - 积分项独立受 integ_sat 限制，防止深度饱和后恢复迟缓
 *
 * @param pi PI控制器结构体指针（需事先调用 foc_pi_init 初始化）
 */
void foc_pi_calc(foc_pi_t* pi)
{
    pi->err = q16_16_sub(pi->target, pi->real);

    q16_16_t kp_term = q16_16_mul(pi->kp, pi->err);
    q16_16_t out_temp = q16_16_add(pi->integral, kp_term);

    uint8_t saturated = (out_temp >= pi->max_value && pi->err > 0)
        || (out_temp <= pi->min_value && pi->err < 0);

    if (!saturated) {
        q16_16_t ki_term = q16_16_mul(pi->ki, pi->err);
        pi->integral = q16_16_add(pi->integral, ki_term);
        pi->integral = q16_16_clip(pi->integral, -pi->integ_sat, pi->integ_sat);
    }

    pi->out = q16_16_add(pi->integral, kp_term);
    pi->out = q16_16_clip(pi->out, pi->min_value, pi->max_value);
}

/**
 * @brief 重置PI控制器状态
 *
 * 清零积分累积值（integral）、误差（err）和输出（out）。
 * 保留 kp、ki、max_value、min_value、integ_sat 等配置参数不变。
 * 适用场景：
 *   - 电机启动/停止时清除历史累积
 *   - 控制模式切换（如速度环→电流环）时重置
 *   - 故障恢复后重新初始化
 *
 * @param pi PI控制器结构体指针
 */
void foc_pi_reset(foc_pi_t* pi)
{
    pi->integral = 0;
    pi->err = 0;
    pi->out = 0;
}

/* ============= 低通滤波实现 ============= */

/**
 * @brief 一阶低通滤波器更新（通用版）
 *
 * 差分方程：out = old + K * (new - old)
 * 等价于  out = (1 - K) * old + K * new
 *
 * 适用场景：
 *   - 电流采样滤波（抑制 ADC 噪声）
 *   - 速度估计平滑
 *   - 角度信号去噪
 *
 * 注意：当滤波系数 K 为 2 的负幂次时，建议使用 foc_lpf_update_shift
 * 以提高性能。
 *
 * @param old_val 上一次滤波输出值（Q16.16格式）
 * @param new_val 当前采样输入值（Q16.16格式）
 * @param lpf_k   滤波系数（Q16.16格式），范围 [0, 1]，越大响应越快
 * @return 滤波后的输出值（Q16.16格式）
 */
q16_16_t foc_lpf_update(q16_16_t old_val, q16_16_t new_val, q16_16_t lpf_k)
{
    q16_16_t delta = q16_16_sub(new_val, old_val);
    return q16_16_add(old_val, q16_16_mul(lpf_k, delta));
}

/**
 * @brief 一阶低通滤波器更新（移位版）
 *
 * 使用右移代替乘法实现滤波，等效滤波系数 K = 1 / (2^shift)。
 * 差分方程：out = old + (new - old) >> shift
 *
 * 适用场景：
 *   - 对性能敏感的实时控制中断中
 *   - 滤波系数可容忍 2 的幂次精度（如 1/2, 1/4, 1/8 等）
 *   - 相比 foc_lpf_update 节省一次 32×32 定点乘法
 *
 * 注意：不支持通用系数配置，需预先确定位移位数。
 *
 * @param old_val 上一次滤波输出值（Q16.16格式）
 * @param new_val 当前采样输入值（Q16.16格式）
 * @param shift   右移位数（0~31），等效滤波系数 = 1/(2^shift)
 *                例如 shift=2 时 K=0.25
 * @return 滤波后的输出值（Q16.16格式）
 */
q16_16_t foc_lpf_update_shift(q16_16_t old_val, q16_16_t new_val, uint8_t shift)
{
    int64_t delta = (int64_t)new_val - old_val;
    return (q16_16_t)(old_val + (delta >> shift));
}

/**
 * @brief 初始化滑动平均滤波器
 *
 * 滑动平均滤波器使用环形缓冲区存储最近的 N 个采样值，
 * 每次更新返回缓冲区内所有采样值的算术平均。
 * 由于 N 必须为 2 的幂，求平均值通过右移实现，无需除法。
 *
 * 约束：
 *   - length 必须为 2 的幂（如 4、8、16、32）
 *   - buf 指向的缓冲区必须由调用者分配，长度 ≥ len
 *
 * @param filter 滤波器结构体指针
 * @param buf    预先分配的环形缓冲区指针（长度 ≥ len）
 * @param len    缓冲区长度（必须为 2 的幂）
 */
void foc_ma_filter_init(foc_ma_filter_t* filter, q16_16_t* buf, uint16_t len)
{
    if (len == 0 || (len & (len - 1)) != 0 || buf == NULL)
        return;

    filter->buffer = buf;
    filter->length = len;
    filter->idx = 0;
    filter->sum = 0;
    filter->shift = 0;
    while ((1u << filter->shift) < len)
        filter->shift++;
}

/**
 * @brief 更新滑动平均滤波器
 *
 * 将新值写入环形缓冲区，替换最旧的值，同时更新累加和。
 * 返回当前窗口的算术平均值 = sum / length（通过右移实现）。
 *
 * 使用场景：
 *   - 电流/电压采样预滤波
 *   - 速度/位置估计平滑
 *   - 需要平滑但无需快速响应的场合
 *
 * @param filter  滤波器结构体指针（需事先调用 foc_ma_filter_init）
 * @param new_val 新的采样值（Q16.16格式）
 * @return 当前滑动平均值（Q16.16格式）
 */
q16_16_t foc_ma_filter_update(foc_ma_filter_t* filter, q16_16_t new_val)
{
    if (filter->buffer == NULL || filter->length == 0)
        return 0;

    filter->sum -= filter->buffer[filter->idx];
    filter->sum += new_val;
    filter->buffer[filter->idx] = new_val;

    filter->idx = (filter->idx + 1) & (filter->length - 1);

    return (q16_16_t)(filter->sum >> filter->shift);
}

/* ============= 角度处理实现 ============= */

/**
 * @brief Q16.16定点数角度归一化
 * @brief 将角度归一化到 [-π, π) 范围
 * @brief 使用64位精确整数除法，无精度损失，支持任意大角度输入
 * @param angle_q 输入角度（Q16.16格式，弧度）
 * @return 归一化后的角度（Q16.16格式）
 * @note 时间复杂度 O(1)
 */
q16_16_t foc_normalize_angle(q16_16_t angle_q)
{
    const q16_16_t TWO_PI_Q = Q16_16_2PI;

    int32_t k = (int32_t)((int64_t)angle_q / TWO_PI_Q);
    q16_16_t result = (q16_16_t)((int64_t)angle_q - (int64_t)k * TWO_PI_Q);

    if (result >= Q16_16_PI) {
        return q16_16_sub(result, TWO_PI_Q);
    } else if (result < q16_16_sub(0, Q16_16_PI)) {
        return q16_16_add(result, TWO_PI_Q);
    }

    return result;
}

/**
 * @brief Q16.16定点数角度归一化（0 ~ 2π）
 * @brief 将角度归一化到 [0, 2π) 范围
 * @brief 利用 foc_normalize_angle 实现，避免代码冗余
 * @param angle_q 输入角度（Q16.16格式，弧度）
 * @return 归一化后的角度（Q16.16格式，范围 [0, 2π)）
 */
q16_16_t foc_normalize_angle_0_2pi(q16_16_t angle_q)
{
    // 先归一化到 [-π, π)，然后平移到 [0, 2π)
    q16_16_t result = foc_normalize_angle(angle_q);

    // 如果结果在 [-π, 0)，加上 2π 转换到 [π, 2π)
    if (result < 0) {
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
 * @see foc_normalize_angle()
 */
q16_16_t foc_angle_diff(q16_16_t target, q16_16_t current)
{
    // 计算原始角度差
    q16_16_t diff = q16_16_sub(target, current);

    // 将差值归一化到[-π, π)范围，确保是最小角度差
    return foc_normalize_angle(diff);
}
