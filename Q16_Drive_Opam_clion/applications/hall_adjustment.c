//
// Created by fubingyan on 25-8-2.
//

/**
 * @file    hall_adjustment.c
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
#include "hall_adjustment.h"

#include "adc.h"
#include "easyflash.h"
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

/** @brief 正常运行时滤波器截止频率 */
#define FILTER_FCUT_NORMAL 10000U

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
    fsm_t fsm;                              /**< FSM 上下文 */
    pt1Filter_t filter[ADC_CH_NUM];         /**< 校准用 PT1 滤波器 */
    pt1Filter_t filter_normal[ADC_CH_NUM];  /**< 正常运行 PT1 滤波器 */
    float adc_max[ADC_CH_NUM];              /**< ADC 采样最大值 */
    float adc_min[ADC_CH_NUM];              /**< ADC 采样最小值 */
    uint32_t adc_raw[ADC_CH_NUM];           /**< ADC 原始采样值（DMA 目标） */
    float angle;                            /**< 当前角度 */
    float target_current;                   /**< 目标电流（校准对齐阶段用） */
    float target_elec_angle;                /**< 目标电角度（校准旋转阶段用） */
    uint16_t filter_init_count;             /**< 滤波器稳定计数器 */
    uint16_t align_count;                   /**< 对齐保持计数器 */
    daemon_context_t daemon_encoder;       /**< 编码器守护进程 */
} hall_adjust_ctx_t;

/* Private variables ---------------------------------------------------------*/

static hall_adjust_ctx_t g_ctx;

/* Global variables ----------------------------------------------------------*/

hall_save_param_t hall_save_param;
pll_context_t pll_ctx;

/* Private function prototypes -----------------------------------------------*/

static void hall_filter_init(pt1Filter_t *filter, uint16_t f_cut, float dt);

/* FSM 状态处理函数 */
static fsm_state_t handler_none(fsm_t *fsm);
static fsm_state_t handler_filter(fsm_t *fsm);
static fsm_state_t handler_align(fsm_t *fsm);
static fsm_state_t handler_rotation(fsm_t *fsm);
static fsm_state_t handler_process(fsm_t *fsm);
static fsm_state_t handler_done(fsm_t *fsm);

/* Exported functions --------------------------------------------------------*/

void hall_adjust_init(void)
{
    // 初始化正常运行滤波器（10kHz 截止频率，FOC PWM 周期采样）
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        hall_filter_init(&g_ctx.filter_normal[i], FILTER_FCUT_NORMAL,
                         FOC_PWM_PERIOD);
    }

    // 初始化校准滤波器（20Hz 截止频率，1ms 采样周期）
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        hall_filter_init(&g_ctx.filter[i], FILTER_FCUT_ADJUST,
                         ADJUST_FILTER_DT);
    }

    // 从 Flash 加载校准参数
    ef_get_env_blob(FLASH_MAGIC_HALL, &hall_save_param,
                    sizeof(hall_save_param), NULL);

    // 初始化 PLL
    const pll_config_t pll_config = {
        .kp = 250.0f,
        .ki = 32000.0f,
        .sample_time = FOC_PWM_PERIOD,
        .filter_freq_dq = 500.0f,
        .filter_freq_omega = 20.0f,
    };
    pll_init(&pll_ctx, &pll_config);

    // 获取编码器守护进程
    const daemon_config_t encoder_cfg = {
      .name = "line_hall",
      .init_wait_time_ms = 1000,
      .reload_timeout_ms = 10,
      .offline_cb=NULL,
    };

    daemon_error_t err =
        daemon_register_static(&encoder_cfg, &g_ctx.daemon_encoder);
    DEBUG_ASSERT(err == DAEMON_OK);

    // 初始化 FSM
    static fsm_handler_t handlers[HALL_ADJUST_STATE_COUNT];
    static fsm_guard_t transitions[HALL_ADJUST_STATE_COUNT
                                   * HALL_ADJUST_STATE_COUNT];
    static const char *state_names[] = {
        "NONE", "FILTER", "ALIGN", "ROTATION", "PROCESS", "DONE",
    };

    memset(handlers, 0, sizeof(handlers));
    memset(transitions, 0, sizeof(transitions));

    handlers[HALL_ADJUST_STATE_NONE] = handler_none;
    handlers[HALL_ADJUST_STATE_FILTER] = handler_filter;
    handlers[HALL_ADJUST_STATE_ALIGN] = handler_align;
    handlers[HALL_ADJUST_STATE_ROTATION] = handler_rotation;
    handlers[HALL_ADJUST_STATE_PROCESS] = handler_process;
    handlers[HALL_ADJUST_STATE_DONE] = handler_done;

    fsm_config_t fsm_cfg = {
        .handlers = handlers,
        .transitions = transitions,
        .state_count = HALL_ADJUST_STATE_COUNT,
        .state_names = state_names,
        .user_data = &g_ctx,
    };
    fsm_fill(&fsm_cfg, fsm_always_true);
    fsm_init(&g_ctx.fsm, HALL_ADJUST_STATE_NONE, &fsm_cfg);
}

void hall_adjust_task(void)
{
    // 每次调用先对 ADC 原始值进行 PT1 滤波（所有状态共享）
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        pt1FilterApply(&g_ctx.filter[i], (float)g_ctx.adc_raw[i]);
    }

    fsm_step(&g_ctx.fsm);
}

