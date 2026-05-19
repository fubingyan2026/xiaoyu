/**
 * @file    foc_sm.c
 * @brief   FOC State Machine - 使用通用FSM框架重构
 * @author  FOC Development Team
 * @date    2026-02-04
 *
 * @description
 * 基于通用FSM框架(fsm.h)重构的FOC状态机实现。
 * 保持原有API完全兼容，同时利用通用框架的：
 * - 邻接矩阵O(1)转换验证
 * - 进入/退出回调机制
 * - 调试状态名称映射
 */

#include "foc_sm.h"
#include "debug.h"
#include "encoder_alignment.h"
#include "flash_task.h"
#include "foc_port.h"
#include "hall_adjustment.h"
#include "stdlib.h"

static foc_sm_context_t g_foc_sm = {0};

/* 用于调试的状态名称 */
static const char *foc_state_names[] = {"IDLE", "SELF_CHECK", "ALIGN", "ALIGNMENT", "RUN", "HALL", "STOP"};

/**
 * @brief FOC状态进入逻辑（提取为独立函数供适配器调用）
 */
static void foc_entry_logic(foc_sm_context_t *ctx, foc_sm_state_e state)
{
    switch (state)
    {
    case FOC_SM_STATE_ALIGN:
        ctx->ctrl->target_id_q = 0;
        ctx->ctrl->target_iq_q = 0;
        ctx->ctrl->electrical_angle_q = q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f));

        break;

    case FOC_SM_STATE_ALIGNMENT:
        ctx->cali_ctx.capture_idx = 0;
        ctx->cali_ctx.step = FOC_CALI_STEP_FORWARD;
        ctx->cali_ctx.timeout_cnt = 0;
        ctx->cali_ctx.last_angle_q = 0;
        ctx->ctrl->target_iq_q = IF_STARTUP_IQ_Q;
        ctx->ctrl->omega_q = 0;
        ctx->ctrl->electrical_angle_q = q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f));
        break;

    case FOC_SM_STATE_STOP:
        foc_port_pwm_stop(foc_port_get_instance());
        break;

    default:
        break;
    }

    DEBUG_LOGD("foc_sm", "状态机进入:%s", foc_sm_state_to_string(state));
}

/**
 * @brief 框架回调适配器：进入回调
 */
static void fsm_entry_adapter(fsm_t *ctx, fsm_state_t state)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;

    /* 先执行FOC内部进入逻辑 */
    foc_entry_logic(foc_ctx, (foc_sm_state_e)state);

    /* 再调用用户回调（如果设置了） */
    if (foc_ctx->on_entry != NULL)
    {
        foc_ctx->on_entry(foc_ctx);
    }
}

/**
 * @brief 框架回调适配器：退出回调
 */
static void fsm_exit_adapter(fsm_t *ctx, fsm_state_t state)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;

    DEBUG_LOGD("foc_sm", "状态机退出:%s", foc_sm_state_to_string(state));

    /* 调用用户退出回调（如果设置了） */
    if (foc_ctx->on_exit != NULL)
    {
        foc_ctx->on_exit(foc_ctx);
    }
}

/**
 * @brief 周期平均函数
 */
static int32_t cycle_average(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub_data = a - b;
    int32_t ave_data = (a + b) >> 1;

    if (abs(sub_data) > (cyc >> 1))
    {
        if (ave_data >= (cyc >> 1))
        {
            ave_data -= (cyc >> 1);
        }
        else
        {
            ave_data += (cyc >> 1);
        }
    }
    return ave_data;
}

/**
 * @brief 空闲状态处理函数
 */
static fsm_state_t handler_idle(fsm_t *ctx)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;

    if (foc_ctx->ctrl->sw)
    {
        foc_port_pwm_start(foc_port_get_instance());
        return FOC_SM_STATE_RUN;
    }
    return FOC_SM_STATE_IDLE;
}

/**
 * @brief 对齐状态处理函数
 */
static fsm_state_t handler_align(fsm_t *ctx)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;

    if (foc_ctx->ctrl->target_iq_q < ALIGN_CURRENT_Q)
    {
        foc_ctx->ctrl->target_iq_q =
            q16_16_add(foc_ctx->ctrl->target_iq_q, q16_16_mul(ALIGN_CURRENT_Q, STATE_PERIOD_Q));
    }
    else
    {
        static uint16_t align_cnt = 0;
        if (align_cnt++ >= FOC_SM_ALIGN_TIMEOUT_CNT)
        {
            align_cnt = 0;
            foc_ctx->ctrl->electrical_angle_q = q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f));
            return FOC_SM_STATE_ALIGNMENT;
        }
    }
    return FOC_SM_STATE_ALIGN;
}

