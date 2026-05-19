/**
 * @brief: FOC传感器抽象层实现
 * @FilePath: foc_sensor.c
 * @author: fubingyan qq:3245784484
 * @Date: 2026-02-14
 * @version: V1.0.0
 * @note: 抽象编码器和霍尔传感器的统一API
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#include "foc_sensor.h"
#include "filter.h"
#include "maths.h"
#include "utils_math.h"
#include "MT6816.h"
#include "foc_config_q16.h"
#include "hall_adjustment.h"
#include "perf_counter.h"

/* 全局变量 */
static foc_sensor_handle_t g_sensor = {0};
static bool g_sensor_initialized = false;
static float g_mechanical_offset = 0.0f;

/* MT6701传感器操作函数声明 */
static int sensor_mt6701_init(void);
static int sensor_mt6701_calibrate(void);
static bool sensor_mt6701_is_calibrated(void);
static uint16_t sensor_mt6701_get_raw_angle(void);
static float sensor_mt6701_get_angle_rad(void);
static float sensor_mt6701_get_velocity_rads(void);
static void sensor_mt6701_update(void);
static void sensor_mt6701_get_info(foc_sensor_info_t *info);
static void sensor_mt6701_set_offset(float offset);

/* MT6816传感器操作函数声明 */
static int sensor_mt6816_init(void);
static int sensor_mt6816_calibrate(void);
static bool sensor_mt6816_is_calibrated(void);
static uint16_t sensor_mt6816_get_raw_angle(void);
static float sensor_mt6816_get_angle_rad(void);
static float sensor_mt6816_get_velocity_rads(void);
static void sensor_mt6816_update(void);
static void sensor_mt6816_get_info(foc_sensor_info_t *info);
static void sensor_mt6816_set_offset(float offset);

/* 线性霍尔传感器操作函数声明 */
static int sensor_linear_hall_init(void);
static int sensor_linear_hall_calibrate(void);
static bool sensor_linear_hall_is_calibrated(void);
static uint16_t sensor_linear_hall_get_raw_angle(void);
static float sensor_linear_hall_get_angle_rad(void);
static float sensor_linear_hall_get_velocity_rads(void);
static void sensor_linear_hall_update(void);
static void sensor_linear_hall_get_info(foc_sensor_info_t *info);
static void sensor_linear_hall_set_offset(float offset);

/* 空传感器操作函数声明 */
static int sensor_none_init(void);
static int sensor_none_calibrate(void);
static bool sensor_none_is_calibrated(void);
static uint16_t sensor_none_get_raw_angle(void);
static float sensor_none_get_angle_rad(void);
static float sensor_none_get_velocity_rads(void);
static void sensor_none_update(void);
static void sensor_none_get_info(foc_sensor_info_t *info);
static void sensor_none_set_offset(float offset);

/* MT6701传感器操作表 */
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

/* MT6816传感器操作表 */
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

/* 线性霍尔传感器操作表 */
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

/* 空传感器操作表 */
static const foc_sensor_ops_t sensor_ops_none = {
    .init = sensor_none_init,
    .calibrate = sensor_none_calibrate,
    .is_calibrated = sensor_none_is_calibrated,
    .get_raw_angle = sensor_none_get_raw_angle,
    .get_angle_rad = sensor_none_get_angle_rad,
    .get_velocity_rads = sensor_none_get_velocity_rads,
    .update = sensor_none_update,
    .get_info = sensor_none_get_info,
    .set_offset = sensor_none_set_offset,
};

/* 传感器操作表数组 */
static const foc_sensor_ops_t *sensor_ops_table[SENSOR_TYPE_MAX] = {
    [SENSOR_TYPE_NONE] = &sensor_ops_none,     [SENSOR_TYPE_MT6701] = &sensor_ops_mt6701,
    [SENSOR_TYPE_MT6816] = &sensor_ops_mt6816, [SENSOR_TYPE_LINEAR_HALL] = &sensor_ops_linear_hall,
};

/* ==================== 空传感器实现 ==================== */
static int sensor_none_init(void)
{
    (void)0;
    return 0;
}

static int sensor_none_calibrate(void)
{
    return -1;
}

static bool sensor_none_is_calibrated(void)
{
    return false;
}

