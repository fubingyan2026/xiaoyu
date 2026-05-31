/**
 * @file    foc.c
 * @brief   FOC 主模块实现
 *
 * 整合 FOC 核心算法、端口抽象层、状态机和 PI 控制器等子模块，
 * 提供完整的 FOC 控制接口。
 *
 * 主要功能：
 *   - FOC 模块的初始化和反初始化
 *   - 中断处理函数（foc_irq_handler）：编排编码器读取 → PLL → 电流环的执行流水线
 *   - 状态机管理和访问器接口
 *
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V4.0.0
 */

#include "foc.h"

#include <string.h>

#include "debug.h"
#include "foc_core.h"
#include "foc_hal.h"

/**
 * @brief 初始化 FOC 模块
 *
 * 完整初始化流程：
 *   1. 参数检查 → 2. 清零上下文 → 3. 保存配置 → 4. 初始化端口
 *   → 5. 初始化 ADC → 6. 初始化 D/Q 轴 PI 控制器 → 7. 初始化 SVPWM
 *   → 8. 初始化 FSM → 9. 设置 Flash 校准数据 → 10. 启动 PWM
 *
 * 初始化顺序依赖关系：
 *   - PI 控制器需要 pwm_period_s 计算 Ki 的预乘 dt
 *   - SVPWM 需要 v_bus 和 max_duty_ratio
 *   - FSM 依赖已初始化的端口
 *
 * @param ctx    FOC 上下文指针（由调用者分配，可为栈/堆/全局变量）
 * @param config FOC 配置指针（包含电机参数、PI 增益、端口配置等）
 * @return foc_error_t 错误码
 *   - FOC_OK：初始化成功
 *   - FOC_ERROR_NULL_PTR：ctx 或 config 为 NULL
 *   - FOC_ERROR_PORT_INIT_FAILED：端口初始化失败（回调缺失）
 *   - FOC_ERROR_FSM_INIT_FAILED：状态机初始化失败
 */
