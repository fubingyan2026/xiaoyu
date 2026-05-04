/**
 * @brief:  FOC控制器（Q16.16定点版本）
 * @FilePath: foccontroller.c
 * @author: fubingyan qq:3245784484
 * @Date:  2026-01-11
 * @version: V1.0.0
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
#include "foc_ctrl_q16.h" // FOC控制器头文件
#include "bsp_delay.h"
#include "debug.h"
#include "foc_port.h" // FOC端口适配层头文件
#include "foc_port.h"
#include "foc_sensor.h" // FOC传感器抽象层
#include "foc_sm.h"     // FOC状态机头文件
#include "main.h"
#include "memory_pool.h"

/* ==================== 前向声明 ==================== */

/* ==================== 硬件相关全局变量 ==================== */
#define ADC_INSTANCE hadc1
#define PWM_INSTANCE htim1
extern ADC_HandleTypeDef ADC_INSTANCE;
extern TIM_HandleTypeDef PWM_INSTANCE;

uint32_t current_dma_value[3];           // ADC DMA采样值缓冲区
q16_16_t current_adc_init_buff[3] = {0}; // ADC初始偏置值（Q16.16格式）

/* ==================== FOC全局变量 ==================== */

q16_16_pi_t q16_16_pi_id = {0};  // D轴PI控制器实例
q16_16_pi_t q16_16_pi_iq = {0};  // Q轴PI控制器实例
svpwm_t g_svpwm = {0};           // SVPWM结构体实例
foc_sample_t g_adc_sample = {0}; // ADC采样数据结构体实例
foc_ctrl_t foc_ctrl = {0};       // FOC控制结构体实例

/* ==================== 硬件回调函数实现 ==================== */

/**
 * @brief ADC采样读取回调函数（Q16.16定点版本）
 */
static void stm32_adc_read(q16_16_t *current_a, q16_16_t *current_b, q16_16_t *current_c)
{
    if (!current_a || !current_b || !current_c)
    {
        return;
    }

    // 读取ADC原始值
    uint32_t adc_a = current_dma_value[0];
    uint32_t adc_b = current_dma_value[1];

    // 计算ADC差值并转换为Q16.16格式
    // 使用定点运算: (adc - offset) * CURRENT_SAMPLE_FACTOR
    q16_16_t diff_a = INT_TO_Q16_16((int32_t)adc_a) - current_adc_init_buff[0];
    q16_16_t diff_b = INT_TO_Q16_16((int32_t)adc_b) - current_adc_init_buff[1];

    // 乘以采样因子得到电流值（Q16.16格式）
    *current_a = q16_16_mul(diff_a, foc_port_get_instance()->config.current_sample_factor_q);
    *current_b = q16_16_mul(diff_b, foc_port_get_instance()->config.current_sample_factor_q);

    // C相电流 = -(A相 + B相)，使用基尔霍夫定律
    *current_c = -(*current_a + *current_b);
}

/**
 * @brief ADC初始化回调函数
 */
static void stm32_adc_init(void)
{
    __HAL_ADC_DISABLE_IT(&ADC_INSTANCE, ADC_IT_JEOC);
    HAL_ADCEx_InjectedStart(&ADC_INSTANCE);
    adc_dma_start_convert();

    // 等待ADC DMA转换完成（带超时保护）
    // 大厂做法：使用HAL_GetTick()进行超时检测，避免无限循环
    uint32_t timeout = HAL_GetTick();
    const uint32_t adc_init_timeout_ms = 500;  // 500ms超时
    bool adc_data_valid = false;

    while ((HAL_GetTick() - timeout) < adc_init_timeout_ms)
    {
        // 检查三相电流ADC值是否都有效（非零）
        if (current_dma_value[0] != 0 && current_dma_value[1] != 0 && current_dma_value[2] != 0)
        {
            adc_data_valid = true;
            break;
        }
        // 短暂延时后重试，避免长时间阻塞
        delay_ms(1);
        adc_dma_start_convert();
    }

    // 超时处理：使用默认偏置值（避免系统死机）
    if (!adc_data_valid)
    {
        // 可以添加错误日志或告警
        current_dma_value[0] = 2048;  // 默认中值
        current_dma_value[1] = 2048;
        current_dma_value[2] = 2048;
    }

    // 将ADC初始偏置值转换为Q16.16格式
    current_adc_init_buff[0] = INT_TO_Q16_16((int32_t)current_dma_value[0]);
    current_adc_init_buff[1] = INT_TO_Q16_16((int32_t)current_dma_value[1]);
    current_adc_init_buff[2] = INT_TO_Q16_16((int32_t)current_dma_value[2]);

    HAL_TIM_Base_Start_IT(&PWM_INSTANCE);
    HAL_TIM_PWM_Start_IT(&PWM_INSTANCE, TIM_CHANNEL_4);
}

