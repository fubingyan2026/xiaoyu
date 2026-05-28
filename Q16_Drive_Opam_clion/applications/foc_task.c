/**
 * @file    foc_task.c
 * @brief   FOC平台任务模块实现 - 硬件回调与平台初始化
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V1.0.0
 */

#include "foc_task.h"

#include "angle_sensor.h"
#include "bsp_delay.h"
#include "debug.h"
#include "encoder_alignment.h"
#include "foc_config.h"
#include "foc_port.h"
#include "main.h"

/* ==================== 硬件相关 ==================== */
#define ADC_INSTANCE hadc1
#define PWM_INSTANCE htim1
extern ADC_HandleTypeDef ADC_INSTANCE;
extern TIM_HandleTypeDef PWM_INSTANCE;

/* ==================== 全局变量 ==================== */

foc_context_t g_foc_ctx;

/* ==================== 硬件回调函数 ==================== */

static void foc_hw_adc_read(foc_context_t* ctx, q16_16_t* current_a, q16_16_t* current_b, q16_16_t* current_c)
{
    if (!ctx || !current_a || !current_b || !current_c) {
        return;
    }

    uint32_t adc_a = ctx->adc_dma_buffer[0];
    uint32_t adc_b = ctx->adc_dma_buffer[1];

    q16_16_t diff_a = INT_TO_Q16_16((int32_t)adc_a) - ctx->adc_offset[0];
    q16_16_t diff_b = INT_TO_Q16_16((int32_t)adc_b) - ctx->adc_offset[1];

    *current_a = q16_16_mul(diff_a, ctx->port.config.current_sample_factor_q);
    *current_b = q16_16_mul(diff_b, ctx->port.config.current_sample_factor_q);
    *current_c = -(*current_a + *current_b);
}

static void foc_hw_adc_init(foc_context_t* ctx)
{
    if (!ctx) {
        return;
    }

    __HAL_ADC_DISABLE_IT(&ADC_INSTANCE, ADC_IT_JEOC);
    HAL_ADCEx_InjectedStart(&ADC_INSTANCE);
    foc_task_adc_dma_start();

    uint32_t timeout = HAL_GetTick();
    const uint32_t adc_init_timeout_ms = 500;
    bool adc_data_valid = false;

    while ((HAL_GetTick() - timeout) < adc_init_timeout_ms) {
        if (ctx->adc_dma_buffer[0] != 0 && ctx->adc_dma_buffer[1] != 0 && ctx->adc_dma_buffer[2] != 0) {
            adc_data_valid = true;
            break;
        }
        delay_ms(1);
        foc_task_adc_dma_start();
    }

    if (!adc_data_valid) {
        ctx->adc_dma_buffer[0] = 2048;
        ctx->adc_dma_buffer[1] = 2048;
        ctx->adc_dma_buffer[2] = 2048;
    }

    ctx->adc_offset[0] = INT_TO_Q16_16((int32_t)ctx->adc_dma_buffer[0]);
    ctx->adc_offset[1] = INT_TO_Q16_16((int32_t)ctx->adc_dma_buffer[1]);
    ctx->adc_offset[2] = INT_TO_Q16_16((int32_t)ctx->adc_dma_buffer[2]);

    HAL_TIM_Base_Start_IT(&PWM_INSTANCE);
    HAL_TIM_PWM_Start_IT(&PWM_INSTANCE, TIM_CHANNEL_4);
}

static void foc_hw_pwm_output(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td)
{
    if (ta > PWM_PERIOD) ta = PWM_PERIOD;
    if (tb > PWM_PERIOD) tb = PWM_PERIOD;
    if (tc > PWM_PERIOD) tc = PWM_PERIOD;
    if (td > PWM_PERIOD) td = PWM_PERIOD;

    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_1, ta);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_2, tb);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_3, tc);
    __HAL_TIM_SET_COMPARE(&PWM_INSTANCE, TIM_CHANNEL_4, td + 5);
}

static void foc_hw_pwm_start(void)
{
    __HAL_TIM_SET_COUNTER(&PWM_INSTANCE, 0);

    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&PWM_INSTANCE, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&PWM_INSTANCE, TIM_CHANNEL_3);
}

static void foc_hw_pwm_stop(void)
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

static uint16_t foc_hw_encoder_read(foc_context_t* ctx)
{
    if (!ctx) {
        return 0;
    }
    return angle_sensor_get_raw_angle(&ctx->sensor);
}

static void foc_hw_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

/* ==================== 端口回调适配器 ==================== */

static void adc_read_adapter(q16_16_t* ia, q16_16_t* ib, q16_16_t* ic)
{
    foc_hw_adc_read(&g_foc_ctx, ia, ib, ic);
}

static void adc_init_adapter(void)
{
    foc_hw_adc_init(&g_foc_ctx);
}

static uint16_t encoder_read_adapter(void)
{
    return foc_hw_encoder_read(&g_foc_ctx);
}

/* ==================== 导出函数 ==================== */

void foc_task_adc_dma_start(void)
{
    HAL_ADC_Start_DMA(&ADC_INSTANCE, g_foc_ctx.adc_dma_buffer,
        sizeof(g_foc_ctx.adc_dma_buffer) / sizeof(g_foc_ctx.adc_dma_buffer[0]));
}

void foc_task_init(void)
{
    // 初始化角度传感器
    angle_sensor_init_context(&g_foc_ctx.sensor, SENSOR_TYPE_LINEAR_HALL);
    angle_sensor_init(&g_foc_ctx.sensor);

    // 加载编码器校准数据
    encoder_alignment_init();

    // 构建端口配置
    foc_port_config_t port_cfg = {
        .adc_read = adc_read_adapter,
        .adc_init = adc_init_adapter,
        .pwm_output = foc_hw_pwm_output,
        .pwm_start = foc_hw_pwm_start,
        .pwm_stop = foc_hw_pwm_stop,
        .encoder_read = encoder_read_adapter,
        .delay_ms = foc_hw_delay_ms,
        .pwm_period_counts = PWM_PERIOD,
        .current_sample_factor_q = CURRENT_SAMPLE_FACTOR_Q,
    };

    // 构建FOC配置
    foc_config_t foc_cfg = {
        .motor_poles = MOTOR_POLES,
        .pwm_period_s = FOC_PWM_PERIOD,
        .fsm_period_s = STATE_PERIOD,
        .id_kp = CURRENT_ID_KP,
        .id_ki = CURRENT_ID_KI,
        .id_out_max = CURRENT_ID_OUT_MAX,
        .id_integ_sat = CURRENT_ID_INTEG_SAT,
        .iq_kp = CURRENT_IQ_KP,
        .iq_ki = CURRENT_IQ_KI,
        .iq_out_max = CURRENT_IQ_OUT_MAX,
        .iq_integ_sat = CURRENT_IQ_INTEG_SAT,
        .pll_kp = PLL_ELE_KP,
        .pll_ki = PLL_ELE_KI,
        .pll_speed_limit = PLL_SPEED_LIMIT,
        .v_bus = V_BUS,
        .max_duty_ratio = 0.95f,
        .port_config = port_cfg,
        .sensor_type = SENSOR_TYPE_LINEAR_HALL,
        .flash_data = &g_motor_flash_cfg,
    };

    // 初始化FOC模块
    foc_error_t ret = foc_init(&g_foc_ctx, &foc_cfg);
    if (ret != FOC_OK) {
        DEBUG_LOGE("foc_task", "FOC初始化失败: %d", ret);
    }
}
