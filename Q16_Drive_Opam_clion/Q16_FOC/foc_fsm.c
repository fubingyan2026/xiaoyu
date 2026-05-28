/**
 * @file    foc_fsm.c
 * @brief   FOC State Machine - 使用通用FSM框架
 * @author  FOC Development Team
 * @date    2026-02-04
 */

#include "foc_fsm.h"

#include <string.h>

#include "debug.h"
#include "encoder_alignment.h"
#include "flash_task.h"
#include "foc.h"
#include "foc_port.h"
#include "fsm_linear_hall.h"
#include "stdlib.h"

static const char* foc_state_names[] = { "IDLE", "ALIGN", "ALIGNMENT", "RUN", "HALL", "STOP" };

/**
 * @brief 框架回调适配器：进入回调
 */
static void fsm_entry_adapter(fsm_t* fsm_ctx, fsm_state_t state)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    switch (state) {
    case FOC_FSM_STATE_ALIGN:
        foc_set_target_id(foc, 0);
        foc_set_target_iq(foc, 0);
        foc_set_electrical_angle(foc, q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f)));
        break;

    case FOC_FSM_STATE_ALIGNMENT:
        foc_ctx->cali_ctx.capture_idx = 0;
        foc_ctx->cali_ctx.step = FOC_CALI_STEP_FORWARD;
        foc_ctx->cali_ctx.timeout_cnt = 0;
        foc_ctx->cali_ctx.last_angle_q = 0;
        foc_set_target_iq(foc, IF_STARTUP_IQ_Q);
        foc_set_omega(foc, 0);
        foc_set_electrical_angle(foc, q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f)));
        break;

    case FOC_FSM_STATE_STOP:
        foc_port_pwm_stop(&foc->port);
        break;

    default:
        break;
    }

    DEBUG_LOGD("foc_fsm", "状态机进入:%s", foc_fsm_state_to_string(state));
}

/**
 * @brief 框架回调适配器：退出回调
 */
static void fsm_exit_adapter(fsm_t* fsm_ctx, fsm_state_t state)
{
    (void)fsm_ctx;
    DEBUG_LOGD("foc_fsm", "状态机退出:%s", foc_fsm_state_to_string(state));
}

static int32_t cycle_average(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub_data = a - b;
    int32_t ave_data = (a + b) >> 1;

    if (abs(sub_data) > (cyc >> 1)) {
        if (ave_data >= (cyc >> 1)) {
            ave_data -= (cyc >> 1);
        } else {
            ave_data += (cyc >> 1);
        }
    }
    return ave_data;
}

static fsm_state_t handler_idle(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    if (foc_get_sw(foc)) {
        foc_port_pwm_start(&foc->port);
        return FOC_FSM_STATE_RUN;
    }
    return FOC_FSM_STATE_IDLE;
}

static fsm_state_t handler_align(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    if (foc_get_target_iq(foc) < ALIGN_CURRENT_Q) {
        foc_set_target_iq(foc, q16_16_add(foc_get_target_iq(foc), q16_16_mul(ALIGN_CURRENT_Q, STATE_PERIOD_Q)));
    } else {
        static uint16_t align_cnt = 0;
        if (align_cnt++ >= FOC_FSM_ALIGN_TIMEOUT_CNT) {
            align_cnt = 0;
            foc_set_electrical_angle(foc, q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(-4.0f)));
            return FOC_FSM_STATE_ALIGNMENT;
        }
    }
    return FOC_FSM_STATE_ALIGN;
}