foc_error_t foc_init(foc_context_t* ctx, const foc_config_t* config)
{
    if (!ctx || !config) {
        return FOC_ERROR_NULL_PTR;
    }

    /* 如果已初始化，先执行反初始化以重置所有子模块 */
    if (ctx->initialized) {
        foc_deinit(ctx);
    }

    /* 清零整个上下文，确保所有状态从确定值开始 */
    memset(ctx, 0, sizeof(foc_context_t));

    /* 保存配置 */
    ctx->config = *config;

    /* 设置控制参数：默认使能 */
    ctx->sw = true;

    /* 初始化角度传感器（依赖 ctx->config.sensor_type） */
    angle_sensor_init_context(&ctx->sensor, ctx->config.sensor_type);
    angle_sensor_init(&ctx->sensor);

    /* 初始化端口（硬件抽象层），检查必要回调 */
    if (!foc_hal_init(&ctx->port, &ctx->config.port_config)) {
        return FOC_ERROR_PORT_INIT_FAILED;
    }

    /* 初始化 ADC 硬件 */
    foc_hal_adc_init(&ctx->port);

    /* 初始化 D 轴和 Q 轴 PI 控制器 */
    foc_pi_reset(&ctx->pi_id);
    foc_pi_reset(&ctx->pi_iq);

    q16_16_t id_kp_q = FLOAT_TO_Q16_16(ctx->config.id_kp);
    q16_16_t id_ki_q = FLOAT_TO_Q16_16(ctx->config.id_ki);
    q16_16_t id_out_max_q = FLOAT_TO_Q16_16(ctx->config.id_out_max);
    q16_16_t id_out_min_q = -id_out_max_q;
    q16_16_t id_integ_sat_q = FLOAT_TO_Q16_16(ctx->config.id_integ_sat);

    const q16_16_t pwm_dt_q = FLOAT_TO_Q16_16(ctx->config.pwm_period_s);

    foc_pi_init(&ctx->pi_id, id_kp_q, id_ki_q, id_out_max_q, id_out_min_q, id_integ_sat_q, pwm_dt_q);

    q16_16_t iq_kp_q = FLOAT_TO_Q16_16(ctx->config.iq_kp);
    q16_16_t iq_ki_q = FLOAT_TO_Q16_16(ctx->config.iq_ki);
    q16_16_t iq_out_max_q = FLOAT_TO_Q16_16(ctx->config.iq_out_max);
    q16_16_t iq_out_min_q = -iq_out_max_q;
    q16_16_t iq_integ_sat_q = FLOAT_TO_Q16_16(ctx->config.iq_integ_sat);

    foc_pi_init(&ctx->pi_iq, iq_kp_q, iq_ki_q, iq_out_max_q, iq_out_min_q, iq_integ_sat_q, pwm_dt_q);

    /* 初始化 SVPWM */
    foc_svpwm_config_t svpwm_config = {
        .v_bus = FLOAT_TO_Q16_16(ctx->config.v_bus),
        .max_duty_ratio = FLOAT_TO_Q16_16(ctx->config.max_duty_ratio),
    };
    foc_svpwm_init(&ctx->svpwm, &svpwm_config);

    /* 初始化 FOC 状态机 */
    if (foc_fsm_init(&ctx->fsm, ctx) != FOC_FSM_RET_OK) {
        foc_hal_deinit(&ctx->port);
        return FOC_ERROR_FSM_INIT_FAILED;
    }

    /* 初始化编码器校准（从已加载的 Flash 数据导入） */
    if (ctx->config.flash_data != NULL) {
        if (!foc_encoder_init(&ctx->encoder, ctx->config.flash_data->angle_map, ctx->config.flash_data->direction)) {
            /* 校准数据无效或未校准，首次启动属于正常情况 */
            DEBUG_LOGW("foc", "编码器校准数据无效，需执行校准流程");
        }
    }

    /* 启动 PWM 输出 */
    foc_hal_pwm_start(&ctx->port);

    /* 预计算常用 Q16.16 常量（避免运行时中断中重复 FLOAT_TO_Q16_16 调用） */
    ctx->pwm_period_q = FLOAT_TO_Q16_16(ctx->config.pwm_period_s);
    ctx->state_period_q = FLOAT_TO_Q16_16(ctx->config.fsm_period_s);
    ctx->pll_kp_q = FLOAT_TO_Q16_16(ctx->config.pll_kp);
    ctx->pll_ki_q = FLOAT_TO_Q16_16(ctx->config.pll_ki);
    ctx->pll_speed_limit_q = FLOAT_TO_Q16_16(ctx->config.pll_speed_limit);
    ctx->align_theta_q = FLOAT_TO_Q16_16(ALIGN_THETA);
    ctx->align_current_q = FLOAT_TO_Q16_16(ALIGN_CURRENT);
    ctx->if_startup_iq_q = FLOAT_TO_Q16_16(IF_STARTUP_IQ);
    ctx->if_startup_target_omega_q = FLOAT_TO_Q16_16(IF_STARTUP_OMEGA);
    ctx->if_startup_omega_acc_q = FLOAT_TO_Q16_16(IF_STARTUP_OMEGA_ACC);
    ctx->lpf_beta_q = FLOAT_TO_Q16_16(0.33f);

    ctx->initialized = true;

    return FOC_OK;
}

/**
 * @brief 反初始化 FOC 模块
 *
 * 关闭 PWM 输出、反初始化端口、清除初始化标志。
 * 如果 ctx 为 NULL，直接返回，不做任何操作。
 *
 * @param ctx FOC 上下文指针
 */
void foc_deinit(foc_context_t* ctx)
{
    if (!ctx) {
        return;
    }

    /* 停止 PWM 输出，防止意外驱动电机 */
    foc_hal_pwm_stop(&ctx->port);

    /* 反初始化端口 */
    foc_hal_deinit(&ctx->port);

    /* 清除初始化标志 */
    ctx->initialized = false;
}

/**
 * @brief 检查 FOC 模块是否已初始化
 *
 * @param ctx FOC 上下文指针
 * @return true  已初始化
 * @return false 未初始化或指针为空
 */