static uint16_t sensor_none_get_raw_angle(void)
{
    return 0;
}

static float sensor_none_get_angle_rad(void)
{
    return 0.0f;
}

static float sensor_none_get_velocity_rads(void)
{
    return 0.0f;
}

static void sensor_none_update(void)
{
}

static void sensor_none_get_info(foc_sensor_info_t *info)
{
    if (info)
    {
        info->type = SENSOR_TYPE_NONE;
        info->resolution = 0;
        info->pulses_per_rev = 0;
        info->poles = MOTOR_POLES;
        info->is_calibrated = false;
        info->offset = 0.0f;
    }
}

static void sensor_none_set_offset(float offset)
{
    (void)offset;
}

/* ==================== MT6701传感器实现 ==================== */
static int sensor_mt6701_init(void)
{
    MT6816_Init();
    g_sensor.info.type = SENSOR_TYPE_MT6701;
    g_sensor.info.resolution = 16384;
    g_sensor.info.pulses_per_rev = 16384.0f;
    g_sensor.info.poles = MOTOR_POLES;
    g_sensor.info.is_calibrated = true;
    g_sensor.info.offset = g_mechanical_offset;
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
    while (angle > M_2PI)
        angle -= M_2PI;
    while (angle < 0.0f)
        angle += M_2PI;
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
    if (info)
    {
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
    g_sensor.info.offset = offset;
}

/* ==================== MT6816传感器实现 ==================== */
static int sensor_mt6816_init(void)
{
    MT6816_Init();
    g_sensor.info.type = SENSOR_TYPE_MT6816;
    g_sensor.info.resolution = 16384;
    g_sensor.info.pulses_per_rev = 16384.0f;
    g_sensor.info.poles = MOTOR_POLES;
    g_sensor.info.is_calibrated = true;
    g_sensor.info.offset = g_mechanical_offset;
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
    while (angle > M_2PI)
        angle -= M_2PI;
    while (angle < 0.0f)
        angle += M_2PI;
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
    if (info)
    {
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
    g_sensor.info.offset = offset;
}

/* ==================== 线性霍尔传感器实现 ==================== */
static int sensor_linear_hall_init(void)
{
    hall_adjust_init();
    g_sensor.info.type = SENSOR_TYPE_LINEAR_HALL;
    g_sensor.info.resolution = 65535;
    g_sensor.info.pulses_per_rev = 65535.0f;
    g_sensor.info.poles = MOTOR_POLES;
    g_sensor.info.is_calibrated = (hall_adjust_is_calibrated());
    g_sensor.info.offset = g_mechanical_offset;
    return 0;
}

static int sensor_linear_hall_calibrate(void)
{
    if (hall_adjust_get_state() == HALL_ADJUST_STATE_NONE)
    {
        hall_adjust_start_calibration();
        return 0;
    }
    return -1;
}

static bool sensor_linear_hall_is_calibrated(void)
{
    return (hall_adjust_is_calibrated());
}

static uint16_t sensor_linear_hall_get_raw_angle(void)
{
    float angle_rad = pll_get_angle(&pll_ctx);
    while (angle_rad > M_2PI)
        angle_rad -= M_2PI;
    while (angle_rad < 0.0f)
        angle_rad += M_2PI;
    uint32_t raw = (uint32_t)((angle_rad / M_2PI) * 65536.0f);
    return (uint16_t)(raw & 0xFFFF);
}

static float sensor_linear_hall_get_angle_rad(void)
{
    float angle = pll_get_angle(&pll_ctx);
    angle += g_mechanical_offset;
    while (angle > M_2PI)
        angle -= M_2PI;
    while (angle < 0.0f)
        angle += M_2PI;
    float electrical_angle = angle * MOTOR_POLES;
    while (electrical_angle > M_2PI)
        electrical_angle -= M_2PI;
    while (electrical_angle < 0.0f)
        electrical_angle += M_2PI;
    return electrical_angle;
}

static float sensor_linear_hall_get_velocity_rads(void)
{
    return pll_get_speed(&pll_ctx);
}

static void sensor_linear_hall_update(void)
{
    if (!hall_adjust_is_calibrated())
    {
        hall_adjust_task();
    }
}

static void sensor_linear_hall_get_info(foc_sensor_info_t *info)
{
    if (info)
    {
        info->type = SENSOR_TYPE_LINEAR_HALL;
        info->resolution = 65535;
        info->pulses_per_rev = 65535.0f;
        info->poles = MOTOR_POLES;
        info->is_calibrated = (hall_adjust_is_calibrated());
        info->offset = g_mechanical_offset;
    }
}

static void sensor_linear_hall_set_offset(float offset)
{
    g_mechanical_offset = offset;
    g_sensor.info.offset = offset;
}

/* ==================== 公共API实现 ==================== */
int foc_sensor_init(foc_sensor_type_e type)
{
    if (type >= SENSOR_TYPE_MAX)
    {
        return -1;
    }

    g_sensor.type = type;
    g_sensor.ops = *sensor_ops_table[type];

    if (g_sensor.ops.init)
    {
        int ret = g_sensor.ops.init();
        if (ret != 0)
        {
            return ret;
        }
    }

    g_sensor.is_active = true;
    g_sensor_initialized = true;

    return 0;
}

foc_sensor_type_e foc_sensor_get_default_type(void)
{
    return SENSOR_TYPE_MT6701;
}

foc_sensor_handle_t *foc_sensor_get_handle(void)
{
    return &g_sensor;
}

int foc_sensor_get_data(foc_sensor_data_t *data)
{
    if (!data || !g_sensor_initialized)
    {
        return -1;
    }

    data->electrical_angle = g_sensor.ops.get_angle_rad ? g_sensor.ops.get_angle_rad() : 0.0f;
    data->mechanical_angle = data->electrical_angle / MOTOR_POLES;
    data->velocity = g_sensor.ops.get_velocity_rads ? g_sensor.ops.get_velocity_rads() : 0.0f;
    data->timestamp = get_system_ticks();
    data->status =
        g_sensor.ops.is_calibrated && g_sensor.ops.is_calibrated() ? SENSOR_STATUS_OK : SENSOR_STATUS_CALIBRATING;

    return 0;
}

float foc_sensor_get_electrical_angle(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.get_angle_rad)
    {
        return 0.0f;
    }
    return g_sensor.ops.get_angle_rad();
}

uint16_t foc_sensor_get_raw_angle(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.get_raw_angle)
    {
        return 0;
    }
    return g_sensor.ops.get_raw_angle();
}

