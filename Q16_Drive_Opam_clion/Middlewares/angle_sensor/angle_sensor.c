/**
 * @file    angle_sensor.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   角度传感器抽象层 — 验证与调度（不接触硬件）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 */

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor.h"

#include <string.h>

#include "foc_config_q16.h"
#include "perf_counter.h"

/* Exported functions --------------------------------------------------------*/

angle_sensor_error_t angle_sensor_init(angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }

    if (ctx->initialized) {
        angle_sensor_deinit(ctx);
    }

    if (ctx->ops == NULL || ctx->ops->init == NULL) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }

    int ret = ctx->ops->init(ctx);
    if (ret != 0) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }

    ctx->is_active = true;
    ctx->initialized = true;
    return ANGLE_SENSOR_OK;
}

angle_sensor_error_t angle_sensor_deinit(angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }

    ctx->is_active = false;
    ctx->initialized = false;
    ctx->mechanical_offset = 0.0f;
    memset(&ctx->info, 0, sizeof(ctx->info));

    return ANGLE_SENSOR_OK;
}

bool angle_sensor_is_initialized(angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }
    return ctx->initialized;
}

angle_sensor_type_e angle_sensor_get_default_type(void)
{
    return SENSOR_TYPE_MT6701;
}

angle_sensor_error_t angle_sensor_get_data(angle_sensor_context_t *ctx,
                                           angle_sensor_data_t *data)
{
    if (ctx == NULL || data == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    if (!ctx->initialized) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }

    data->electrical_angle =
        ctx->ops && ctx->ops->get_angle_rad ? ctx->ops->get_angle_rad(ctx)
                                            : 0.0f;
    data->mechanical_angle = data->electrical_angle / MOTOR_POLES;
    data->velocity = ctx->ops && ctx->ops->get_velocity_rads
                         ? ctx->ops->get_velocity_rads(ctx)
                         : 0.0f;
    data->timestamp = get_system_ticks();
    data->status = (ctx->ops && ctx->ops->is_calibrated
                    && ctx->ops->is_calibrated(ctx))
                       ? SENSOR_STATUS_OK
                       : SENSOR_STATUS_CALIBRATING;

    return ANGLE_SENSOR_OK;
}

float angle_sensor_get_electrical_angle(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized || ctx->ops == NULL
        || ctx->ops->get_angle_rad == NULL) {
        return 0.0f;
    }
    return ctx->ops->get_angle_rad(ctx);
}

uint16_t angle_sensor_get_raw_angle(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized || ctx->ops == NULL
        || ctx->ops->get_raw_angle == NULL) {
        return 0;
    }
    return ctx->ops->get_raw_angle(ctx);
}

float angle_sensor_get_mechanical_angle(angle_sensor_context_t *ctx)
{
    return angle_sensor_get_electrical_angle(ctx) / MOTOR_POLES;
}

float angle_sensor_get_velocity(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized || ctx->ops == NULL
        || ctx->ops->get_velocity_rads == NULL) {
        return 0.0f;
    }
    return ctx->ops->get_velocity_rads(ctx);
}

bool angle_sensor_is_calibrated(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized || ctx->ops == NULL
        || ctx->ops->is_calibrated == NULL) {
        return false;
    }
    return ctx->ops->is_calibrated(ctx);
}

angle_sensor_error_t angle_sensor_calibrate(angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    if (!ctx->initialized) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }
    if (ctx->ops == NULL || ctx->ops->calibrate == NULL) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }
    int ret = ctx->ops->calibrate(ctx);
    if (ret != 0) {
        return ANGLE_SENSOR_ERROR_CALIBRATING;
    }
    return ANGLE_SENSOR_OK;
}

void angle_sensor_update(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized || ctx->ops == NULL
        || ctx->ops->update == NULL) {
        return;
    }
    ctx->ops->update(ctx);
}

void angle_sensor_get_info(angle_sensor_context_t *ctx,
                           angle_sensor_info_t *info)
{
    if (ctx == NULL || info == NULL) {
        return;
    }
    if (!ctx->initialized) {
        memset(info, 0, sizeof(*info));
        return;
    }
    if (ctx->ops && ctx->ops->get_info) {
        ctx->ops->get_info(ctx, info);
    }
}

void angle_sensor_set_offset(angle_sensor_context_t *ctx, float offset)
{
    if (ctx == NULL) {
        return;
    }
    ctx->mechanical_offset = offset;
    if (ctx->initialized && ctx->ops && ctx->ops->set_offset) {
        ctx->ops->set_offset(ctx, offset);
    }
}

angle_sensor_error_t angle_sensor_switch_type(angle_sensor_context_t *ctx,
                                              angle_sensor_type_e type)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    if (!ctx->initialized) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }
    if (type >= SENSOR_TYPE_MAX) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }
    if (ctx->info.type == type) {
        return ANGLE_SENSOR_OK;
    }

    ctx->is_active = false;

    if (ctx->ops == NULL || ctx->ops->init == NULL) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }

    int ret = ctx->ops->init(ctx);
    if (ret != 0) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }

    ctx->is_active = true;
    return ANGLE_SENSOR_OK;
}

angle_sensor_type_e angle_sensor_get_type(angle_sensor_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return SENSOR_TYPE_NONE;
    }
    return ctx->info.type;
}

bool angle_sensor_is_active(angle_sensor_context_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }
    return ctx->is_active;
}