bool foc_is_initialized(const foc_context_t* ctx)
{
    return (ctx && ctx->initialized);
}

/**
 * @brief FOC 中断处理函数
 *
 * 在 ADC DMA 转换完成中断中调用，编排完整的 FOC 控制流水线：
 *   1. 读取编码器角度
 *   2. 在 RUN 状态下：通过编码器映射表获取电气角度 → 更新 PLL（角度 + 速度估计）
 *   3. 执行电流环算法：Clarke/Park 变换 → PI 控制 → 逆 Park → SVPWM → PWM 输出
 *
 * 注意：此函数在 PWM 中断上下文中执行（约 16.8kHz），
 * 必须保证执行时间小于 PWM 周期，不可包含阻塞调用。
 *
 * @param ctx FOC 上下文指针
 */
void foc_irq_handler(foc_context_t* ctx)
{
    if (!ctx || !ctx->initialized) {
        return;
    }

    /* 步骤1：读取编码器当前位置 */
    ctx->raw_angle = foc_hal_encoder_read(&ctx->port);
    /* 步骤2：更新电气角度和 PLL（仅在 RUN 状态下执行）
     * 在 ALIGN、ALIGNMENT 等状态下，电气角度由状态机直接控制 */
    if (fsm_current_state(&ctx->fsm.fsm) == FOC_FSM_STATE_RUN) {
        float electrical_angle = foc_encoder_track_sector(&ctx->encoder, ctx->raw_angle);
        ctx->electrical_angle_q = FLOAT_TO_Q16_16(electrical_angle);
        foc_core_pll_run(ctx->electrical_angle_q, ctx->pwm_period_q, &ctx->pll_phase_q,
            &ctx->pll_omega_q, ctx->pll_kp_q, ctx->pll_ki_q, ctx->pll_speed_limit_q);
        /* 将 rad/s 转换为 RPM：ω_rpm = ω_rad/s * 60 / (2π * 极对数) */
        ctx->pll_velocity_rpm = Q16_16_TO_FLOAT(ctx->pll_omega_q) * 60.0f / M_2PI / ctx->config.motor_poles;
    }
    /* 步骤3：执行电流环核心算法
     * sin/cos → Clarke/Park → PI → 逆 Park → SVPWM → PWM 输出 */
    foc_core_current_loop(ctx, ctx->electrical_angle_q);
}

/**
 * @brief 请求 FOC 状态机切换到指定状态
 *
 * @param ctx   FOC 上下文指针
 * @param state 目标状态值
 * @return foc_error_t 错误码
 *   - FOC_OK：切换成功
 *   - FOC_ERROR_UNINITIALIZED：未初始化
 *   - FOC_ERROR_GENERIC：状态不支持切换
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
 *
 * @param ctx FOC 上下文指针
 * @return 当前状态枚举值，未初始化时返回 FOC_FSM_STATE_IDLE
 */
foc_fsm_state_e foc_current_state(const foc_context_t* ctx)
{
    if (!ctx || !ctx->initialized) {
        return FOC_FSM_STATE_IDLE;
    }

    return (foc_fsm_state_e)ctx->fsm.fsm.current_state;
}

/* ==================== 访问器函数 ==================== */

/**
 * @brief 设置 Q 轴电流目标值
 * @param ctx FOC 上下文指针
 * @param iq  Q 轴电流目标值（Q16.16 格式，安培）
 */
void foc_set_target_iq(foc_context_t* ctx, q16_16_t iq)
{
    if (ctx) {
        ctx->target_iq_q = iq;
    }
}

/**
 * @brief 设置 D 轴电流目标值
 * @param ctx FOC 上下文指针
 * @param id  D 轴电流目标值（Q16.16 格式，安培）
 */
void foc_set_target_id(foc_context_t* ctx, q16_16_t id)
{
    if (ctx) {
        ctx->target_id_q = id;
    }
}