static fsm_state_t handler_alignment(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;
    motor_flash_config_t* flash_data = (motor_flash_config_t*)foc_ctx->flash_data;
    q16_16_t threshold = q16_16_mul(ALIGN_THETA_Q, FLOAT_TO_Q16_16(0.5f));

    if (foc_get_omega(foc) < IF_STARTUP_OMEGA_Q) {
        foc_set_omega(foc, q16_16_add(foc_get_omega(foc), IF_STARTUP_OMEGA_ACC_Q));
    } else {
        foc_set_omega(foc, IF_STARTUP_OMEGA_Q);
    }

    foc_set_target_iq(foc, IF_STARTUP_IQ_Q);

    switch (foc_ctx->cali_ctx.step) {
    case FOC_CALI_STEP_FORWARD: {
        if (foc_get_electrical_angle(foc) < foc_ctx->cali_ctx.last_angle_q) {
            foc_set_electrical_angle(foc, q16_16_add(foc_get_electrical_angle(foc), foc_get_omega(foc)));
        } else {
            foc_set_electrical_angle(foc, foc_ctx->cali_ctx.last_angle_q);

            if (foc_ctx->cali_ctx.timeout_cnt++ >= FOC_FSM_ELEC_ANGLE_STABLE_TIME) {
                foc_ctx->cali_ctx.timeout_cnt = 0;
                foc_ctx->cali_ctx.last_angle_q = q16_16_add(foc_ctx->cali_ctx.last_angle_q, ALIGN_THETA_Q);

                if ((foc_get_electrical_angle(foc) >= threshold) && (foc_ctx->cali_ctx.capture_idx >= 0) && (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps) && (flash_data != NULL)) {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] = foc_get_raw_angle(foc);
                    DEBUG_LOGI("foc_fsm", "正向校准数据缓存到flash:%d", foc_ctx->cali_ctx.capture_idx);
                }

                if (foc_get_electrical_angle(foc) >= threshold) {
                    foc_ctx->cali_ctx.capture_idx++;
                }

                if (foc_ctx->cali_ctx.capture_idx >= (int16_t)(g_encoder_calib.total_steps + FOC_FSM_CALI_STEPS_EXTRA)) {
                    foc_ctx->cali_ctx.step = FOC_CALI_STEP_TRANSITION;
                    foc_ctx->cali_ctx.transition_cnt = 0;
                    return FOC_FSM_STATE_ALIGNMENT;
                }
            }
        }
        break;
    }

    case FOC_CALI_STEP_TRANSITION: {
        if (foc_ctx->cali_ctx.transition_cnt < FOC_FSM_TRANSITION_STEPS) {
            q16_16_t transition_ratio = q16_16_sub(Q16_16_ONE, q16_16_div(INT_TO_Q16_16(foc_ctx->cali_ctx.transition_cnt), INT_TO_Q16_16(FOC_FSM_TRANSITION_STEPS)));
            q16_16_t current_omega = q16_16_mul(foc_get_omega(foc), transition_ratio);
            foc_set_electrical_angle(foc, q16_16_add(foc_get_electrical_angle(foc), current_omega));
            foc_ctx->cali_ctx.transition_cnt++;
        } else {
            if (foc_ctx->cali_ctx.transition_cnt < FOC_FSM_TRANSITION_STEPS + FOC_FSM_STOP_TIME) {
                foc_ctx->cali_ctx.transition_cnt++;
            } else {
                foc_ctx->cali_ctx.step = FOC_CALI_STEP_REVERSE;
                foc_ctx->cali_ctx.transition_cnt = 0;
                foc_set_target_iq(foc, IF_STARTUP_IQ_Q);
            }
        }
        break;
    }

    case FOC_CALI_STEP_REVERSE: {
        if (foc_get_electrical_angle(foc) > foc_ctx->cali_ctx.last_angle_q) {
            foc_set_electrical_angle(foc, q16_16_sub(foc_get_electrical_angle(foc), foc_get_omega(foc)));
        } else {
            foc_set_electrical_angle(foc, foc_ctx->cali_ctx.last_angle_q);

            if (foc_ctx->cali_ctx.timeout_cnt++ >= FOC_FSM_ELEC_ANGLE_STABLE_TIME) {
                foc_ctx->cali_ctx.timeout_cnt = 0;
                foc_ctx->cali_ctx.capture_idx--;

                if ((foc_get_electrical_angle(foc) >= threshold) && (foc_ctx->cali_ctx.capture_idx >= 0) && (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps) && (flash_data != NULL)) {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] = cycle_average(flash_data->angle_map[foc_ctx->cali_ctx.capture_idx], foc_get_raw_angle(foc),
                        g_encoder_calib.encoder_lines);
                    DEBUG_LOGI("foc_fsm", "反向平均数据缓存到flash:%d", foc_ctx->cali_ctx.capture_idx);
                }

                if (foc_ctx->cali_ctx.capture_idx <= -(int16_t)FOC_FSM_CALI_STEPS_EXTRA) {
                    foc_ctx->cali_ctx.step = FOC_CALI_STEP_COMPLETE;
                } else {
                    foc_ctx->cali_ctx.last_angle_q = q16_16_sub(foc_ctx->cali_ctx.last_angle_q, ALIGN_THETA_Q);
                }
            }
        }
        break;
    }

    case FOC_CALI_STEP_COMPLETE: {
        if (flash_data != NULL) {
            foc_ctx->cali_ctx.capture_idx = 0;

            encoder_detect_direction(flash_data->angle_map, &g_encoder_calib);
            flash_data->direction = (int16_t)g_encoder_calib.direction;

            if (g_encoder_calib.direction == MOTOR_DIR_REVERSE) {
                while (foc_ctx->cali_ctx.capture_idx <= (int16_t)g_encoder_calib.total_steps) {
                    flash_data->angle_map[foc_ctx->cali_ctx.capture_idx] = g_encoder_calib.encoder_lines - flash_data->angle_map[foc_ctx->cali_ctx.capture_idx];
                    foc_ctx->cali_ctx.capture_idx++;
                }
            }

            DEBUG_LOGI("foc_fsm", "编码器方向:%d", g_encoder_calib.direction);
            encoder_detect_direction(flash_data->angle_map, &g_encoder_calib);
            g_encoder_calib.direction = flash_data->direction;
            flash_task_request(FLASH_TASK_WRITE_ANGLE, flash_data, sizeof(g_motor_flash_cfg));
        }

        foc_set_target_id(foc, 0);
        return FOC_FSM_STATE_RUN;
    }

    default:
        break;
    }

    return FOC_FSM_STATE_ALIGNMENT;
}

