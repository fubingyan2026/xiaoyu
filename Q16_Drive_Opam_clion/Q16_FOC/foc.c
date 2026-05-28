/**
 * @file    foc.c
 * @brief   FOC主模块实现
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V4.0.0
 */

#include "foc.h"

#include <string.h>

#include "encoder_alignment.h"
#include "foc_core.h"
#include "foc_port.h"

/**
 * @brief 初始化FOC模块
 */
foc_error_t foc_init(foc_context_t* ctx, const foc_config_t* config)
{
    if (!ctx || !config) {
        return FOC_ERROR_NULL_PTR;
    }

    // 如果已初始化，先反初始化
    if (ctx->initialized) {
        foc_deinit(ctx);
    }

    // 清零整个上下文
    memset(ctx, 0, sizeof(foc_context_t));

    // 保存配置
    ctx->config = *config;

    // 设置控制参数
    ctx->sw = true;

    // 初始化端口
    if (!foc_port_init(&ctx->port, &ctx->config.port_config)) {
        return FOC_ERROR_PORT_INIT_FAILED;
    }

    // 初始化ADC
    foc_port_adc_init(&ctx->port);

    // 初始化PI控制器
    foc_pi_reset(&ctx->pi_id);
    foc_pi_reset(&ctx->pi_iq);

    q16_16_t id_kp_q = FLOAT_TO_Q16_16(ctx->config.id_kp);
    q16_16_t id_ki_q = FLOAT_TO_Q16_16(ctx->config.id_ki);
    q16_16_t id_out_max_q = FLOAT_TO_Q16_16(ctx->config.id_out_max);
    q16_16_t id_out_min_q = -id_out_max_q;
    q16_16_t id_integ_sat_q = FLOAT_TO_Q16_16(ctx->config.id_integ_sat);

    foc_pi_init(&ctx->pi_id, id_kp_q, id_ki_q, id_out_max_q, id_out_min_q, id_integ_sat_q);

    q16_16_t iq_kp_q = FLOAT_TO_Q16_16(ctx->config.iq_kp);
    q16_16_t iq_ki_q = FLOAT_TO_Q16_16(ctx->config.iq_ki);
    q16_16_t iq_out_max_q = FLOAT_TO_Q16_16(ctx->config.iq_out_max);
    q16_16_t iq_out_min_q = -iq_out_max_q;
    q16_16_t iq_integ_sat_q = FLOAT_TO_Q16_16(ctx->config.iq_integ_sat);

    foc_pi_init(&ctx->pi_iq, iq_kp_q, iq_ki_q, iq_out_max_q, iq_out_min_q, iq_integ_sat_q);

    // 初始化SVPWM
    foc_svpwm_config_t svpwm_config = {
        .v_bus = FLOAT_TO_Q16_16(ctx->config.v_bus),
        .max_duty_ratio = FLOAT_TO_Q16_16(ctx->config.max_duty_ratio),
    };
    foc_svpwm_init(&ctx->svpwm, &svpwm_config);

    // 初始化状态机
    if (foc_fsm_init(&ctx->fsm, ctx) != FOC_FSM_RET_OK) {
        foc_port_deinit(&ctx->port);
        return FOC_ERROR_FSM_INIT_FAILED;
    }

    // 设置Flash校准数据
    foc_fsm_set_flash_data(&ctx->fsm, ctx->config.flash_data);

    // 启动PWM
    foc_port_pwm_start(&ctx->port);

    ctx->initialized = true;

    return FOC_OK;
}

/**
 * @brief 反初始化FOC模块
 */
void foc_deinit(foc_context_t* ctx)
{
    if (!ctx) {
        return;
    }

    // 停止PWM
    foc_port_pwm_stop(&ctx->port);

    // 反初始化端口
    foc_port_deinit(&ctx->port);

    // 清除状态
    ctx->initialized = false;
}

/**
 * @brief 检查FOC模块是否已初始化
 */
bool foc_is_initialized(const foc_context_t* ctx)
{
    return (ctx && ctx->initialized);
}

/**
 * @brief FOC中断处理函数
 *
 * 编排完整的中断处理流程：编码器读取 → 电气角度更新/PLL → 电流环算法
 */
