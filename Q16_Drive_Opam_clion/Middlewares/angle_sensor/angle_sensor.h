/**
 * @file    angle_sensor.h
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   角度传感器抽象层 — 统一传感器接口
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef ANGLE_SENSOR_H
#define ANGLE_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 传感器类型枚举
 */
typedef enum {
    SENSOR_TYPE_NONE = 0,    /**< 无传感器 */
    SENSOR_TYPE_MT6701,      /**< MT6701磁编码器(SPI) */
    SENSOR_TYPE_MT6816,      /**< MT6816磁编码器(SPI) */
    SENSOR_TYPE_LINEAR_HALL, /**< 线性霍尔传感器(PLL) */
    SENSOR_TYPE_MAX          /**< 传感器类型数量（哨兵） */
} angle_sensor_type_e;

/**
 * @brief 传感器状态枚举
 */
typedef enum {
    SENSOR_STATUS_OK = 0,      /**< 正常 */
    SENSOR_STATUS_ERROR,       /**< 错误 */
    SENSOR_STATUS_NOT_INIT,    /**< 未初始化 */
    SENSOR_STATUS_CALIBRATING, /**< 校准中 */
    SENSOR_STATUS_TIMEOUT      /**< 超时 */
} angle_sensor_status_e;

/**
 * @brief 角度传感器错误码枚举
 */
typedef enum {
    ANGLE_SENSOR_OK = 0,                    /**< 操作成功 */
    ANGLE_SENSOR_ERROR_NULL_PTR,            /**< 空指针错误 */
    ANGLE_SENSOR_ERROR_UNINITIALIZED,       /**< 未初始化 */
    ANGLE_SENSOR_ERROR_ALREADY_INITIALIZED, /**< 已初始化 */
    ANGLE_SENSOR_ERROR_NOT_SUPPORTED,       /**< 传感器不支持此操作 */
    ANGLE_SENSOR_ERROR_CALIBRATING,         /**< 传感器正在校准中 */
} angle_sensor_error_t;

/**
 * @brief 传感器信息结构体
 */
typedef struct {
    angle_sensor_type_e type; /**< 传感器类型 */
    uint16_t resolution;      /**< 角度分辨率(如14位为16384) */
    float pulses_per_rev;     /**< 每转脉冲数 */
    uint8_t poles;            /**< 电机极对数 */
    bool is_calibrated;       /**< 校准状态 */
    float offset;             /**< 机械偏移(弧度) */
} angle_sensor_info_t;

/**
 * @brief 传感器数据结构体（输出）
 */
typedef struct {
    float electrical_angle;       /**< 电角度(弧度, 0-2π) */
    float mechanical_angle;       /**< 机械角度(弧度, 0-2π) */
    float velocity;               /**< 角速度(rad/s) */
    uint32_t timestamp;           /**< 测量时间戳 */
    angle_sensor_status_e status; /**< 传感器状态 */
} angle_sensor_data_t;

/* 前向声明 — ops 和 context 相互引用 */
typedef struct angle_sensor_context angle_sensor_context_t;

/**
 * @brief 传感器操作表（函数指针多态）
 */
typedef struct angle_sensor_ops_s {
    int (*init)(angle_sensor_context_t *ctx);
    int (*calibrate)(angle_sensor_context_t *ctx);
    bool (*is_calibrated)(angle_sensor_context_t *ctx);
    uint16_t (*get_raw_angle)(angle_sensor_context_t *ctx);
    float (*get_angle_rad)(angle_sensor_context_t *ctx);
    float (*get_velocity_rads)(angle_sensor_context_t *ctx);
    void (*update)(angle_sensor_context_t *ctx);
    void (*get_info)(angle_sensor_context_t *ctx, angle_sensor_info_t *info);
    void (*set_offset)(angle_sensor_context_t *ctx, float offset);
} angle_sensor_ops_t;

/**
 * @brief 传感器上下文结构体
 */
struct angle_sensor_context {
    const angle_sensor_ops_t *ops; /**< 传感器操作表（由 init_context 设置） */
    angle_sensor_info_t info;      /**< 传感器信息缓存 */
    float mechanical_offset;       /**< 机械角度偏移（弧度） */
    bool is_active;                /**< 激活标志 */
    bool initialized;              /**< 初始化标志 */
};

/* Exported functions prototypes ---------------------------------------------*/

angle_sensor_error_t angle_sensor_init(angle_sensor_context_t *ctx);
angle_sensor_error_t angle_sensor_deinit(angle_sensor_context_t *ctx);
bool angle_sensor_is_initialized(angle_sensor_context_t *ctx);
angle_sensor_type_e angle_sensor_get_default_type(void);
angle_sensor_error_t angle_sensor_get_data(angle_sensor_context_t *ctx,
                                           angle_sensor_data_t *data);
float angle_sensor_get_electrical_angle(angle_sensor_context_t *ctx);
uint16_t angle_sensor_get_raw_angle(angle_sensor_context_t *ctx);
float angle_sensor_get_mechanical_angle(angle_sensor_context_t *ctx);
float angle_sensor_get_velocity(angle_sensor_context_t *ctx);
bool angle_sensor_is_calibrated(angle_sensor_context_t *ctx);
angle_sensor_error_t angle_sensor_calibrate(angle_sensor_context_t *ctx);
void angle_sensor_update(angle_sensor_context_t *ctx);
void angle_sensor_get_info(angle_sensor_context_t *ctx,
                           angle_sensor_info_t *info);
void angle_sensor_set_offset(angle_sensor_context_t *ctx, float offset);
angle_sensor_error_t angle_sensor_switch_type(angle_sensor_context_t *ctx,
                                              angle_sensor_type_e type);
angle_sensor_type_e angle_sensor_get_type(angle_sensor_context_t *ctx);
bool angle_sensor_is_active(angle_sensor_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ANGLE_SENSOR_H */