static fsm_state_t handler_run(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    foc_set_target_id(foc, 0);
    foc_set_target_iq(foc, FLOAT_TO_Q16_16(0.25f));

    return FOC_FSM_STATE_RUN;
}

static fsm_state_t handler_hall(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    fsm_linear_hall_task();
    foc_set_target_iq(foc, FLOAT_TO_Q16_16(fsm_linear_hall_get_current()));
    foc_set_electrical_angle(foc, FLOAT_TO_Q16_16(fsm_linear_hall_get_elec_angle()));

    if (fsm_linear_hall_is_done()) {
        return FOC_FSM_STATE_ALIGN;
    }

    return FOC_FSM_STATE_HALL;
}

static fsm_state_t handler_stop(fsm_t* fsm_ctx)
{
    (void)fsm_ctx;
    return FOC_FSM_STATE_STOP;
}

foc_fsm_ret_e foc_fsm_init(foc_fsm_context_t* ctx, void* parent)
{
    if ((ctx == NULL) || (parent == NULL)) {
        return FOC_FSM_RET_ERROR;
    }

    memset(ctx, 0, sizeof(foc_fsm_context_t));
    ctx->parent = parent;
    ctx->flash_data = NULL;

    static fsm_handler_t handlers[FOC_FSM_STATE_COUNT];
    static fsm_guard_t transitions[FOC_FSM_STATE_COUNT * FOC_FSM_STATE_COUNT];
    memset(handlers, 0, sizeof(handlers));
    memset(transitions, 0, sizeof(transitions));

    handlers[FOC_FSM_STATE_IDLE] = handler_idle;
    handlers[FOC_FSM_STATE_ALIGN] = handler_align;
    handlers[FOC_FSM_STATE_ALIGNMENT] = handler_alignment;
    handlers[FOC_FSM_STATE_RUN] = handler_run;
    handlers[FOC_FSM_STATE_HALL] = handler_hall;
    handlers[FOC_FSM_STATE_STOP] = handler_stop;

    fsm_config_t config = {
        .handlers = handlers,
        .transitions = transitions,
        .state_count = FOC_FSM_STATE_COUNT,
        .entry_cb = fsm_entry_adapter,
        .exit_cb = fsm_exit_adapter,
        .state_names = foc_state_names,
        .user_data = ctx,
    };
    fsm_fill(&config, fsm_always_true);

    fsm_err_t ret = fsm_init(&ctx->fsm, FOC_FSM_STATE_IDLE, &config);
    if (ret != FSM_OK) {
        return FOC_FSM_RET_ERROR;
    }

    ctx->cali_ctx.capture_idx = 0;
    ctx->cali_ctx.step = FOC_CALI_STEP_FORWARD;
    ctx->cali_ctx.timeout_cnt = 0;
    ctx->cali_ctx.last_angle_q = 0;

    return FOC_FSM_RET_OK;
}

foc_fsm_ret_e foc_fsm_step(foc_fsm_context_t* ctx)
{
    if (ctx == NULL) {
        return FOC_FSM_RET_ERROR;
    }

    fsm_err_t ret = fsm_step(&ctx->fsm);
    if (ret != FSM_OK) {
        return FOC_FSM_RET_ERROR;
    }

    return FOC_FSM_RET_OK;
}

foc_fsm_ret_e foc_fsm_request_state(foc_fsm_context_t* ctx, foc_fsm_state_e state)
{
    if (ctx == NULL) {
        return FOC_FSM_RET_ERROR;
    }

    fsm_err_t ret = fsm_goto(&ctx->fsm, state);
    if (ret == FSM_OK) {
        DEBUG_LOGD("foc_fsm", "状态机设置:%s成功", foc_fsm_state_to_string(state));
        return FOC_FSM_RET_OK;
    } else {
        DEBUG_LOGE("foc_fsm", "状态机设置:%s失败", foc_fsm_state_to_string(state));
        return FOC_FSM_RET_INVALID_STATE;
    }
}

const char* foc_fsm_state_to_string(foc_fsm_state_e state)
{
    if (state < FOC_FSM_STATE_COUNT) {
        return foc_state_names[state];
    }
    return "UNKNOWN";
}

void foc_fsm_set_flash_data(foc_fsm_context_t* ctx, void* flash_data)
{
    if (ctx != NULL) {
        ctx->flash_data = flash_data;
    }
}
