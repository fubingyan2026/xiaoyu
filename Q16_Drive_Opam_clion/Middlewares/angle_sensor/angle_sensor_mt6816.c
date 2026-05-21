/**
 * @file    angle_sensor_mt6816.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   MT6816 磁编码器传感器实现（SPI）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor_mt6816.h"

#include <string.h>

#include "MT6816.h"
#include "foc_config_q16.h"
#include "utils_math.h"

/* Private function prototypes -----------------------------------------------*/

static int sensor_mt6816_init(angle_sensor_context_t *ctx);
static int sensor_mt6816_calibrate(angle_sensor_context_t *ctx);
static bool sensor_mt6816_is_calibrated(angle_sensor_context_t *ctx);
static uint16_t sensor_mt6816_get_raw_angle(angle_sensor_context_t *ctx);
static float sensor_mt6816_get_angle_rad(angle_sensor_context_t *ctx);
static float sensor_mt6816_get_velocity_rads(angle_sensor_context_t *ctx);
static void sensor_mt6816_update(angle_sensor_context_t *ctx);
static void sensor_mt6816_get_info(angle_sensor_context_t *ctx,
                                   angle_sensor_info_t *info);
static void sensor_mt6816_set_offset(angle_sensor_context_t *ctx, float offset);

/* Private variables ---------------------------------------------------------*/

static const angle_sensor_ops_t sensor_ops_mt6816 = {
    .init = sensor_mt6816_init,
    .calibrate = sensor_mt6816_calibrate,
    .is_calibrated = sensor_mt6816_is_calibrated,
    .get_raw_angle = sensor_mt6816_get_raw_angle,
    .get_angle_rad = sensor_mt6816_get_angle_rad,
    .get_velocity_rads = sensor_mt6816_get_velocity_rads,
    .update = sensor_mt6816_update,
    .get_info = sensor_mt6816_get_info,
    .set_offset = sensor_mt6816_set_offset,
};

/* Exported functions --------------------------------------------------------*/

angle_sensor_error_t angle_sensor_mt6816_init_context(
    angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    ctx->ops = &sensor_ops_mt6816;
    ctx->initialized = false;
    ctx->is_active = false;
    ctx->mechanical_offset = 0.0f;
    memset(&ctx->info, 0, sizeof(ctx->info));
    return ANGLE_SENSOR_OK;
}

/* Private functions ---------------------------------------------------------*/

static int sensor_mt6816_init(angle_sensor_context_t *ctx)
{
    MT6816_Init();
    ctx->info.type = SENSOR_TYPE_MT6816;
    ctx->info.resolution = 16384;
    ctx->info.pulses_per_rev = 16384.0f;
    ctx->info.poles = MOTOR_POLES;
    ctx->info.is_calibrated = true;
    ctx->info.offset = ctx->mechanical_offset;
    return 0;
}

static int sensor_mt6816_calibrate(angle_sensor_context_t *ctx)
{
    (void)ctx;
    return 0;
}

static bool sensor_mt6816_is_calibrated(angle_sensor_context_t *ctx)
{
    (void)ctx;
    return true;
}

static uint16_t sensor_mt6816_get_raw_angle(angle_sensor_context_t *ctx)
{
    (void)ctx;
    return REIN_MT6816_Get_AngleData();
}

static float sensor_mt6816_get_angle_rad(angle_sensor_context_t *ctx)
{
    uint16_t raw = REIN_MT6816_Get_AngleData();
    float angle = ((float)raw / 16384.0f) * M_2PI;
    angle += ctx->mechanical_offset;
    utils_norm_angle_0_2pi(&angle);
    return angle;
}

static float sensor_mt6816_get_velocity_rads(angle_sensor_context_t *ctx)
{
    (void)ctx;
    return 0.0f;
}

static void sensor_mt6816_update(angle_sensor_context_t *ctx)
{
    (void)ctx;
}

static void sensor_mt6816_get_info(angle_sensor_context_t *ctx,
                                   angle_sensor_info_t *info)
{
    if (info) {
        info->type = SENSOR_TYPE_MT6816;
        info->resolution = 16384;
        info->pulses_per_rev = 16384.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = true;
        info->offset = ctx->mechanical_offset;
    }
}

static void sensor_mt6816_set_offset(angle_sensor_context_t *ctx, float offset)
{
    ctx->mechanical_offset = offset;
    ctx->info.offset = offset;
}
