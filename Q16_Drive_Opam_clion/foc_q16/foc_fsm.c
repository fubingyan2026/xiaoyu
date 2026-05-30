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
#include "foc_hal.h"
#include "fsm_linear_hall.h"
#include "stdlib.h"

static const char* foc_state_names[] = { "IDLE", "ALIGN", "ALIGNMENT", "RUN", "HALL", "STOP" };

/**
 * @brief 通用 FSM 框架回调适配器：状态进入时执行初始化操作
 *
 * 根据不同状态执行相应的进入动作：
 *   - FOC_FSM_STATE_ALIGN：置零电流，设定预定位角度
 *   - FOC_FSM_STATE_ALIGNMENT：复位校准上下文，设置 IF 启动电流和初始角度
 *   - FOC_FSM_STATE_STOP：停止 PWM 输出
 *
 * @param fsm_ctx 通用 FSM 上下文指针（实际为 foc_fsm_context_t*）
 * @param state 进入的状态枚举值
 */
static void fsm_entry_adapter(fsm_t* fsm_ctx, fsm_state_t state)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    switch (state) {
    case FOC_FSM_STATE_ALIGN:
        /* 清零电流目标值，设置反向 360°（-4 * π/2）的初始电气角度用于转子预定位 */
        foc_set_target_id(foc, 0);
        foc_set_target_iq(foc, 0);
        foc_set_electrical_angle(foc, q16_16_mul(foc->align_theta_q, FLOAT_TO_Q16_16(-4.0f)));
        break;

    case FOC_FSM_STATE_ALIGNMENT:
        /* 复位编码器校准状态机 */
        foc_ctx->cali_ctx.capture_idx = 0;
        foc_ctx->cali_ctx.step = FOC_CALI_STEP_FORWARD;
        foc_ctx->cali_ctx.timeout_cnt = 0;
        foc_ctx->cali_ctx.last_angle_q = 0;
        /* 施加 IF 启动电流，速度为 0，角度回退到反向 360° 起始位置 */
        foc_set_target_iq(foc, foc->if_startup_iq_q);
        foc_set_omega(foc, 0);
        foc_set_electrical_angle(foc, q16_16_mul(foc->align_theta_q, FLOAT_TO_Q16_16(-4.0f)));
        break;

    case FOC_FSM_STATE_STOP:
        foc_hal_pwm_stop(&foc->port);
        break;

    default:
        break;
    }

    DEBUG_LOGD("foc_fsm", "状态机进入:%s", foc_fsm_state_to_string(state));
}

/**
 * @brief 通用 FSM 框架回调适配器：状态退出时执行清理操作
 *
 * 目前仅记录调试日志，未执行实质性退出动作。
 *
 * @param fsm_ctx 通用 FSM 上下文指针（实际为 foc_fsm_context_t*）
 * @param state 退出的状态枚举值
 */
static void fsm_exit_adapter(fsm_t* fsm_ctx, fsm_state_t state)
{
    (void)fsm_ctx;
    DEBUG_LOGD("foc_fsm", "状态机退出:%s", foc_fsm_state_to_string(state));
}

/**
 * @brief 循环角度平均值计算（用于编码器校准反向平均）
 *
 * 处理编码器角度在循环边界（0 到 encoder_lines 之间环绕）时的平均值计算。
 * 直接平均会遇到环绕问题（例如 350° 和 10° 直接平均 = 180°，实际应为 0°）。
 * 通过判断差值是否超过半周期来处理这种情况。
 *
 * @param a   第一个角度值（编码器原始读数）
 * @param b   第二个角度值（编码器原始读数）
 * @param cyc 编码器总周期（如 encoder_lines = 4096）
 * @return    循环意义上的平均值
 *
 * @note 示例：
 *   - a=4090, b=10, cyc=4096 → 返回 0（正确平均值）
 *   - a=100, b=200, cyc=4096 → 返回 150（直接平均即可）
 */
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

/**
 * @brief IDLE（空闲）状态处理器
 *
 * 在空闲状态下，FOC 不输出电流。检测到开关闭合时，
 * 启动 PWM 输出并切换到 RUN 状态。
 *
 * @param fsm_ctx 通用 FSM 上下文指针
 * @return 下一个状态：FOC_FSM_STATE_RUN（开关闭合）或 FOC_FSM_STATE_IDLE（维持空闲）
 */
static fsm_state_t handler_idle(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    if (foc_get_sw(foc)) {
        foc_hal_pwm_start(&foc->port);
        return FOC_FSM_STATE_RUN;
    }
    return FOC_FSM_STATE_IDLE;
}