uint16_t hall_adjust_get_angle(void)
{
    // 对 ADC 原始值进行正常运行滤波
    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        pt1FilterApply(&g_ctx.filter_normal[i], (float)g_ctx.adc_raw[i]);
    }

    // 归一化到 [-1, 1] 范围
    float y = (float)((int32_t)g_ctx.filter_normal[1].state
                      - hall_save_param.adcAmplitudeBias[1])
              / (float)hall_save_param.adcAmplitudeMax[1];
    float x = (float)((int32_t)g_ctx.filter_normal[0].state
                      - hall_save_param.adcAmplitudeBias[0])
              / (float)hall_save_param.adcAmplitudeMax[0];

    // 未校准时使用极小值防止除零
    if (hall_save_param.hall_adjust_flag != ADJUST_FLAG_VAL) {
        y = 0.0001f;
        x = 0.0001f;
    }

    // PLL 锁相环更新
    pll_update(&pll_ctx, y, x);
    g_ctx.angle = pll_get_angle(&pll_ctx);

    // atan2 近似计算（覆盖 PLL 结果）
    g_ctx.angle = atan2_approx(y, x);

    // 角度规范化到 [0, 2*PI]
    UTILS_NAN_ZERO(g_ctx.angle);
    utils_norm_angle_rad(&g_ctx.angle);

    // 喂狗
   
    daemon_reload(&g_ctx.daemon_encoder);
    
    // 转换为 14 位角度 (0-16383)
    return (uint16_t)((g_ctx.angle + M_PI) / (M_2PI)*16384.0f);
}

void hall_adjust_adc_dma_start(void)
{
    HAL_ADC_Start_DMA(&hadc2, g_ctx.adc_raw, ADC_CH_NUM);
}

bool hall_adjust_is_calibrated(void)
{
    return (fsm_current_state(&g_ctx.fsm) == HALL_ADJUST_STATE_DONE);
}

hall_adjust_state_t hall_adjust_get_state(void)
{
    return (hall_adjust_state_t)fsm_current_state(&g_ctx.fsm);
}

void hall_adjust_start_calibration(void)
{
    fsm_jump(&g_ctx.fsm, HALL_ADJUST_STATE_NONE);
}

float hall_adjust_get_target_current(void)
{
    return g_ctx.target_current;
}

float hall_adjust_get_target_elec_angle(void)
{
    return g_ctx.target_elec_angle;
}

/* Private functions ---------------------------------------------------------*/

static void hall_filter_init(pt1Filter_t *filter, uint16_t f_cut, float dt)
{
    pt1FilterInit(filter, pt1FilterGain(f_cut, dt));
}

/* FSM 状态处理函数 ----------------------------------------------------------*/

static fsm_state_t handler_none(fsm_t *fsm)
{
    hall_adjust_ctx_t *ctx = (hall_adjust_ctx_t *)fsm_user_data(fsm);

    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        ctx->adc_max[i] = 0.0f;
        ctx->adc_min[i] = 0x1000;
    }

    return HALL_ADJUST_STATE_FILTER;
}

static fsm_state_t handler_filter(fsm_t *fsm)
{
    hall_adjust_ctx_t *ctx = (hall_adjust_ctx_t *)fsm_user_data(fsm);

    if (++ctx->filter_init_count >= FILTER_INIT_WAIT) {
        ctx->filter_init_count = 0;
        return HALL_ADJUST_STATE_ALIGN;
    }

    return HALL_ADJUST_STATE_FILTER;
}

static fsm_state_t handler_align(fsm_t *fsm)
{
    hall_adjust_ctx_t *ctx = (hall_adjust_ctx_t *)fsm_user_data(fsm);

    if (ctx->target_current < ALIGN_CURRENT) {
        ctx->target_current += 0.05f;
    } else {
        if (ctx->align_count < ALIGN_HOLD_TIME) {
            ctx->align_count++;
        } else {
            ctx->align_count = 0;
            return HALL_ADJUST_STATE_ROTATION;
        }
    }

    return HALL_ADJUST_STATE_ALIGN;
}

static fsm_state_t handler_rotation(fsm_t *fsm)
{
    hall_adjust_ctx_t *ctx = (hall_adjust_ctx_t *)fsm_user_data(fsm);

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
        return HALL_ADJUST_STATE_PROCESS;
    }

    return HALL_ADJUST_STATE_ROTATION;
}

static fsm_state_t handler_process(fsm_t *fsm)
{
    (void)fsm;

    hall_save_param.hall_adjust_flag = ADJUST_FLAG_VAL;

    for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        hall_save_param.adcAmplitudeBias[i] =
            (int16_t)((g_ctx.adc_max[i] + g_ctx.adc_min[i]) * 0.5f);
        hall_save_param.adcAmplitudeMax[i] =
            (int16_t)((g_ctx.adc_max[i] - g_ctx.adc_min[i]) * 0.5f);
    }

    flash_task_request(FLASH_TASK_WRITE_HALL, &hall_save_param,
                       sizeof(hall_save_param));

    return HALL_ADJUST_STATE_DONE;
}

static fsm_state_t handler_done(fsm_t *fsm)
{
    (void)fsm;

    return HALL_ADJUST_STATE_NONE;
}
