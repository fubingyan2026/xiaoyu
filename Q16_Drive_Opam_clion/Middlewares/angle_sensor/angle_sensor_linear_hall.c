/**
 * @file    angle_sensor_linear_hall.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   线性霍尔传感器实现（PLL）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor_linear_hall.h"

#include <string.h>

#include "device_linear_hall.h"
#include "foc_config_q16.h"
#include "fsm_linear_hall_aligened.h"
#include "utils_math.h"

/* Private function prototypes -----------------------------------------------*/

static int sensor_linear_hall_init(angle_sensor_context_t* ctx);
static int sensor_linear_hall_calibrate(angle_sensor_context_t* ctx);
static bool sensor_linear_hall_is_calibrated(angle_sensor_context_t* ctx);
static uint16_t sensor_linear_hall_get_raw_angle(angle_sensor_context_t* ctx);
static float sensor_linear_hall_get_angle_rad(angle_sensor_context_t* ctx);
static float sensor_linear_hall_get_velocity_rads(angle_sensor_context_t* ctx);
static void sensor_linear_hall_update(angle_sensor_context_t* ctx);
static void sensor_linear_hall_get_info(angle_sensor_context_t* ctx,
    angle_sensor_info_t* info);
static void sensor_linear_hall_set_offset(angle_sensor_context_t* ctx,
    float offset);

/* Private variables ---------------------------------------------------------*/

static const angle_sensor_ops_t sensor_ops_linear_hall = {
    .init = sensor_linear_hall_init,
    .calibrate = sensor_linear_hall_calibrate,
    .is_calibrated = sensor_linear_hall_is_calibrated,
    .get_raw_angle = sensor_linear_hall_get_raw_angle,
    .get_angle_rad = sensor_linear_hall_get_angle_rad,
    .get_velocity_rads = sensor_linear_hall_get_velocity_rads,
    .update = sensor_linear_hall_update,
    .get_info = sensor_linear_hall_get_info,
    .set_offset = sensor_linear_hall_set_offset,
};

/* Exported functions --------------------------------------------------------*/

angle_sensor_error_t angle_sensor_linear_hall_init_context(
    angle_sensor_context_t* ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    ctx->ops = &sensor_ops_linear_hall;
    ctx->initialized = false;
    ctx->is_active = false;
    ctx->mechanical_offset = 0.0f;
    memset(&ctx->info, 0, sizeof(ctx->info));
    return ANGLE_SENSOR_OK;
}

/* Private functions ---------------------------------------------------------*/

static int sensor_linear_hall_init(angle_sensor_context_t* ctx)
{
    device_linear_hall_init();
    fsm_linear_hall_aligened_init();
    ctx->info.type = SENSOR_TYPE_LINEAR_HALL;
    ctx->info.resolution = 65535;
    ctx->info.pulses_per_rev = 65535.0f;
    ctx->info.poles = MOTOR_POLES;
    ctx->info.is_calibrated = fsm_linear_hall_aligened_is_done();
    ctx->info.offset = ctx->mechanical_offset;
    return 0;
}

static int sensor_linear_hall_calibrate(angle_sensor_context_t* ctx)
{
    (void)ctx;
    if (fsm_linear_hall_aligened_get_state() == FSM_LINEAR_HALL_ALIGENED_STATE_NONE) {
        fsm_linear_hall_aligened_start();
        return 0;
    }
    return -1;
}

static bool sensor_linear_hall_is_calibrated(angle_sensor_context_t* ctx)
{
    (void)ctx;
    return fsm_linear_hall_aligened_is_done();
}

static uint16_t sensor_linear_hall_get_raw_angle(angle_sensor_context_t* ctx)
{
    (void)ctx;
    float angle_rad = device_linear_hall_get_angle_rad();
    utils_norm_angle_0_2pi(&angle_rad);
    uint32_t raw = (uint32_t)((angle_rad / M_2PI) * 65536.0f);
    return (uint16_t)(raw & 0xFFFF);
}

static float sensor_linear_hall_get_angle_rad(angle_sensor_context_t* ctx)
{
    (void)ctx;
    return device_linear_hall_get_angle_rad();
}

static float sensor_linear_hall_get_velocity_rads(angle_sensor_context_t* ctx)
{
    (void)ctx;
    return device_linear_hall_get_velocity_rads();
}

static void sensor_linear_hall_update(angle_sensor_context_t* ctx)
{
    (void)ctx;
    if (!fsm_linear_hall_aligened_is_done()) {
        fsm_linear_hall_aligened_task();
    }
}

static void sensor_linear_hall_get_info(angle_sensor_context_t* ctx,
    angle_sensor_info_t* info)
{
    if (info) {
        info->type = SENSOR_TYPE_LINEAR_HALL;
        info->resolution = 65535;
        info->pulses_per_rev = 65535.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = fsm_linear_hall_aligened_is_done();
        info->offset = ctx->mechanical_offset;
    }
}

static void sensor_linear_hall_set_offset(angle_sensor_context_t* ctx,
    float offset)
{
    ctx->mechanical_offset = offset;
    ctx->info.offset = offset;
}