float foc_sensor_get_mechanical_angle(void)
{
    return foc_sensor_get_electrical_angle() / MOTOR_POLES;
}

float foc_sensor_get_velocity(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.get_velocity_rads)
    {
        return 0.0f;
    }
    return g_sensor.ops.get_velocity_rads();
}

bool foc_sensor_is_calibrated(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.is_calibrated)
    {
        return false;
    }
    return g_sensor.ops.is_calibrated();
}

int foc_sensor_calibrate(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.calibrate)
    {
        return -1;
    }
    return g_sensor.ops.calibrate();
}

void foc_sensor_update(void)
{
    if (!g_sensor_initialized || !g_sensor.ops.update)
    {
        return;
    }
    g_sensor.ops.update();
}

void foc_sensor_get_info(foc_sensor_info_t *info)
{
    if (!g_sensor_initialized || !info)
    {
        return;
    }
    if (g_sensor.ops.get_info)
    {
        g_sensor.ops.get_info(info);
    }
}

void foc_sensor_set_offset(float offset)
{
    g_mechanical_offset = offset;
    if (g_sensor.ops.set_offset)
    {
        g_sensor.ops.set_offset(offset);
    }
}

int foc_sensor_switch_type(foc_sensor_type_e type)
{
    if (type >= SENSOR_TYPE_MAX)
    {
        return -1;
    }

    if (g_sensor.type == type)
    {
        return 0;
    }

    g_sensor.is_active = false;
    g_sensor.type = type;
    g_sensor.ops = *sensor_ops_table[type];

    if (g_sensor.ops.init)
    {
        int ret = g_sensor.ops.init();
        if (ret != 0)
        {
            return ret;
        }
    }

    g_sensor.is_active = true;

    return 0;
}

foc_sensor_type_e foc_sensor_get_type(void)
{
    return g_sensor.type;
}

bool foc_sensor_is_active(void)
{
    return g_sensor.is_active;
}
