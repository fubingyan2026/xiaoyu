//
// Created by fubingyan on 25-8-2.
//

/**
 * @file    device_linear_hall.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-08-02
 * @brief   线性霍尔传感器设备驱动（ADC2 + DMA + PLL 角度计算）
 * @attention
 *
 * Copyright (c) 2025 by fubingyan, All Rights Reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "device_linear_hall.h"

#include "easyflash.h"
#include "flash_task.h"
#include "foc_config_q16.h"
#include "hal_adc.h"
#include "public.h"

/* Private constants ---------------------------------------------------------*/

/** @brief 校准完成标志值 */
#define ADJUST_FLAG_VAL 0xEEEEU

/** @brief 正常运行时滤波器截止频率 */
#define FILTER_FCUT_NORMAL 10000U

/* Private variables ---------------------------------------------------------*/

static hal_adc_context_t s_adc_ctx;

static uint32_t s_adc_raw[DEVICE_LINEAR_HALL_ADC_CH_NUM];

static hal_adc_dma_config_t s_dma_cfg;

static pt1Filter_t s_filter_normal[DEVICE_LINEAR_HALL_ADC_CH_NUM];

static float s_angle;

static daemon_context_t s_daemon_encoder;

/* Global variables ----------------------------------------------------------*/

pll_context_t pll_ctx;
hall_save_param_t hall_save_param;

/* Exported functions --------------------------------------------------------*/

void device_linear_hall_init(void)
{
    /* 初始化 ADC2 */
    stm32_adc_init_context(&s_adc_ctx);

    hal_adc_config_t adc_cfg = {
        .instance = HAL_ADC_INSTANCE_2,
        .resolution = HAL_ADC_RESOLUTION_12B,
        .data_align = HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = HAL_ADC_SCAN_MODE_ENABLE,
        .continuous = HAL_ADC_CONTINUOUS_DISABLE,
        .trigger = HAL_ADC_TRIGGER_SOFTWARE,
        .dma_mode = HAL_ADC_DMA_ENABLE,
        .clock_prescaler = 4,
        .nbr_of_conversion = DEVICE_LINEAR_HALL_ADC_CH_NUM,
        .timeout = 100,
    };
    hal_adc_init(&s_adc_ctx, &adc_cfg);

    hal_adc_channel_config_t ch_cfg1 = {
        .channel = HAL_ADC_CHANNEL_3,
        .sample_time = HAL_ADC_SAMPLETIME_24CYCLES_5,
        .rank = 0,
    };
    hal_adc_config_channel(&s_adc_ctx, &ch_cfg1);

    hal_adc_channel_config_t ch_cfg2 = {
        .channel = HAL_ADC_CHANNEL_17,
        .sample_time = HAL_ADC_SAMPLETIME_24CYCLES_5,
        .rank = 1,
    };
    hal_adc_config_channel(&s_adc_ctx, &ch_cfg2);

    s_dma_cfg.buffer = s_adc_raw;
    s_dma_cfg.buffer_length = DEVICE_LINEAR_HALL_ADC_CH_NUM;
    s_dma_cfg.circular_mode = false;

    /* 初始化正常运行滤波器（10kHz 截止频率，FOC PWM 周期采样） */
    for (uint8_t i = 0; i < DEVICE_LINEAR_HALL_ADC_CH_NUM; i++) {
        pt1FilterInit(&s_filter_normal[i],
            pt1FilterGain(FILTER_FCUT_NORMAL, FOC_PWM_PERIOD));
    }

    /* 从 Flash 加载校准参数 */
    ef_get_env_blob(FLASH_MAGIC_HALL, &hall_save_param,
        sizeof(hall_save_param), NULL);

    /* 初始化 PLL */
    const pll_config_t pll_config = {
        .kp = 250.0f,
        .ki = 32000.0f,
        .sample_time = FOC_PWM_PERIOD,
        .filter_freq_omega = 20.0f,
    };
    pll_init(&pll_ctx, &pll_config);

    /* 注册守护进程 */
    const daemon_config_t encoder_cfg = {
        .name = "line_hall",
        .init_wait_time_ms = 1000,
        .reload_timeout_ms = 10,
        .offline_cb = NULL,
    };

    daemon_error_t err =
        daemon_register_static(&encoder_cfg, &s_daemon_encoder);
    DEBUG_ASSERT(err == DAEMON_OK);
}

void device_linear_hall_start_dma(void)
{
    hal_adc_start_dma(&s_adc_ctx, HAL_ADC_INSTANCE_2, &s_dma_cfg);
}

const uint32_t* device_linear_hall_get_raw_buffer(void)
{
    return s_adc_raw;
}

float device_linear_hall_get_angle_rad(void)
{
    /* 对 ADC 原始值进行正常运行滤波 */
    for (uint8_t i = 0; i < DEVICE_LINEAR_HALL_ADC_CH_NUM; i++) {
        pt1FilterApply(&s_filter_normal[i],
            (float)device_linear_hall_get_raw_buffer()[i]);
    }

    /* 归一化到 [-1, 1] 范围 */
    float y = (float)((int32_t)s_filter_normal[1].state
                  - hall_save_param.adcAmplitudeBias[1])
              / (float)hall_save_param.adcAmplitudeMax[1];
    float x = (float)((int32_t)s_filter_normal[0].state
                  - hall_save_param.adcAmplitudeBias[0])
              / (float)hall_save_param.adcAmplitudeMax[0];

    /* 未校准时使用极小值防止除零 */
    if (hall_save_param.calibrated_flag != ADJUST_FLAG_VAL) {
        y = 0.0001f;
        x = 0.0001f;
    }

    /* PLL 锁相环更新 */
    pll_update(&pll_ctx, y, x);
    s_angle = pll_get_angle(&pll_ctx);

    utils_norm_angle_rad(&s_angle);

    /* 喂狗 */
    daemon_reload(&s_daemon_encoder);

    return s_angle;
}

float device_linear_hall_get_velocity_rads(void)
{
    return pll_get_speed(&pll_ctx);
}

uint16_t device_linear_hall_get_angle(void)
{
    /* 确保角度已计算 */
    device_linear_hall_get_angle_rad();

    /* 转换为 14 位角度 (0-16383) */
    return (uint16_t)((s_angle + M_PI) / (M_2PI)*16384.0f);
}