/**
 * @brief 设置 FOC 开关状态
 * @param ctx FOC 上下文指针
 * @param sw  true 开启 FOC 控制，false 关闭
 */
void foc_set_sw(foc_context_t* ctx, bool sw)
{
    if (ctx) {
        ctx->sw = sw;
    }
}

/**
 * @brief 设置角速度
 * @param ctx   FOC 上下文指针
 * @param omega 角速度值（Q16.16 格式，rad/s）
 */
void foc_set_omega(foc_context_t* ctx, q16_16_t omega)
{
    if (ctx) {
        ctx->target_omega_q = omega;
    }
}

/**
 * @brief 设置 PLL 输出相位
 * @param ctx   FOC 上下文指针
 * @param phase PLL 相位值（Q16.16 格式，弧度）
 */
void foc_set_pll_phase(foc_context_t* ctx, q16_16_t phase)
{
    if (ctx) {
        ctx->pll_phase_q = phase;
    }
}

/**
 * @brief 设置电气角度
 *
 * 在开环运行（对齐、IF 启动、校准）时由状态机直接设置电气角度。
 * 在闭环运行时由 PLL 或编码器映射表更新。
 *
 * @param ctx   FOC 上下文指针
 * @param angle 电气角度（Q16.16 格式，弧度）
 */
void foc_set_electrical_angle(foc_context_t* ctx, q16_16_t angle)
{
    if (ctx) {
        ctx->electrical_angle_q = angle;
    }
}

/**
 * @brief 获取 Q 轴电流目标值
 * @param ctx FOC 上下文指针
 * @return Q 轴电流目标值（Q16.16 格式），ctx 为空时返回 0
 */
q16_16_t foc_get_target_iq(const foc_context_t* ctx)
{
    return ctx ? ctx->target_iq_q : 0;
}

/**
 * @brief 获取 D 轴电流目标值
 * @param ctx FOC 上下文指针
 * @return D 轴电流目标值（Q16.16 格式），ctx 为空时返回 0
 */
q16_16_t foc_get_target_id(const foc_context_t* ctx)
{
    return ctx ? ctx->target_id_q : 0;
}

/**
 * @brief 获取角速度
 * @param ctx FOC 上下文指针
 * @return 角速度（Q16.16 格式，rad/s），ctx 为空时返回 0
 */
q16_16_t foc_get_omega(const foc_context_t* ctx)
{
    return ctx ? ctx->target_omega_q : 0;
}

/**
 * @brief 获取 PLL 输出相位
 * @param ctx FOC 上下文指针
 * @return PLL 相位值（Q16.16 格式，弧度），ctx 为空时返回 0
 */
q16_16_t foc_get_pll_phase(const foc_context_t* ctx)
{
    return ctx ? ctx->pll_phase_q : 0;
}

/**
 * @brief 获取电气角度
 * @param ctx FOC 上下文指针
 * @return 电气角度（Q16.16 格式，弧度），ctx 为空时返回 0
 */
q16_16_t foc_get_electrical_angle(const foc_context_t* ctx)
{
    return ctx ? ctx->electrical_angle_q : 0;
}

/**
 * @brief 获取 PLL 估计的速度（RPM）
 * @param ctx FOC 上下文指针
 * @return 速度值（RPM），ctx 为空时返回 0.0f
 */
float foc_get_velocity_rpm(const foc_context_t* ctx)
{
    return ctx ? ctx->pll_velocity_rpm : 0.0f;
}

/**
 * @brief 获取 FOC 开关状态
 * @param ctx FOC 上下文指针
 * @return true FOC 开启，false 关闭，ctx 为空时返回 false
 */
bool foc_get_sw(const foc_context_t* ctx)
{
    return ctx ? ctx->sw : false;
}

/**
 * @brief 获取编码器原始角度值
 * @param ctx FOC 上下文指针
 * @return 编码器原始计数值（0 ~ encoder_lines - 1），ctx 为空时返回 0
 */
uint16_t foc_get_raw_angle(const foc_context_t* ctx)
{
    return ctx ? ctx->raw_angle : 0;
}