void foc_irq_handler(foc_context_t* ctx)
{
    if (!ctx || !ctx->initialized) {
        return;
    }

    // 1. 读取编码器角度
    ctx->raw_angle_q = foc_port_encoder_read(&ctx->port);

    // 2. 更新电气角度和PLL（仅RUN状态）
    if (fsm_current_state(&ctx->fsm.fsm) == FOC_FSM_STATE_RUN) {
        float electrical_angle = encoder_track_sector(ctx->raw_angle_q, &g_encoder_calib);
        ctx->electrical_angle_q = FLOAT_TO_Q16_16(electrical_angle);

        q16_16_t pll_kp_q = INT_TO_Q16_16((int32_t)ctx->config.pll_kp);
        q16_16_t pll_ki_q = INT_TO_Q16_16((int32_t)ctx->config.pll_ki);
        foc_core_pll_run(ctx->electrical_angle_q, FOC_PWM_PERIOD_Q, &ctx->pll_phase_q,
            &ctx->pll_velocity_q, pll_kp_q, pll_ki_q);
        ctx->pll_velocity_rpm = Q16_16_TO_FLOAT(ctx->pll_velocity_q) * 60.0f / M_2PI / ctx->config.motor_poles;
    }

    // 3. 执行纯电流环算法
    foc_core_current_loop(ctx, ctx->electrical_angle_q);
}

/**
 * @brief 请求状态切换
 */
foc_error_t foc_request_state(foc_context_t* ctx, foc_fsm_state_e state)
{
    if (!ctx || !ctx->initialized) {
        return FOC_ERROR_UNINITIALIZED;
    }

    foc_fsm_ret_e ret = foc_fsm_request_state(&ctx->fsm, state);
    if (ret != FOC_FSM_RET_OK) {
        return FOC_ERROR_GENERIC;
    }

    return FOC_OK;
}

/**
 * @brief 获取当前状态机状态
 */
foc_fsm_state_e foc_current_state(const foc_context_t* ctx)
{
    if (!ctx || !ctx->initialized) {
        return FOC_FSM_STATE_IDLE;
    }

    return (foc_fsm_state_e)ctx->fsm.fsm.current_state;
}

/* ==================== 访问器函数 ==================== */

void foc_set_target_iq(foc_context_t* ctx, q16_16_t iq)
{
    if (ctx) {
        ctx->target_iq_q = iq;
    }
}

void foc_set_target_id(foc_context_t* ctx, q16_16_t id)
{
    if (ctx) {
        ctx->target_id_q = id;
    }
}

void foc_set_sw(foc_context_t* ctx, bool sw)
{
    if (ctx) {
        ctx->sw = sw;
    }
}

void foc_set_omega(foc_context_t* ctx, q16_16_t omega)
{
    if (ctx) {
        ctx->omega_q = omega;
    }
}

void foc_set_pll_phase(foc_context_t* ctx, q16_16_t phase)
{
    if (ctx) {
        ctx->pll_phase_q = phase;
    }
}

void foc_set_electrical_angle(foc_context_t* ctx, q16_16_t angle)
{
    if (ctx) {
        ctx->electrical_angle_q = angle;
    }
}

q16_16_t foc_get_target_iq(const foc_context_t* ctx)
{
    return ctx ? ctx->target_iq_q : 0;
}

q16_16_t foc_get_target_id(const foc_context_t* ctx)
{
    return ctx ? ctx->target_id_q : 0;
}

q16_16_t foc_get_omega(const foc_context_t* ctx)
{
    return ctx ? ctx->omega_q : 0;
}

q16_16_t foc_get_pll_phase(const foc_context_t* ctx)
{
    return ctx ? ctx->pll_phase_q : 0;
}

q16_16_t foc_get_electrical_angle(const foc_context_t* ctx)
{
    return ctx ? ctx->electrical_angle_q : 0;
}

float foc_get_velocity_rpm(const foc_context_t* ctx)
{
    return ctx ? ctx->pll_velocity_rpm : 0.0f;
}

bool foc_get_sw(const foc_context_t* ctx)
{
    return ctx ? ctx->sw : false;
}

uint16_t foc_get_raw_angle(const foc_context_t* ctx)
{
    return ctx ? ctx->raw_angle_q : 0;
}