/**
 * @brief 校准对齐状态处理函数
 */
static fsm_state_t handler_alignment(fsm_t *ctx)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;
    foc_ctrl_t *ctrl = foc_ctx->ctrl;
    motor_flash_config_t *flash_data = foc_ctx->flash_data;
    q16_16_t threshold = q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(0.5f));

    /* 速度爬升 */
    if (ctrl->omega_q < IF_STARTUP_OMEGA_Q)
    {
        ctrl->omega_q = q16_16_add(ctrl->omega_q, IF_STARTUP_OMEGA_ACC_Q);
    }
    else
    {
        ctrl->omega_q = IF_STARTUP_OMEGA_Q;
    }

    ctrl->target_iq_q = IF_STARTUP_IQ_Q;

    switch (foc_ctx->cali_ctx.step)
    {
    case FOC_CALI_STEP_FORWARD: {
        if (ctrl->electrical_angle_q < foc_ctx->cali_ctx.last_angle_q)
        {
            ctrl->electrical_angle_q = q16_16_add(ctrl->electrical_angle_q, ctrl->omega_q);
        }
        else
        {
            ctrl->electrical_angle_q = foc_ctx->cali_ctx.last_angle_q;

            if (foc_ctx->cali_ctx.timeout_cnt++ >= FOC_SM_ELEC_ANGLE_STABLE_TIME)
            {
                foc_ctx->cali_ctx.timeout_cnt = 0;
                foc_ctx->cali_ctx.last_angle_q = q16_16_add(foc_ctx->cali_ctx.last_angle_q, ALIGN_THETA_Q);

                if ((ctrl->electrical_angle_q >= threshold) && (foc_ctx->cali_ctx.capture_idx >= 0) &&
                    (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps) && (flash_data != NULL))
                {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] = ctrl->raw_angle_q;
                    DEBUG_LOGI("foc_sm", "正向校准数据缓存到flash:%d", foc_ctx->cali_ctx.capture_idx);
                }

                if (ctrl->electrical_angle_q >= threshold)
                {
                    foc_ctx->cali_ctx.capture_idx++;
                }

                if (foc_ctx->cali_ctx.capture_idx >= (int16_t)(g_encoder_calib.total_steps + FOC_SM_CALI_STEPS_EXTRA))
                {
                    // 切换到反向前，先减速到零
                    foc_ctx->cali_ctx.step = FOC_CALI_STEP_TRANSITION;
                    foc_ctx->cali_ctx.transition_cnt = 0;
                    return FOC_SM_STATE_ALIGNMENT;
                }
            }
        }
        break;
    }

    case FOC_CALI_STEP_TRANSITION: {
        // 过渡状态：平滑减速到零，同时减小电流
        if (foc_ctx->cali_ctx.transition_cnt < FOC_SM_TRANSITION_STEPS)
        {
            // 计算减速因子（从1线性减小到0）
            q16_16_t transition_ratio =
                q16_16_sub(Q16_16_ONE, q16_16_div(INT_TO_Q16_16(foc_ctx->cali_ctx.transition_cnt),
                                                  INT_TO_Q16_16(FOC_SM_TRANSITION_STEPS)));

            // 1. 减速：速度线性减小
            q16_16_t current_omega = q16_16_mul(ctrl->omega_q, transition_ratio);
            ctrl->electrical_angle_q = q16_16_add(ctrl->electrical_angle_q, current_omega);

            // 2. 减电流：电流与速度同步减小，避免制动转矩
            //            ctrl->target_iq_q = q16_16_mul(IF_STARTUP_IQ_Q, transition_ratio);

            foc_ctx->cali_ctx.transition_cnt++;
        }
        else
        {
            // 减速完成，速度接近零，电流接近零
            // 短暂停顿，确保完全停止
            if (foc_ctx->cali_ctx.transition_cnt < FOC_SM_TRANSITION_STEPS + FOC_SM_STOP_TIME)
            {
                // 保持零速和零电流
                foc_ctx->cali_ctx.transition_cnt++;
            }
            else
            {
                // 完全停止后，切换到反向状态
                foc_ctx->cali_ctx.step = FOC_CALI_STEP_REVERSE;
                foc_ctx->cali_ctx.transition_cnt = 0;
                // 恢复反向电流
                ctrl->target_iq_q = IF_STARTUP_IQ_Q;
            }
        }
        break;
    }

    case FOC_CALI_STEP_REVERSE: {
        if (ctrl->electrical_angle_q > foc_ctx->cali_ctx.last_angle_q)
        {
            ctrl->electrical_angle_q = q16_16_sub(ctrl->electrical_angle_q, ctrl->omega_q);
        }
        else
        {
            ctrl->electrical_angle_q = foc_ctx->cali_ctx.last_angle_q;

            if (foc_ctx->cali_ctx.timeout_cnt++ >= FOC_SM_ELEC_ANGLE_STABLE_TIME)
            {
                foc_ctx->cali_ctx.timeout_cnt = 0;
                foc_ctx->cali_ctx.capture_idx--;

                if ((ctrl->electrical_angle_q >= threshold) && (foc_ctx->cali_ctx.capture_idx >= 0) &&
                    (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps) && (flash_data != NULL))
                {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] =
                        cycle_average(flash_data->angle_map[foc_ctx->cali_ctx.capture_idx], ctrl->raw_angle_q,
                                      g_encoder_calib.encoder_lines);
                    DEBUG_LOGI("foc_sm", "反向平均数据缓存到flash:%d", foc_ctx->cali_ctx.capture_idx);
                }

                if (foc_ctx->cali_ctx.capture_idx <= -(int16_t)FOC_SM_CALI_STEPS_EXTRA)
                {
                    foc_ctx->cali_ctx.step = FOC_CALI_STEP_COMPLETE;
                }
                else
                {
                    foc_ctx->cali_ctx.last_angle_q = q16_16_sub(foc_ctx->cali_ctx.last_angle_q, ALIGN_THETA_Q);
                }
            }
        }
        break;
    }

    case FOC_CALI_STEP_COMPLETE: {
        if (flash_data != NULL)
        {
            foc_ctx->cali_ctx.capture_idx = 0;

            encoder_detect_direction(flash_data->angle_map, &g_encoder_calib);
            flash_data->direction = (int16_t)g_encoder_calib.direction;

            if (g_encoder_calib.direction == MOTOR_DIR_REVERSE)
            {
                while (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps)
                {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] =
                        g_encoder_calib.encoder_lines - flash_data->angle_map[foc_ctx->cali_ctx.capture_idx];
                    foc_ctx->cali_ctx.capture_idx++;
                }
            }

            DEBUG_LOGI("foc_sm", "编码器方向:%d", g_encoder_calib.direction);
            encoder_detect_direction(flash_data->angle_map, &g_encoder_calib); // 检查电机方向
            g_encoder_calib.direction = flash_data->direction;                 // 设置电机方向
            /* 延迟写入Flash */
            flash_task_request(FLASH_TASK_WRITE_ENCODER, flash_data, sizeof(g_motor_flash_cfg));
        }

        ctrl->target_id_q = 0;
        return FOC_SM_STATE_RUN;
    }

    default:
        break;
    }

    return FOC_SM_STATE_ALIGNMENT;
}

