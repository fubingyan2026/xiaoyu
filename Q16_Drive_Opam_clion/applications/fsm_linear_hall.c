//
// Created by fubingyan on 25-8-2.
//

/**
 * @file    fsm_linear_hall.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-19
 * @brief   霍尔传感器校准模块（基于 FSM 框架重构）
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 */

/* Includes ------------------------------------------------------------------*/
#include "fsm_linear_hall.h"

#include "flash_task.h"
#include "foc_config_q16.h"
#include "public.h"

/* Private constants ---------------------------------------------------------*/

/** @brief 校准完成标志值 */
#define ADJUST_FLAG_VAL 0xEEEEU

/** @brief 校准旋转速度 */
#define ADJUST_VELOCITY 0.025f

/** @brief 对齐保持时间（控制周期数） */
#define ALIGN_HOLD_TIME 250U

/** @brief 旋转圈数 */
#define ROTATION_CIRCLES 2.5f

/** @brief 校准时滤波器截止频率 */
#define FILTER_FCUT_ADJUST 20U

/** @brief 滤波器稳定等待周期数 */
#define FILTER_INIT_WAIT 500U

/** @brief 校准滤波器的采样周期 */
#define ADJUST_FILTER_DT 0.001f

/* Private types -------------------------------------------------------------*/

/**
 * @brief 霍尔校准模块内部上下文
 */
typedef struct {
    fsm_t fsm; /**< FSM 上下文 */
    pt1Filter_t filter[ADC_CH_NUM]; /**< 校准用 PT1 滤波器 */
    float adc_max[ADC_CH_NUM]; /**< ADC 采样最大值 */
    float adc_min[ADC_CH_NUM]; /**< ADC 采样最小值 */
    float target_current; /**< 目标电流（校准对齐阶段用） */
    float target_elec_angle; /**< 目标电角度（校准旋转阶段用） */
    uint16_t filter_init_count; /**< 滤波器稳定计数器 */
    uint16_t align_count; /**< 对齐保持计数器 */
} fsm_linear_hall_ctx_t;

/* Private variables ---------------------------------------------------------*/

static fsm_linear_hall_ctx_t g_ctx;

/* Private function prototypes -----------------------------------------------*/

/* FSM 状态处理函数 */
static fsm_state_t handler_none(fsm_t* fsm);
static fsm_state_t handler_filter(fsm_t* fsm);
static fsm_state_t handler_align(fsm_t* fsm);
static fsm_state_t handler_rotation(fsm_t* fsm);
static fsm_state_t handler_process(fsm_t* fsm);
static fsm_state_t handler_done(fsm_t* fsm);

/* Exported functions --------------------------------------------------------*/

void fsm_linear_hall_init(void)
{
    // 初始化校准滤波器（20Hz 截止频率，1ms 采样周期）
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        pt1FilterInit(&g_ctx.filter[i],
            pt1FilterGain(FILTER_FCUT_ADJUST, ADJUST_FILTER_DT));
    }

    // 初始化 FSM
    static fsm_handler_t handlers[FSM_LINEAR_HALL_STATE_COUNT];
    static fsm_guard_t transitions[FSM_LINEAR_HALL_STATE_COUNT
        * FSM_LINEAR_HALL_STATE_COUNT];
    static const char* state_names[] = {
        "NONE",
        "FILTER",
        "ALIGN",
        "ROTATION",
        "PROCESS",
        "DONE",
    };

    memset(handlers, 0, sizeof(handlers));
    memset(transitions, 0, sizeof(transitions));

    handlers[FSM_LINEAR_HALL_STATE_NONE] = handler_none;
    handlers[FSM_LINEAR_HALL_STATE_FILTER] = handler_filter;
    handlers[FSM_LINEAR_HALL_STATE_ALIGN] = handler_align;
    handlers[FSM_LINEAR_HALL_STATE_ROTATION] = handler_rotation;
    handlers[FSM_LINEAR_HALL_STATE_PROCESS] = handler_process;
    handlers[FSM_LINEAR_HALL_STATE_DONE] = handler_done;

    fsm_config_t fsm_cfg = {
        .handlers = handlers,
        .transitions = transitions,
        .state_count = FSM_LINEAR_HALL_STATE_COUNT,
        .state_names = state_names,
        .user_data = &g_ctx,
    };
    fsm_fill(&fsm_cfg, fsm_always_true);
    fsm_init(&g_ctx.fsm, FSM_LINEAR_HALL_STATE_NONE, &fsm_cfg);
}

