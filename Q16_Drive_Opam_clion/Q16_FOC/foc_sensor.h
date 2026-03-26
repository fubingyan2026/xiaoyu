/**
 * @brief: FOC传感器抽象层 - 统一传感器接口
 * @FilePath: foc_sensor.h
 * @author: fubingyan qq:3245784484
 * @Date: 2026-02-14
 * @version: V1.0.0
 * @note: 抽象编码器和霍尔传感器的统一API
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef FOC_SENSOR_H
#define FOC_SENSOR_H

#include "stdbool.h"
#include <stdint.h>

/* 传感器类型枚举 */
typedef enum
{
    SENSOR_TYPE_NONE = 0,    /* 无传感器 */
    SENSOR_TYPE_MT6701,      /* MT6701磁编码器(SPI) */
    SENSOR_TYPE_MT6816,      /* MT6816磁编码器(SPI) */
    SENSOR_TYPE_LINEAR_HALL, /* 线性霍尔传感器(PLL) */
    SENSOR_TYPE_HALL,        /* 传统霍尔传感器(3-bit) */
    SENSOR_TYPE_MAX
} foc_sensor_type_e;

/* 传感器状态枚举 */
typedef enum
{
    SENSOR_STATUS_OK = 0,      /* 正常 */
    SENSOR_STATUS_ERROR,       /* 错误 */
    SENSOR_STATUS_NOT_INIT,    /* 未初始化 */
    SENSOR_STATUS_CALIBRATING, /* 校准中 */
    SENSOR_STATUS_TIMEOUT      /* 超时 */
} foc_sensor_status_e;

/* 传感器信息结构体 */
typedef struct
{
    foc_sensor_type_e type; /* 传感器类型 */
    uint16_t resolution;    /* 角度分辨率(如14位为16384) */
    float pulses_per_rev;   /* 每转脉冲数 */
    uint8_t poles;          /* 电机极对数 */
    bool is_calibrated;     /* 校准状态 */
    float offset;           /* 机械偏移(弧度) */
} foc_sensor_info_t;

/* 传感器数据结构体(输出) */
typedef struct
{
    float electrical_angle;     /* 电角度(弧度,0-2π) */
    float mechanical_angle;     /* 机械角度(弧度,0-2π) */
    float velocity;             /* 角速度(rad/s) */
    uint32_t timestamp;         /* 测量时间戳 */
    foc_sensor_status_e status; /* 传感器状态 */
} foc_sensor_data_t;

/* 传感器操作结构体(函数指针表模式) */
typedef struct foc_sensor_ops_s
{
    int (*init)(void);                         /* 初始化传感器 */
    int (*calibrate)(void);                    /* 开始校准 */
    bool (*is_calibrated)(void);               /* 检查校准完成 */
    uint16_t (*get_raw_angle)(void);           /* 获取原始角度 */
    float (*get_angle_rad)(void);              /* 获取角度(弧度) */
    float (*get_velocity_rads)(void);          /* 获取速度(rad/s) */
    void (*update)(void);                      /* 更新传感器(在ISR或快循环中调用) */
    void (*get_info)(foc_sensor_info_t *info); /* 获取传感器信息 */
    void (*set_offset)(float offset);          /* 设置机械偏移 */
} foc_sensor_ops_t;

/* 传感器句柄结构体 */
typedef struct
{
    foc_sensor_type_e type; /* 当前传感器类型 */
    foc_sensor_ops_t ops;   /* 传感器操作 */
    foc_sensor_info_t info; /* 传感器信息 */
    foc_sensor_data_t data; /* 最近读取的数据 */
    bool is_active;         /* 激活标志 */
} foc_sensor_handle_t;

/* 公共API */

/**
 * @brief 初始化传感器子系统
 * @param type 传感器类型
 * @return 0成功,负数失败
 */
int foc_sensor_init(foc_sensor_type_e type);

/**
 * @brief 获取默认传感器类型
 * @return 默认传感器类型
 */
foc_sensor_type_e foc_sensor_get_default_type(void);

/**
 * @brief 获取当前传感器句柄
 * @return 传感器句柄指针
 */
foc_sensor_handle_t *foc_sensor_get_handle(void);

/**
 * @brief 获取传感器数据(统一接口)
 * @param data 数据结构体指针
 * @return 0成功,负数失败
 */
int foc_sensor_get_data(foc_sensor_data_t *data);

/**
 * @brief 获取电角度(最常用)
 * @return 电角度(弧度)
 */
float foc_sensor_get_electrical_angle(void);

/**
 * @brief 获取原始角度值
 * @return 原始角度(0-resolution)
 */
uint16_t foc_sensor_get_raw_angle(void);

/**
 * @brief 获取机械角度
 * @return 机械角度(弧度)
 */
float foc_sensor_get_mechanical_angle(void);

/**
 * @brief 获取角速度
 * @return 速度(rad/s)
 */
float foc_sensor_get_velocity(void);

/**
 * @brief 检查传感器是否已校准
 * @return 已校准返回true
 */
bool foc_sensor_is_calibrated(void);

/**
 * @brief 启动传感器校准
 * @return 0成功,负数失败
 */
int foc_sensor_calibrate(void);

/**
 * @brief 更新传感器(在快循环中调用)
 */
void foc_sensor_update(void);

/**
 * @brief 获取传感器信息
 * @param info 信息结构体指针
 */
void foc_sensor_get_info(foc_sensor_info_t *info);

/**
 * @brief 设置机械偏移
 * @param offset 偏移量(弧度)
 */
void foc_sensor_set_offset(float offset);

/**
 * @brief 运行时切换传感器类型
 * @param type 新传感器类型
 * @return 0成功,负数失败
 */
int foc_sensor_switch_type(foc_sensor_type_e type);

/**
 * @brief 获取当前传感器类型
 * @return 当前传感器类型
 */
foc_sensor_type_e foc_sensor_get_type(void);

/**
 * @brief 检查传感器是否激活
 * @return 已激活返回true
 */
bool foc_sensor_is_active(void);

#endif