/**
 * @brief 运行状态处理函数
 */
static fsm_state_t handler_run(fsm_t *ctx)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;

    foc_ctx->ctrl->target_id_q = 0;
    foc_ctx->ctrl->target_iq_q = FLOAT_TO_Q16_16(0.25f);

    return FOC_SM_STATE_RUN;
}

/**
 * @brief 霍尔传感器状态处理函数
 */
static fsm_state_t handler_hall(fsm_t *ctx)
{
    foc_sm_context_t *foc_ctx = (foc_sm_context_t *)ctx;
    hall_adjust_t *hall = &hall_adjust;

    hall->hall_adjust_task();
    foc_ctx->ctrl->target_iq_q = FLOAT_TO_Q16_16(hall->motorTargetCurrent);
    foc_ctx->ctrl->electrical_angle_q = FLOAT_TO_Q16_16(hall->motorTargetElectricalAngle);

    if (hall->hall_adjust_state == ADJUST_DONE)
    {
        return FOC_SM_STATE_ALIGN;
    }

    return FOC_SM_STATE_HALL;
}

/**
 * @brief 停止状态处理函数
 */
static fsm_state_t handler_stop(fsm_t *ctx)
{
    (void)ctx;
    return FOC_SM_STATE_STOP;
}

/**
 * @brief FOC状态机初始化函数
 */