/**
 * @brief ALIGN（对齐/预定位）状态处理器
 *
 * 执行转子预定位：逐渐增加 Iq 电流到目标值 foc->align_current_q，
 * 通过定子磁场将转子拉到已知角度位置。
 * 电流达到目标值后保持一段时间（由 FOC_FSM_ALIGN_TIMEOUT_CNT 设定），
 * 然后切换到 ALIGNMENT（校准）状态。
 *
 * @param fsm_ctx 通用 FSM 上下文指针
 * @return 下一个状态：FOC_FSM_STATE_ALIGNMENT（对齐超时）或 FOC_FSM_STATE_ALIGN（维持对齐）
 */
static fsm_state_t handler_align(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    if (foc_get_target_iq(foc) < foc->align_current_q) {
        foc_set_target_iq(foc, q16_16_add(foc_get_target_iq(foc), q16_16_mul(foc->align_current_q, foc->state_period_q)));
    } else {
        static uint16_t align_cnt = 0;
        if (align_cnt++ >= FOC_FSM_ALIGN_TIMEOUT_CNT) {
            align_cnt = 0;
            foc_set_electrical_angle(foc, q16_16_mul(foc->align_theta_q, FLOAT_TO_Q16_16(-4.0f)));
            return FOC_FSM_STATE_ALIGNMENT;
        }
    }
    return FOC_FSM_STATE_ALIGN;
}

/**
 * @brief ALIGNMENT（编码器校准）状态处理器
 *
 * 编码器自动校准主状态机。通过正向扫描和反向扫描两个阶段，
 * 建立编码器物理位置到电气角度的映射表。
 *
 * 校准流程：
 *   1. FOC_CALI_STEP_FORWARD（正向扫描）：
 *      逐步增加电气角度，在每个角度台阶等待稳定后，
 *      记录编码器读数 → 电气角度的映射关系
 *   2. FOC_CALI_STEP_TRANSITION（过渡）：
 *      速度减速到 0，为反向扫描做准备
 *   3. FOC_CALI_STEP_REVERSE（反向扫描）：
 *      反向逐步减小电气角度，对每个位置再次记录编码器读数，
 *      与正向数据做循环平均，消除机械间隙误差
 *   4. FOC_CALI_STEP_COMPLETE（完成）：
 *      检测方向并写入 Flash，切换到 RUN 状态
 *
 * @param fsm_ctx 通用 FSM 上下文指针
 * @return 下一个状态：FOC_FSM_STATE_ALIGNMENT（校准中）或 FOC_FSM_STATE_RUN（校准完成）
 */
