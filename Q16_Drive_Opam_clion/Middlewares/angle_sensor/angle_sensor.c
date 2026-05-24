/**
 * @file    angle_sensor.c
 * @author  fubingyan
 * @version V2.1.0
 * @date    2026-05-24
 * @brief   角度传感器抽象层 — 配置驱动实现
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor.h"

#include <string.h>

#include "device_linear_hall.h"
#include "device_mt6701.h"
#include "device_mt6816.h"
#include "foc_config_q16.h"
#include "fsm_linear_hall.h"
#include "perf_counter.h"
#include "utils_math.h"

/* Private types -------------------------------------------------------------*/

/**
 * @brief 传感器配置结构体 — 描述每种传感器类型的设备驱动绑定和参数
 */
struct sensor_config {
    angle_sensor_type_e type;
    uint16_t resolution;
    float pulses_per_rev;

    void (*device_init)(void);
    uint16_t (*device_get_raw_angle)(void);
    float (*device_get_angle_rad)(void);
    float (*device_get_velocity)(void);

    int (*calibrate_start)(void);
    bool (*calibrate_is_done)(void);
    void (*calibrate_update)(void);
};

/* Private function prototypes -----------------------------------------------*/

static int sensor_generic_init(angle_sensor_context_t* ctx);

/* Calibrate helpers ---------------------------------------------------------*/

static int linear_hall_calibrate_start(void)
{
    if (fsm_linear_hall_get_state() == FSM_LINEAR_HALL_STATE_NONE) {
        fsm_linear_hall_start();
        return 0;
    }
    return -1;
}

static void linear_hall_calibrate_update(void)
{
    if (!fsm_linear_hall_is_done()) {
        fsm_linear_hall_task();
    }
}

/* Private variables ---------------------------------------------------------*/

static const struct sensor_config s_sensor_configs[SENSOR_TYPE_MAX] = {
    [SENSOR_TYPE_NONE] = {
        .type = SENSOR_TYPE_NONE,
        .resolution = 0,
        .pulses_per_rev = 0.0f,
    },
    [SENSOR_TYPE_MT6701] = {
        .type = SENSOR_TYPE_MT6701,
        .resolution = 16384,
        .pulses_per_rev = 16384.0f,
        .device_init = device_mt6816_init,
        .device_get_raw_angle = device_mt6701_get_angle_data,
    },
    [SENSOR_TYPE_MT6816] = {
        .type = SENSOR_TYPE_MT6816,
        .resolution = 16384,
        .pulses_per_rev = 16384.0f,
        .device_init = device_mt6816_init,
        .device_get_raw_angle = device_mt6816_get_angle_data,
    },
    [SENSOR_TYPE_LINEAR_HALL] = {
        .type = SENSOR_TYPE_LINEAR_HALL,
        .resolution = 65535,
        .pulses_per_rev = 65535.0f,
        .device_init = device_linear_hall_init,
        .device_get_angle_rad = device_linear_hall_get_angle_rad,
        .device_get_velocity = device_linear_hall_get_velocity_rads,
        .calibrate_start = linear_hall_calibrate_start,
        .calibrate_is_done = fsm_linear_hall_is_done,
        .calibrate_update = linear_hall_calibrate_update,
    },
};

/* Exported functions --------------------------------------------------------*/

angle_sensor_error_t angle_sensor_init_context(angle_sensor_context_t* ctx,
    angle_sensor_type_e type)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    if (type >= SENSOR_TYPE_MAX) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }

    ctx->config = &s_sensor_configs[type];
    ctx->initialized = false;
    ctx->is_active = false;
    ctx->mechanical_offset = 0.0f;
    memset(&ctx->info, 0, sizeof(ctx->info));
    return ANGLE_SENSOR_OK;
}

angle_sensor_error_t angle_sensor_init(angle_sensor_context_t* ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }

    if (ctx->initialized) {
        angle_sensor_deinit(ctx);
    }

    if (ctx->config == NULL) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }

    int ret = sensor_generic_init(ctx);
    if (ret != 0) {
        return ANGLE_SENSOR_ERROR_NOT_SUPPORTED;
    }

    ctx->is_active = true;
    ctx->initialized = true;
    return ANGLE_SENSOR_OK;
}

angle_sensor_error_t angle_sensor_deinit(angle_sensor_context_t* ctx)
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

bool angle_sensor_is_initialized(angle_sensor_context_t* ctx)
{
    if (ctx == NULL) {
        return false;
    }
    return ctx->initialized;
}