void fsm_linear_hall_task(void)
{
    // 每次调用先对 ADC 原始值进行 PT1 滤波（所有状态共享）
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        pt1FilterApply(&g_ctx.filter[i], (float)device_linear_hall_get_raw_buffer()[i]);
    }

    fsm_step(&g_ctx.fsm);
}

bool fsm_linear_hall_is_done(void)
{
    return (fsm_current_state(&g_ctx.fsm) == FSM_LINEAR_HALL_STATE_DONE);
}

fsm_linear_hall_state_t fsm_linear_hall_get_state(void)
{
    return (fsm_linear_hall_state_t)fsm_current_state(&g_ctx.fsm);
}

void fsm_linear_hall_start(void)
{
    fsm_goto(&g_ctx.fsm, FSM_LINEAR_HALL_STATE_NONE);
}

float fsm_linear_hall_get_current(void)
{
    return g_ctx.target_current;
}

float fsm_linear_hall_get_elec_angle(void)
{
    return g_ctx.target_elec_angle;
}

/* Private functions ---------------------------------------------------------*/

/* FSM 状态处理函数 ----------------------------------------------------------*/

static fsm_state_t handler_none(fsm_t* fsm)
{
    fsm_linear_hall_ctx_t* ctx = (fsm_linear_hall_ctx_t*)fsm_user_data(fsm);

    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        ctx->adc_max[i] = 0.0f;
        ctx->adc_min[i] = 0x1000;
    }

    return FSM_LINEAR_HALL_STATE_FILTER;
}

static fsm_state_t handler_filter(fsm_t* fsm)
{
    fsm_linear_hall_ctx_t* ctx = (fsm_linear_hall_ctx_t*)fsm_user_data(fsm);

    if (++ctx->filter_init_count >= FILTER_INIT_WAIT) {
        ctx->filter_init_count = 0;
        return FSM_LINEAR_HALL_STATE_ALIGN;
    }

    return FSM_LINEAR_HALL_STATE_FILTER;
}

static fsm_state_t handler_align(fsm_t* fsm)
{
    fsm_linear_hall_ctx_t* ctx = (fsm_linear_hall_ctx_t*)fsm_user_data(fsm);

    if (ctx->target_current < ALIGN_CURRENT) {
        ctx->target_current += 0.05f;
    } else {
        if (ctx->align_count < ALIGN_HOLD_TIME) {
            ctx->align_count++;
        } else {
            ctx->align_count = 0;
            return FSM_LINEAR_HALL_STATE_ROTATION;
        }
    }

    return FSM_LINEAR_HALL_STATE_ALIGN;
}

static fsm_state_t handler_rotation(fsm_t* fsm)
{
    fsm_linear_hall_ctx_t* ctx = (fsm_linear_hall_ctx_t*)fsm_user_data(fsm);

    // 跟踪 ADC 采样最大值和最小值
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        if (ctx->adc_max[i] < ctx->filter[i].state) {
            ctx->adc_max[i] = ctx->filter[i].state;
        }
        if (ctx->adc_min[i] > ctx->filter[i].state) {
            ctx->adc_min[i] = ctx->filter[i].state;
        }
    }

    // 递增电角度
    float max_angle = (float)MOTOR_POLES * M_2PI * ROTATION_CIRCLES * 1.25f;
    if (ctx->target_elec_angle < max_angle) {
        ctx->target_elec_angle += ADJUST_VELOCITY;
    } else {
        ctx->target_elec_angle = 0.0f;
        ctx->target_current = 0.0f;
        return FSM_LINEAR_HALL_STATE_PROCESS;
    }

    return FSM_LINEAR_HALL_STATE_ROTATION;
}

static fsm_state_t handler_process(fsm_t* fsm)
{
    (void)fsm;

    hall_save_param.calibrated_flag = ADJUST_FLAG_VAL;

    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        hall_save_param.adcAmplitudeBias[i] = (int16_t)((g_ctx.adc_max[i] + g_ctx.adc_min[i]) * 0.5f);
        hall_save_param.adcAmplitudeMax[i] = (int16_t)((g_ctx.adc_max[i] - g_ctx.adc_min[i]) * 0.5f);
    }

    flash_task_request(FLASH_TASK_WRITE_HALL, &hall_save_param,
        sizeof(hall_save_param));

    return FSM_LINEAR_HALL_STATE_DONE;
}

static fsm_state_t handler_done(fsm_t* fsm)
{
    (void)fsm;

    return FSM_LINEAR_HALL_STATE_NONE;
}