/**
 * @brief PWM输出回调函数
 */
static void stm32_pwm_output(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td)
{
    if (ta > PWM_PERIOD)
        ta = PWM_PERIOD;
    if (tb > PWM_PERIOD)
        tb = PWM_PERIOD;
    if (tc > PWM_PERIOD)
        tc = PWM_PERIOD;
    if (td > PWM_PERIOD)
        td = PWM_PERIOD;

    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_1, ta);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_2, tb);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_3, tc);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_4, td + 5);
}

/**
 * @brief PWM启动回调函数
 */
static void stm32_pwm_start(void)
{
    __HAL_TIM_SET_COUNTER(&PWM_INSTANCE, 0);

    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_3);
}

/**
 * @brief PWM停止回调函数
 */
static void stm32_pwm_stop(void)
{
    __HAL_TIM_DISABLE(&PWM_INSTANCE);
    HAL_TIMEx_PWMN_Stop(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&PWM_INSTANCE, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&PWM_INSTANCE, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop_IT(&PWM_INSTANCE, TIM_CHANNEL_4);
    HAL_TIM_Base_Stop_IT(&PWM_INSTANCE);
}

/**
 * @brief 编码器读取回调函数
 */
static uint16_t stm32_encoder_read(void)
{
    return foc_sensor_get_raw_angle();
}

/**
 * @brief 延时回调函数
 */
static void stm32_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

/**
 * @brief 启动ADC DMA转换
 */
void adc_dma_start_convert(void)
{
    HAL_ADC_Start_DMA(&ADC_INSTANCE, current_dma_value, sizeof(current_dma_value) / sizeof(current_dma_value[0]));
}

/**
 * @brief 周期平均函数（未在当前文件中使用，可能为遗留或通用函数）
 * @param _a 值A
 * @param _b 值B
 * @param _cyc 周期值
 * @return 平均值
 */
static int32_t cycle_average(const int32_t _a, const int32_t _b, const int32_t _cyc);

/**
 * @brief FOC锁相环（PLL）运行函数
 * @param phase_q 输入相位（Q16.16格式）
 * @param dt_q 采样时间间隔（Q16.16格式）
 * @param phase_var_q 输出相位变量指针（Q16.16格式）
 * @param speed_var_q 输出速度变量指针（Q16.16格式）
 * @param kp_q 比例增益（Q16.16格式）
 * @param ki_q 积分增益（Q16.16格式）
 */
static void foc_pll_run(q16_16_t phase_q, q16_16_t dt_q, q16_16_t *phase_var_q, q16_16_t *speed_var_q, q16_16_t kp_q,
                        q16_16_t ki_q);

/**
 * @brief FOC初始化函数
 */
extern void foc_init(void)
{
    foc_sensor_init(foc_sensor_get_default_type()); // 传感器初始化
    encoder_alignment_init();                       // 加载电机闪存数据

    foc_ctrl.foc_ctrl_cycle_s = FOC_PWM_PERIOD_Q;
    foc_ctrl.sw = true; // 设置FOC控制开关为真
    /* 初始化FOC端口 */
    foc_port_config_t port_config = {
        .adc_read = stm32_adc_read,
        .adc_init = stm32_adc_init,
        .pwm_output = stm32_pwm_output,
        .pwm_start = stm32_pwm_start,
        .pwm_stop = stm32_pwm_stop,
        .encoder_read = stm32_encoder_read,
        .delay_ms = stm32_delay_ms,
        .pwm_period_counts = PWM_PERIOD,
        .current_sample_factor_q = CURRENT_SAMPLE_FACTOR_Q,
    };

    foc_port_t *port = foc_port_get_instance();
    foc_port_init(port, &port_config, &g_adc_sample, &g_svpwm, &foc_ctrl);
    foc_port_adc_init(port); // 初始化ADC

    q16_16_pi_reset(&q16_16_pi_id);
    q16_16_pi_reset(&q16_16_pi_iq);

    // D轴 PI 控制器初始化
    q16_16_pi_init(&q16_16_pi_id, CURRENT_ID_KP_Q, CURRENT_ID_KI_Q, CURRENT_ID_OUT_MAX_Q, CURRENT_ID_OUT_MIN_Q,
                   CURRENT_ID_INTEG_SAT_Q);

    // Q轴 PI 控制器初始化
    q16_16_pi_init(&q16_16_pi_iq, CURRENT_IQ_KP_Q, CURRENT_IQ_KI_Q, CURRENT_IQ_OUT_MAX_Q, CURRENT_IQ_OUT_MIN_Q,
                   CURRENT_IQ_INTEG_SAT_Q);

    foc_sm_init(foc_sm_get_instance(), &foc_ctrl);                    // FOC状态机初始化
    foc_sm_set_flash_data(foc_sm_get_instance(), &g_motor_flash_cfg); // 设置Flash数据

    g_svpwm.v_bus = FLOAT_TO_Q16_16(V_BUS);          // 设置SVPWM母线电压
    g_svpwm.max_duty_ratio = FLOAT_TO_Q16_16(0.95f); // 设置SVPWM最大占空比
    foc_port_pwm_start(port);                        // 启动PWM
}

/**
 * @brief FOC ADC中断计算函数
 * 在ADC中断中调用，执行FOC控制算法的核心计算
 */
volatile float id_measured, iq_measured;
uint32_t get_time_us = 0;
uint32_t get_time_us_last = 0;
uint32_t dealta_us = 0;
void foc_adc_irq_calc(void)
{
    foc_port_t *port = foc_port_get_instance();

    foc_port_encoder_read(port); // 调用编码器读取钩子
    get_time_us_last = micros();

    // // 如果不是对齐状态，则更新电气角度
    if (foc_sm_current_state() == FOC_SM_STATE_RUN)
    {
        float electrical_angle = encoder_track_sector(foc_ctrl.raw_angle_q, &g_encoder_calib); // 扇区跟踪器计算电气角度
        //        foc_ctrl.electrical_angle_q += FLOAT_TO_Q16_16(0.001f); // 转换为Q16.16格式

        foc_ctrl.electrical_angle_q = FLOAT_TO_Q16_16(electrical_angle);
        foc_pll_run(foc_ctrl.electrical_angle_q, foc_ctrl.foc_ctrl_cycle_s, &foc_ctrl.pll_phase_q,
                    &foc_ctrl.pll_velocity_q, PLL_ELE_KP_Q, PLL_ELE_KI_Q);
        foc_ctrl.pll_velocity_rpm = Q16_16_TO_FLOAT(foc_ctrl.pll_velocity_q) * 60.0f / M_2PI / MOTOR_POLES;
    }
    get_time_us = micros();

    // 计算PLL相位的正弦和余弦值
    q16_16_sin_cos(foc_ctrl.electrical_angle_q, &g_svpwm.sin_cos[0], &g_svpwm.sin_cos[1]);

    // 触发ADC采样/读取
    foc_port_adc_read(port);

    // 直接使用Q16.16格式的采样值（无需转换）
    q16_16_t ia_q = g_adc_sample.current_sample_q[0];
    q16_16_t ib_q = g_adc_sample.current_sample_q[1];
    q16_16_t ic_q = g_adc_sample.current_sample_q[2];

    // Clarke 变换（使用库函数将三相电流转换为两相静止坐标系）
    q16_16_t i_alpha_q = 0;
    q16_16_t i_beta_q = 0;
    q16_16_clarke_transform(ia_q, ib_q, ic_q, &i_alpha_q, &i_beta_q);

    // Park 变换（使用库函数将两相静止坐标系电流转换为两相旋转坐标系）
    q16_16_t id_raw_q = 0;
    q16_16_t iq_raw_q = 0;

    q16_16_park_transform(i_alpha_q, i_beta_q, g_svpwm.sin_cos[0], g_svpwm.sin_cos[1], &id_raw_q, &iq_raw_q);

    // 低通滤波（使用库函数 q16_16_lpf_update 对D轴和Q轴电流进行滤波）
    const q16_16_t lpf_beta_q = FLOAT_TO_Q16_16(0.33f); // 低通滤波系数
    id_measured = Q16_16_TO_FLOAT(id_raw_q);
    iq_measured = Q16_16_TO_FLOAT(iq_raw_q);

    static q16_16_t id_out_q = 0;                                 // D轴电流输出
    static q16_16_t iq_out_q = 0;                                 // Q轴电流输出
    id_out_q = q16_16_lpf_update(id_out_q, id_raw_q, lpf_beta_q); // D轴电流低通滤波
    iq_out_q = q16_16_lpf_update(iq_out_q, iq_raw_q, lpf_beta_q); // Q轴电流低通滤波

    // 计算电流环：直接调用 q16_16_pi_calc
    q16_16_pi_id.target = foc_ctrl.target_id_q; // 设置D轴PI控制器目标值
    q16_16_pi_id.real = id_out_q;               // 设置D轴PI控制器实际值
    q16_16_pi_iq.target = foc_ctrl.target_iq_q; // 设置Q轴PI控制器目标值
    q16_16_pi_iq.real = iq_out_q;               // 设置Q轴PI控制器实际值

    // 更新 PI 控制器（定点，使用定点 dt）
    q16_16_pi_calc(&q16_16_pi_iq, foc_ctrl.foc_ctrl_cycle_s); // Q轴PI控制器计算
    q16_16_pi_calc(&q16_16_pi_id, foc_ctrl.foc_ctrl_cycle_s); // D轴PI控制器计算

    // 假设PI控制器已计算出vd_q和vq_q（Q16.16格式）
    // 将 PI 输出用于 SVPWM（示例；vd/vq 为定点）
    g_svpwm.vd = q16_16_pi_id.out; // D轴电压（Q16.16格式）
    g_svpwm.vq = q16_16_pi_iq.out; // Q轴电压（Q16.16格式）
    // 逆Park变换，将D/Q轴电压转换为Alpha/Beta轴电压
    q16_16_ipark_transform(g_svpwm.vd, g_svpwm.vq, g_svpwm.sin_cos[0], g_svpwm.sin_cos[1], &g_svpwm.v_alpha,
                           &g_svpwm.v_beta);

    // 调用SVPWM计算占空比
    svpwm_calculate(&g_svpwm);
    foc_port_pwm_update(port); // 调用PWM输出钩子
    dealta_us = get_time_us - get_time_us_last;

    static uint16_t printt_tick = 0;
    if (printt_tick++ > 10000)
    {
        printt_tick = 0;
        // DEBUG_INFO("velocity:%d(RPM)", (int32_t)foc_ctrl.pll_velocity_rpm);
        // DEBUG_INFO("run_foc_time:%d", dealta_us);
    }
}

/**
 * @brief FOC锁相环（PLL）运行函数
 * @param phase_q 输入相位（Q16.16格式）
 * @param dt_q 采样时间间隔（Q16.16格式）
 * @param phase_var_q 输出相位变量指针（Q16.16格式）
 * @param speed_var_q 输出速度变量指针（Q16.16格式）
 * @param kp_q 比例增益（Q16.16格式）
 * @param ki_q 积分增益（Q16.16格式）
 */
static void foc_pll_run(q16_16_t phase_q, q16_16_t dt_q, q16_16_t *phase_var_q, q16_16_t *speed_var_q, q16_16_t kp_q,
                        q16_16_t ki_q)
{
    // 计算相位误差并归一化到 [-π, π)
    q16_16_t delta_theta_q = q16_16_sub(phase_q, *phase_var_q);
    delta_theta_q = q16_16_normalize_angle(delta_theta_q);

    // 使用64位中间结果避免溢出
    // 比例项：kp * delta_theta（使用64位乘法避免溢出）
    int64_t kp_term_64 = ((int64_t)kp_q * (int64_t)delta_theta_q) >> 16;
    q16_16_t kp_term = (q16_16_t)kp_term_64;

    // 速度项：speed + kp_term（使用64位加法避免溢出）
    q16_16_t speed_term = q16_16_add(*speed_var_q, kp_term);

    // 相位增量：speed_term * dt（使用64位乘法）
    int64_t phase_inc_64 = ((int64_t)speed_term * (int64_t)dt_q) >> 16;
    q16_16_t phase_inc = (q16_16_t)phase_inc_64;

    // 更新相位变量并归一化到 [-π, π)
    *phase_var_q = q16_16_add(*phase_var_q, phase_inc);
    *phase_var_q = q16_16_normalize_angle(*phase_var_q);

    // 积分项：ki * delta_theta * dt（使用64位乘法避免溢出）
    int64_t ki_delta_64 = ((int64_t)ki_q * (int64_t)delta_theta_q) >> 16;
    int64_t speed_inc_64 = (ki_delta_64 * (int64_t)dt_q) >> 16;
    q16_16_t speed_inc = (q16_16_t)speed_inc_64;

    // 更新速度变量，添加饱和限制
    *speed_var_q = q16_16_add(*speed_var_q, speed_inc);

    // 速度饱和限制（防止积分饱和）
    const q16_16_t MAX_SPEED_Q = FLOAT_TO_Q16_16(1000.0f);  // 最大速度限制
    const q16_16_t MIN_SPEED_Q = FLOAT_TO_Q16_16(-1000.0f); // 最小速度限制

    if (*speed_var_q > MAX_SPEED_Q)
        *speed_var_q = MAX_SPEED_Q;
    else if (*speed_var_q < MIN_SPEED_Q)
        *speed_var_q = MIN_SPEED_Q;
}