float angle_sensor_get_electrical_angle(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return 0.0f;
    }

    const struct sensor_config* cfg = ctx->config;
    float angle;

    if (cfg->device_get_angle_rad) {
        angle = cfg->device_get_angle_rad();
    } else if (cfg->device_get_raw_angle) {
        uint16_t raw = cfg->device_get_raw_angle();
        angle = ((float)raw / (float)cfg->resolution) * M_2PI;
    } else {
        return 0.0f;
    }

    angle += ctx->mechanical_offset;
    utils_norm_angle_0_2pi(&angle);
    return angle;
}

uint16_t angle_sensor_get_raw_angle(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return 0;
    }

    const struct sensor_config* cfg = ctx->config;

    if (cfg->device_get_raw_angle) {
        return cfg->device_get_raw_angle();
    }
    if (cfg->device_get_angle_rad) {
        float angle = cfg->device_get_angle_rad();
        utils_norm_angle_0_2pi(&angle);
        uint32_t raw = (uint32_t)((angle / M_2PI) * (float)cfg->resolution);
        return (uint16_t)(raw & 0xFFFF);
    }
    return 0;
}

float angle_sensor_get_mechanical_angle(angle_sensor_context_t* ctx)
{
    return angle_sensor_get_electrical_angle(ctx) / MOTOR_POLES;
}

float angle_sensor_get_velocity(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return 0.0f;
    }

    const struct sensor_config* cfg = ctx->config;

    if (cfg->device_get_velocity) {
        return cfg->device_get_velocity();
    }
    return 0.0f;
}

bool angle_sensor_is_calibrated(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return false;
    }

    const struct sensor_config* cfg = ctx->config;

    if (cfg->calibrate_is_done) {
        return cfg->calibrate_is_done();
    }
    return true;
}

angle_sensor_error_t angle_sensor_calibrate(angle_sensor_context_t* ctx)
{
    if (ctx == NULL) {
        return ANGLE_SENSOR_ERROR_NULL_PTR;
    }
    if (!ctx->initialized) {
        return ANGLE_SENSOR_ERROR_UNINITIALIZED;
    }

    const struct sensor_config* cfg = ctx->config;

    if (cfg->calibrate_start) {
        int ret = cfg->calibrate_start();
        if (ret != 0) {
            return ANGLE_SENSOR_ERROR_CALIBRATING;
        }
    }
    return ANGLE_SENSOR_OK;
}

void angle_sensor_update(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }

    const struct sensor_config* cfg = ctx->config;

    if (cfg->calibrate_update) {
        cfg->calibrate_update();
    }
}

void angle_sensor_get_info(angle_sensor_context_t* ctx,
    angle_sensor_info_t* info)
{
    if (ctx == NULL || info == NULL) {
        return;
    }
    if (!ctx->initialized) {
        memset(info, 0, sizeof(*info));
        return;
    }

    const struct sensor_config* cfg = ctx->config;

    info->type = cfg->type;
    info->resolution = cfg->resolution;
    info->pulses_per_rev = cfg->pulses_per_rev;
    info->poles = MOTOR_POLES;
    info->is_calibrated = cfg->calibrate_is_done
        ? cfg->calibrate_is_done()
        : true;
    info->offset = ctx->mechanical_offset;
}

void angle_sensor_set_offset(angle_sensor_context_t* ctx, float offset)
{
    if (ctx == NULL) {
        return;
    }

    ctx->mechanical_offset = offset;
    if (ctx->initialized) {
        ctx->info.offset = offset;
    }
}

angle_sensor_type_e angle_sensor_get_type(angle_sensor_context_t* ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return SENSOR_TYPE_NONE;
    }
    return ctx->info.type;
}

bool angle_sensor_is_active(angle_sensor_context_t* ctx)
{
    if (ctx == NULL) {
        return false;
    }
    return ctx->is_active;
}

/* Private functions ---------------------------------------------------------*/

static int sensor_generic_init(angle_sensor_context_t* ctx)
{
    const struct sensor_config* cfg = ctx->config;

    if (cfg->device_init) {
        cfg->device_init();
    }

    ctx->info.type = cfg->type;
    ctx->info.resolution = cfg->resolution;
    ctx->info.pulses_per_rev = cfg->pulses_per_rev;
    ctx->info.poles = MOTOR_POLES;
    ctx->info.is_calibrated = cfg->calibrate_is_done
        ? cfg->calibrate_is_done()
        : true;
    ctx->info.offset = ctx->mechanical_offset;

    if (cfg->type == SENSOR_TYPE_LINEAR_HALL) {
        fsm_linear_hall_init();
    }

    return 0;
}
