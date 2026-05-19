/**
 * @file    foc_sensor.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-20
 * @brief   FOC 传感器抽象层实现（统一编码器和霍尔传感器 API）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 */

/* Includes ------------------------------------------------------------------*/
#include "foc_sensor.h"

#include "foc_config_q16.h"
#include "hall_adjustment.h"
#include "MT6816.h"
#include "perf_counter.h"
#include "public.h"

/* Private constants ---------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/**
 * @brief 传感器上下文结构体（模块内部定义）
 */
struct foc_sensor_context {
    foc_sensor_config_t config; /**< 配置参数（嵌套） */
    foc_sensor_ops_t ops;       /**< 传感器操作表 */
    foc_sensor_info_t info;     /**< 传感器信息缓存 */
    bool is_active;             /**< 激活标志 */
    bool initialized;           /**< 初始化标志 */
};

/* Private variables ---------------------------------------------------------*/

/** @brief 传感器模块上下文（单例） */
static foc_sensor_context_t g_ctx;

/** @brief 机械角度偏移 */
static float g_mechanical_offset = 0.0f;

/* Private function prototypes -----------------------------------------------*/

// MT6701 传感器操作
static int sensor_mt6701_init(void);
static int sensor_mt6701_calibrate(void);
static bool sensor_mt6701_is_calibrated(void);
static uint16_t sensor_mt6701_get_raw_angle(void);
static float sensor_mt6701_get_angle_rad(void);
static float sensor_mt6701_get_velocity_rads(void);
static void sensor_mt6701_update(void);
static void sensor_mt6701_get_info(foc_sensor_info_t *info);
static void sensor_mt6701_set_offset(float offset);

// MT6816 传感器操作
static int sensor_mt6816_init(void);
static int sensor_mt6816_calibrate(void);
static bool sensor_mt6816_is_calibrated(void);
static uint16_t sensor_mt6816_get_raw_angle(void);
static float sensor_mt6816_get_angle_rad(void);
static float sensor_mt6816_get_velocity_rads(void);
static void sensor_mt6816_update(void);
static void sensor_mt6816_get_info(foc_sensor_info_t *info);
static void sensor_mt6816_set_offset(float offset);

// 线性霍尔传感器操作
static int sensor_linear_hall_init(void);
static int sensor_linear_hall_calibrate(void);
static bool sensor_linear_hall_is_calibrated(void);
static uint16_t sensor_linear_hall_get_raw_angle(void);
static float sensor_linear_hall_get_angle_rad(void);
static float sensor_linear_hall_get_velocity_rads(void);
static void sensor_linear_hall_update(void);
static void sensor_linear_hall_get_info(foc_sensor_info_t *info);
static void sensor_linear_hall_set_offset(float offset);

// 传感器操作表
static const foc_sensor_ops_t sensor_ops_mt6701 = {
    .init = sensor_mt6701_init,
    .calibrate = sensor_mt6701_calibrate,
    .is_calibrated = sensor_mt6701_is_calibrated,
    .get_raw_angle = sensor_mt6701_get_raw_angle,
    .get_angle_rad = sensor_mt6701_get_angle_rad,
    .get_velocity_rads = sensor_mt6701_get_velocity_rads,
    .update = sensor_mt6701_update,
    .get_info = sensor_mt6701_get_info,
    .set_offset = sensor_mt6701_set_offset,
};