foc_sm_ret_e foc_sm_init(foc_sm_context_t *ctx, foc_ctrl_t *ctrl)
{
    if ((ctx == NULL) || (ctrl == NULL))
    {
        return FOC_SM_RET_ERROR;
    }

    /* 初始化FOC特定字段 */
    memset(ctx, 0, sizeof(foc_sm_context_t));
    ctx->ctrl = ctrl;
    ctx->flash_data = NULL;
    ctx->on_entry = NULL;
    ctx->on_exit = NULL;

    /* 构建FSM配置 */
    static fsm_handler_t handlers[FOC_SM_STATE_COUNT];
    static fsm_guard_t transitions[FOC_SM_STATE_COUNT * FOC_SM_STATE_COUNT];
    memset(handlers, 0, sizeof(handlers));
    memset(transitions, 0, sizeof(transitions));

    handlers[FOC_SM_STATE_IDLE] = handler_idle;
    handlers[FOC_SM_STATE_ALIGN] = handler_align;
    handlers[FOC_SM_STATE_ALIGNMENT] = handler_alignment;
    handlers[FOC_SM_STATE_RUN] = handler_run;
    handlers[FOC_SM_STATE_HALL] = handler_hall;
    handlers[FOC_SM_STATE_STOP] = handler_stop;

    fsm_config_t config = {
        .handlers = handlers,
        .transitions = transitions,
        .state_count = FOC_SM_STATE_COUNT,
        .entry_cb = fsm_entry_adapter,
        .exit_cb = fsm_exit_adapter,
        .state_names = foc_state_names,
        .user_data = ctx,
    };
    fsm_fill(&config, fsm_always_true);

    fsm_err_t ret = fsm_init(&ctx->fsm, FOC_SM_STATE_IDLE, &config);
    if (ret != FSM_OK)
    {
        return FOC_SM_RET_ERROR;
    }

    /* 初始化FOC特定字段 */
    ctx->cali_ctx.capture_idx = 0;
    ctx->cali_ctx.step = FOC_CALI_STEP_FORWARD;
    ctx->cali_ctx.timeout_cnt = 0;
    ctx->cali_ctx.last_angle_q = 0;

    return FOC_SM_RET_OK;
}

/**
 * @brief FOC状态机步进函数
 */
foc_sm_ret_e foc_sm_step(foc_sm_context_t *ctx)
{
    if (ctx == NULL)
    {
        return FOC_SM_RET_ERROR;
    }

    /* 使用通用FSM框架步进 */
    fsm_err_t ret = fsm_step(&ctx->fsm);
    if (ret != FSM_OK)
    {
        return FOC_SM_RET_ERROR;
    }

    return FOC_SM_RET_OK;
}

/**
 * @brief 请求FOC状态机切换到指定状态
 */
foc_sm_ret_e foc_sm_request_state(foc_sm_context_t *ctx, foc_sm_state_e state)
{
    if (ctx == NULL)
    {
        return FOC_SM_RET_ERROR;
    }

    /* 使用通用FSM框架请求转换 */
    fsm_err_t ret = fsm_goto(&ctx->fsm, state);
    if (ret == FSM_OK)
    {
        DEBUG_LOGD("foc_sm", "状态机设置:%s成功", foc_sm_state_to_string(state));
        return FOC_SM_RET_OK;
    }
    else
    {
        DEBUG_LOGE("foc_sm", "状态机设置:%s失败", foc_sm_state_to_string(state));
        return FOC_SM_RET_INVALID_STATE;
    }
}

/**
 * @brief 将FOC状态枚举转换为字符串
 */
const char *foc_sm_state_to_string(foc_sm_state_e state)
{
    if (state < FOC_SM_STATE_COUNT)
    {
        return foc_state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief 获取FOC状态机全局实例
 */
foc_sm_context_t *foc_sm_get_instance(void)
{
    return &g_foc_sm;
}

/**
 * @brief 获取当前状态
 */
foc_sm_state_e foc_sm_current_state(void)
{
    return g_foc_sm.fsm.current_state;
}

/**
 * @brief FOC状态机计算函数（包装器）
 */
void foc_sm_calc(void)
{
    foc_sm_step(&g_foc_sm);
}

/**
 * @brief 设置Flash数据指针
 */
void foc_sm_set_flash_data(foc_sm_context_t *ctx, motor_flash_config_t *flash_data)
{
    if (ctx != NULL)
    {
        ctx->flash_data = flash_data;
    }
}

/**
 * @brief 获取Flash数据指针
 */
motor_flash_config_t *foc_sm_get_flash_data(foc_sm_context_t *ctx)
{
    return (ctx != NULL) ? ctx->flash_data : NULL;
}

/**
 * @brief 设置进入回调
 */
void foc_sm_set_on_entry(foc_sm_context_t *ctx, foc_sm_on_entry_t on_entry)
{
    if (ctx != NULL)
    {
        ctx->on_entry = on_entry;
    }
}

/**
 * @brief 设置退出回调
 */
void foc_sm_set_on_exit(foc_sm_context_t *ctx, foc_sm_on_exit_t on_exit)
{
    if (ctx != NULL)
    {
        ctx->on_exit = on_exit;
    }
}