static fsm_state_t handler_alignment(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;
    motor_flash_config_t* flash_data = (motor_flash_config_t*)foc_ctx->flash_data;
    q16_16_t threshold = q16_16_mul(foc->align_theta_q, FLOAT_TO_Q16_16(0.5f));

    if (foc_get_omega(foc) < foc->if_startup_omega_q) {
        foc_set_omega(foc, q16_16_add(foc_get_omega(foc), foc->if_startup_omega_acc_q));
    } else {
        foc_set_omega(foc, foc->if_startup_omega_q);
    }

    foc_set_target_iq(foc, foc->if_startup_iq_q);

    switch (foc_ctx->cali_ctx.step) {
    case FOC_CALI_STEP_FORWARD: {
        if (foc_get_electrical_angle(foc) < foc_ctx->cali_ctx.last_angle_q) {
            foc_set_electrical_angle(foc, q16_16_add(foc_get_electrical_angle(foc), foc_get_omega(foc)));
        } else {
            foc_set_electrical_angle(foc, foc_ctx->cali_ctx.last_angle_q);

            if (foc_ctx->cali_ctx.timeout_cnt++ >= FOC_FSM_ELEC_ANGLE_STABLE_TIME) {
                foc_ctx->cali_ctx.timeout_cnt = 0;
                foc_ctx->cali_ctx.last_angle_q = q16_16_add(foc_ctx->cali_ctx.last_angle_q, foc->align_theta_q);

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
                foc_set_target_iq(foc, foc->if_startup_iq_q);
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
                    foc_ctx->cali_ctx.last_angle_q = q16_16_sub(foc_ctx->cali_ctx.last_angle_q, foc->align_theta_q);
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

/**
 * @brief RUN（正常运行）状态处理器
 *
 * 正常运行模式：D 轴电流目标设为 0（单位功率因数控制），
 * Q 轴电流目标设为 0.25A（轻载运行）。
 * 实际运行时由更高层（如速度环）动态修改电流目标值。
 *
 * @param fsm_ctx 通用 FSM 上下文指针
 * @return 始终返回 FOC_FSM_STATE_RUN（维持运行状态）
 */
static fsm_state_t handler_run(fsm_t* fsm_ctx)
{
    foc_fsm_context_t* foc_ctx = (foc_fsm_context_t*)fsm_ctx;
    foc_context_t* foc = (foc_context_t*)foc_ctx->parent;

    foc_set_target_id(foc, 0);
    foc_set_target_iq(foc, FLOAT_TO_Q16_16(0.25f));

    return FOC_FSM_STATE_RUN;
}

/**
 * @brief HALL（霍尔传感器启动）状态处理器
 *
 * 使用线性霍尔传感器进行初始位置检测和开环启动。
 * 霍尔传感器检测到转子位置后，通过 fsm_linear_hall_task() 更新角度和电流，
 * 逐渐将电机加速到可切换至无感 FOC 的速度。
 * 当 fsm_linear_hall 模块完成后，切换到 ALIGN 状态进行编码器校准。
 *
 * @param fsm_ctx 通用 FSM 上下文指针
 * @return 下一个状态：FOC_FSM_STATE_ALIGN（霍尔完成）或 FOC_FSM_STATE_HALL（维持霍尔模式）
 */
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

/**
 * @brief STOP（停止）状态处理器
 *
 * PWM 输出已在进入此状态时停止（在 fsm_entry_adapter 中执行），
 * 此处仅保持状态不变。
 *
 * @param fsm_ctx 通用 FSM 上下文指针（未使用）
 * @return 始终返回 FOC_FSM_STATE_STOP（维持停止状态）
 */
static fsm_state_t handler_stop(fsm_t* fsm_ctx)
{
    (void)fsm_ctx;
    return FOC_FSM_STATE_STOP;
}

/**
 * @brief 初始化 FOC 状态机
 *
 * 注册所有状态处理器函数和状态间允许的转换，
 * 初始化通用 FSM 框架并设置起始状态为 IDLE。
 *
 * @param ctx    FSM 上下文指针
 * @param parent 父级 FOC 上下文指针（foc_context_t*）
 * @return FOC_FSM_RET_OK 初始化成功
 *         FOC_FSM_RET_ERROR 参数为空或 FSM 初始化失败
 */
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
    /* 所有状态间均可自由转换（使用 fsm_always_true 守卫） */
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

/**
 * @brief 执行一步 FOC 状态机
 *
 * 调用通用 FSM 框架的 fsm_step() 执行当前状态处理器函数。
 * 应在主循环中以 STATE_PERIOD（1ms）的间隔周期调用。
 *
 * @param ctx FSM 上下文指针
 * @return FOC_FSM_RET_OK 执行成功
 *         FOC_FSM_RET_ERROR 参数为空或 FSM 执行出错
 */
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

/**
 * @brief 请求 FOC 状态机切换到指定状态
 *
 * 通过通用 FSM 框架的 fsm_goto() 请求状态切换。
 * 如果目标状态无效或当前状态不允许切换到目标状态，则返回错误。
 *
 * @param ctx   FSM 上下文指针
 * @param state 目标状态枚举值
 * @return FOC_FSM_RET_OK 切换成功
 *         FOC_FSM_RET_ERROR 参数为空
 *         FOC_FSM_RET_INVALID_STATE 目标状态不允许切换
 */
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

/**
 * @brief 将状态枚举值转换为可读的字符串
 *
 * 用于调试日志输出，便于跟踪状态机运行状态。
 *
 * @param state 状态枚举值
 * @return 状态名称字符串（"IDLE"、"ALIGN" 等），无效状态返回 "UNKNOWN"
 */
const char* foc_fsm_state_to_string(foc_fsm_state_e state)
{
    if (state < FOC_FSM_STATE_COUNT) {
        return foc_state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief 设置 Flash 校准数据指针
 *
 * 状态机在校准完成后需要将校准数据写入 Flash，
 * 此函数传递 Flash 数据结构的指针给状态机上下文。
 *
 * @param ctx        FSM 上下文指针
 * @param flash_data Flash 校准数据结构体指针（motor_flash_config_t*）
 */
void foc_fsm_set_flash_data(foc_fsm_context_t* ctx, void* flash_data)
{
    if (ctx != NULL) {
        ctx->flash_data = flash_data;
    }
}