static const foc_sensor_ops_t sensor_ops_mt6816 = {
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

static const foc_sensor_ops_t sensor_ops_linear_hall = {
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

/** @brief 传感器操作表索引数组 */
static const foc_sensor_ops_t *sensor_ops_table[SENSOR_TYPE_MAX] = {
    [SENSOR_TYPE_NONE] = NULL,
    [SENSOR_TYPE_MT6701] = &sensor_ops_mt6701,
    [SENSOR_TYPE_MT6816] = &sensor_ops_mt6816,
    [SENSOR_TYPE_LINEAR_HALL] = &sensor_ops_linear_hall,
};

// 传感器激活辅助函数（初始化 / 切换的公共逻辑）
static foc_sensor_error_t sensor_do_activate(foc_sensor_type_e type);

/* Exported functions --------------------------------------------------------*/

foc_sensor_error_t foc_sensor_init(const foc_sensor_config_t *config)
{
    if (!config) {
        return FOC_SENSOR_ERROR_NULL_PTR;
    }
    if (config->type >= SENSOR_TYPE_MAX) {
        return FOC_SENSOR_ERROR_INVALID_TYPE;
    }

    // 如果已初始化，先反初始化
    if (g_ctx.initialized) {
        foc_sensor_deinit();
    }

    g_ctx.config = *config;

    foc_sensor_error_t ret = sensor_do_activate(config->type);
    if (ret != FOC_SENSOR_OK) {
        return ret;
    }

    g_ctx.initialized = true;
    return FOC_SENSOR_OK;
}

foc_sensor_error_t foc_sensor_deinit(void)
{
    g_ctx.is_active = false;
    g_ctx.initialized = false;
    g_mechanical_offset = 0.0f;
    memset(&g_ctx.config, 0, sizeof(g_ctx.config));
    memset(&g_ctx.ops, 0, sizeof(g_ctx.ops));
    memset(&g_ctx.info, 0, sizeof(g_ctx.info));

    return FOC_SENSOR_OK;
}

bool foc_sensor_is_initialized(void)
{
    return g_ctx.initialized;
}

foc_sensor_type_e foc_sensor_get_default_type(void)
{
    return SENSOR_TYPE_MT6701;
}

foc_sensor_error_t foc_sensor_get_data(foc_sensor_data_t *data)
{
    if (!data) {
        return FOC_SENSOR_ERROR_NULL_PTR;
    }
    if (!g_ctx.initialized) {
        return FOC_SENSOR_ERROR_UNINITIALIZED;
    }

    data->electrical_angle =
        g_ctx.ops.get_angle_rad ? g_ctx.ops.get_angle_rad() : 0.0f;
    data->mechanical_angle = data->electrical_angle / MOTOR_POLES;
    data->velocity =
        g_ctx.ops.get_velocity_rads ? g_ctx.ops.get_velocity_rads() : 0.0f;
    data->timestamp = get_system_ticks();
    data->status =
        g_ctx.ops.is_calibrated && g_ctx.ops.is_calibrated()
            ? SENSOR_STATUS_OK
            : SENSOR_STATUS_CALIBRATING;

    return FOC_SENSOR_OK;
}

float foc_sensor_get_electrical_angle(void)
{
    if (!g_ctx.initialized || !g_ctx.ops.get_angle_rad) {
        return 0.0f;
    }
    return g_ctx.ops.get_angle_rad();
}

uint16_t foc_sensor_get_raw_angle(void)
{
    if (!g_ctx.initialized || !g_ctx.ops.get_raw_angle) {
        return 0;
    }
    return g_ctx.ops.get_raw_angle();
}

float foc_sensor_get_mechanical_angle(void)
{
    return foc_sensor_get_electrical_angle() / MOTOR_POLES;
}

float foc_sensor_get_velocity(void)
{
    if (!g_ctx.initialized || !g_ctx.ops.get_velocity_rads) {
        return 0.0f;
    }
    return g_ctx.ops.get_velocity_rads();
}

bool foc_sensor_is_calibrated(void)
{
    if (!g_ctx.initialized || !g_ctx.ops.is_calibrated) {
        return false;
    }
    return g_ctx.ops.is_calibrated();
}

foc_sensor_error_t foc_sensor_calibrate(void)
{
    if (!g_ctx.initialized) {
        return FOC_SENSOR_ERROR_UNINITIALIZED;
    }
    if (!g_ctx.ops.calibrate) {
        return FOC_SENSOR_ERROR_NOT_SUPPORTED;
    }
    int ret = g_ctx.ops.calibrate();
    if (ret != 0) {
        return FOC_SENSOR_ERROR_CALIBRATING;
    }
    return FOC_SENSOR_OK;
}

void foc_sensor_update(void)
{
    if (!g_ctx.initialized || !g_ctx.ops.update) {
        return;
    }
    g_ctx.ops.update();
}

void foc_sensor_get_info(foc_sensor_info_t *info)
{
    if (!info) {
        return;
    }
    if (!g_ctx.initialized) {
        memset(info, 0, sizeof(*info));
        return;
    }
    if (g_ctx.ops.get_info) {
        g_ctx.ops.get_info(info);
    }
}

void foc_sensor_set_offset(float offset)
{
    g_mechanical_offset = offset;
    if (g_ctx.initialized && g_ctx.ops.set_offset) {
        g_ctx.ops.set_offset(offset);
    }
}

foc_sensor_error_t foc_sensor_switch_type(foc_sensor_type_e type)
{
    if (!g_ctx.initialized) {
        return FOC_SENSOR_ERROR_UNINITIALIZED;
    }
    if (type >= SENSOR_TYPE_MAX) {
        return FOC_SENSOR_ERROR_INVALID_TYPE;
    }
    if (g_ctx.config.type == type) {
        return FOC_SENSOR_OK;
    }

    g_ctx.is_active = false;
    g_ctx.config.type = type;

    foc_sensor_error_t ret = sensor_do_activate(type);
    if (ret != FOC_SENSOR_OK) {
        return ret;
    }

    g_ctx.is_active = true;
    return FOC_SENSOR_OK;
}

foc_sensor_type_e foc_sensor_get_type(void)
{
    if (!g_ctx.initialized) {
        return SENSOR_TYPE_NONE;
    }
    return g_ctx.config.type;
}

bool foc_sensor_is_active(void)
{
    return g_ctx.is_active;
}

/* Private functions ---------------------------------------------------------*/

static foc_sensor_error_t sensor_do_activate(foc_sensor_type_e type)
{
    if (!sensor_ops_table[type]) {
        return FOC_SENSOR_ERROR_INVALID_TYPE;
    }

    g_ctx.ops = *sensor_ops_table[type];

    if (g_ctx.ops.init) {
        int ret = g_ctx.ops.init();
        if (ret != 0) {
            return FOC_SENSOR_ERROR_NOT_SUPPORTED;
        }
    }

    g_ctx.is_active = true;
    return FOC_SENSOR_OK;
}

/* ==================== MT6701 传感器实现 ==================== */

static int sensor_mt6701_init(void)
{
    MT6816_Init();
    g_ctx.info.type = SENSOR_TYPE_MT6701;
    g_ctx.info.resolution = 16384;
    g_ctx.info.pulses_per_rev = 16384.0f;
    g_ctx.info.poles = MOTOR_POLES;
    g_ctx.info.is_calibrated = true;
    g_ctx.info.offset = g_mechanical_offset;
    return 0;
}

static int sensor_mt6701_calibrate(void)
{
    return 0;
}

static bool sensor_mt6701_is_calibrated(void)
{
    return true;
}

static uint16_t sensor_mt6701_get_raw_angle(void)
{
    return REIN_MT6701_Get_AngleData();
}

static float sensor_mt6701_get_angle_rad(void)
{
    uint16_t raw = sensor_mt6701_get_raw_angle();
    float angle = ((float)raw / 16384.0f) * M_2PI;
    angle += g_mechanical_offset;
    while (angle > M_2PI) {
        angle -= M_2PI;
    }
    while (angle < 0.0f) {
        angle += M_2PI;
    }
    return angle;
}

static float sensor_mt6701_get_velocity_rads(void)
{
    return 0.0f;
}

static void sensor_mt6701_update(void)
{
}

static void sensor_mt6701_get_info(foc_sensor_info_t *info)
{
    if (info) {
        info->type = SENSOR_TYPE_MT6701;
        info->resolution = 16384;
        info->pulses_per_rev = 16384.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = true;
        info->offset = g_mechanical_offset;
    }
}

static void sensor_mt6701_set_offset(float offset)
{
    g_mechanical_offset = offset;
    g_ctx.info.offset = offset;
}

/* ==================== MT6816 传感器实现 ==================== */

static int sensor_mt6816_init(void)
{
    MT6816_Init();
    g_ctx.info.type = SENSOR_TYPE_MT6816;
    g_ctx.info.resolution = 16384;
    g_ctx.info.pulses_per_rev = 16384.0f;
    g_ctx.info.poles = MOTOR_POLES;
    g_ctx.info.is_calibrated = true;
    g_ctx.info.offset = g_mechanical_offset;
    return 0;
}

static int sensor_mt6816_calibrate(void)
{
    return 0;
}

static bool sensor_mt6816_is_calibrated(void)
{
    return true;
}

static uint16_t sensor_mt6816_get_raw_angle(void)
{
    return REIN_MT6816_Get_AngleData();
}

static float sensor_mt6816_get_angle_rad(void)
{
    uint16_t raw = sensor_mt6816_get_raw_angle();
    float angle = ((float)raw / 16384.0f) * M_2PI;
    angle += g_mechanical_offset;
    while (angle > M_2PI) {
        angle -= M_2PI;
    }
    while (angle < 0.0f) {
        angle += M_2PI;
    }
    return angle;
}

static float sensor_mt6816_get_velocity_rads(void)
{
    return 0.0f;
}

static void sensor_mt6816_update(void)
{
}

static void sensor_mt6816_get_info(foc_sensor_info_t *info)
{
    if (info) {
        info->type = SENSOR_TYPE_MT6816;
        info->resolution = 16384;
        info->pulses_per_rev = 16384.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = true;
        info->offset = g_mechanical_offset;
    }
}

static void sensor_mt6816_set_offset(float offset)
{
    g_mechanical_offset = offset;
    g_ctx.info.offset = offset;
}

/* ==================== 线性霍尔传感器实现 ==================== */

static int sensor_linear_hall_init(void)
{
    hall_adjust_init();
    g_ctx.info.type = SENSOR_TYPE_LINEAR_HALL;
    g_ctx.info.resolution = 65535;
    g_ctx.info.pulses_per_rev = 65535.0f;
    g_ctx.info.poles = MOTOR_POLES;
    g_ctx.info.is_calibrated = hall_adjust_is_calibrated();
    g_ctx.info.offset = g_mechanical_offset;
    return 0;
}

static int sensor_linear_hall_calibrate(void)
{
    if (hall_adjust_get_state() == HALL_ADJUST_STATE_NONE) {
        hall_adjust_start_calibration();
        return 0;
    }
    return -1;
}

static bool sensor_linear_hall_is_calibrated(void)
{
    return hall_adjust_is_calibrated();
}

static uint16_t sensor_linear_hall_get_raw_angle(void)
{
    float angle_rad = pll_get_angle(&pll_ctx);
    while (angle_rad > M_2PI) {
        angle_rad -= M_2PI;
    }
    while (angle_rad < 0.0f) {
        angle_rad += M_2PI;
    }
    uint32_t raw = (uint32_t)((angle_rad / M_2PI) * 65536.0f);
    return (uint16_t)(raw & 0xFFFF);
}

static float sensor_linear_hall_get_angle_rad(void)
{
    float angle = pll_get_angle(&pll_ctx);
    angle += g_mechanical_offset;
    while (angle > M_2PI) {
        angle -= M_2PI;
    }
    while (angle < 0.0f) {
        angle += M_2PI;
    }
    float electrical_angle = angle * MOTOR_POLES;
    while (electrical_angle > M_2PI) {
        electrical_angle -= M_2PI;
    }
    while (electrical_angle < 0.0f) {
        electrical_angle += M_2PI;
    }
    return electrical_angle;
}

static float sensor_linear_hall_get_velocity_rads(void)
{
    return pll_get_speed(&pll_ctx);
}

static void sensor_linear_hall_update(void)
{
    if (!hall_adjust_is_calibrated()) {
        hall_adjust_task();
    }
}

static void sensor_linear_hall_get_info(foc_sensor_info_t *info)
{
    if (info) {
        info->type = SENSOR_TYPE_LINEAR_HALL;
        info->resolution = 65535;
        info->pulses_per_rev = 65535.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = hall_adjust_is_calibrated();
        info->offset = g_mechanical_offset;
    }
}

static void sensor_linear_hall_set_offset(float offset)
{
    g_mechanical_offset = offset;
    g_ctx.info.offset = offset;
}

