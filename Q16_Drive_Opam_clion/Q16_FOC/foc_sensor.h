/**
 * @brief: FOC传感器抽象层 - 统一传感器接口
 * @FilePath: foc_sensor.h
 * @author: fubingyan qq:3245784484
 * @Date: 2026-02-14
 * @version: V2.0.0
 * @note: 抽象编码器和霍尔传感器的统一API
 * @copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef FOC_SENSOR_H
#define FOC_SENSOR_H

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
} foc_sensor_type_e;

/**
 * @brief 传感器状态枚举
 */
typedef enum {
    SENSOR_STATUS_OK = 0,      /**< 正常 */
    SENSOR_STATUS_ERROR,       /**< 错误 */
    SENSOR_STATUS_NOT_INIT,    /**< 未初始化 */
    SENSOR_STATUS_CALIBRATING, /**< 校准中 */
    SENSOR_STATUS_TIMEOUT      /**< 超时 */
} foc_sensor_status_e;

/**
 * @brief FOC 传感器错误码枚举
 */
typedef enum {
    FOC_SENSOR_OK = 0,                    /**< 操作成功 */
    FOC_SENSOR_ERROR_NULL_PTR,            /**< 空指针错误 */
    FOC_SENSOR_ERROR_UNINITIALIZED,       /**< 未初始化 */
    FOC_SENSOR_ERROR_ALREADY_INITIALIZED, /**< 已初始化 */
    FOC_SENSOR_ERROR_INVALID_TYPE,        /**< 无效传感器类型 */
    FOC_SENSOR_ERROR_NOT_SUPPORTED,       /**< 传感器不支持此操作 */
    FOC_SENSOR_ERROR_CALIBRATING,         /**< 传感器正在校准中 */
} foc_sensor_error_t;

/**
 * @brief 传感器信息结构体
 */
typedef struct {
    foc_sensor_type_e type; /**< 传感器类型 */
    uint16_t resolution;    /**< 角度分辨率(如14位为16384) */
    float pulses_per_rev;   /**< 每转脉冲数 */
    uint8_t poles;          /**< 电机极对数 */
    bool is_calibrated;     /**< 校准状态 */
    float offset;           /**< 机械偏移(弧度) */
} foc_sensor_info_t;

/**
 * @brief 传感器数据结构体（输出）
 */
typedef struct {
    float electrical_angle;     /**< 电角度(弧度, 0-2π) */
    float mechanical_angle;     /**< 机械角度(弧度, 0-2π) */
    float velocity;             /**< 角速度(rad/s) */
    uint32_t timestamp;         /**< 测量时间戳 */
    foc_sensor_status_e status; /**< 传感器状态 */
} foc_sensor_data_t;

/**
 * @brief 传感器操作表（函数指针多态）
 */
typedef struct foc_sensor_ops_s {
    int (*init)(void);                         /**< 初始化传感器 */
    int (*calibrate)(void);                    /**< 开始校准 */
    bool (*is_calibrated)(void);               /**< 检查校准完成 */
    uint16_t (*get_raw_angle)(void);           /**< 获取原始角度 */
    float (*get_angle_rad)(void);              /**< 获取角度(弧度) */
    float (*get_velocity_rads)(void);          /**< 获取速度(rad/s) */
    void (*update)(void);                      /**< 更新传感器(快循环中调用) */
    void (*get_info)(foc_sensor_info_t *info); /**< 获取传感器信息 */
    void (*set_offset)(float offset);          /**< 设置机械偏移 */
} foc_sensor_ops_t;

/**
 * @brief 传感器配置结构体
 */
typedef struct {
    foc_sensor_type_e type; /**< 传感器类型 */
} foc_sensor_config_t;

/**
 * @brief 传感器上下文结构体（前向声明）
 */
typedef struct foc_sensor_context foc_sensor_context_t;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化传感器子系统
 * @param config 传感器配置指针
 * @return 操作结果错误码
 */
foc_sensor_error_t foc_sensor_init(const foc_sensor_config_t *config);

/**
 * @brief 反初始化传感器子系统
 * @return 操作结果错误码
 */
foc_sensor_error_t foc_sensor_deinit(void);

/**
 * @brief 检查传感器是否已初始化
 * @return true表示已初始化
 */
bool foc_sensor_is_initialized(void);

/**
 * @brief 获取默认传感器类型
 * @return 默认传感器类型
 */
foc_sensor_type_e foc_sensor_get_default_type(void);

/**
 * @brief 获取传感器数据（统一接口）
 * @param data 数据结构体指针（输出）
 * @return 操作结果错误码
 */
foc_sensor_error_t foc_sensor_get_data(foc_sensor_data_t *data);

/**
 * @brief 获取电角度（最常用）
 * @return 电角度(弧度)，未初始化返回0.0f
 */
float foc_sensor_get_electrical_angle(void);

/**
 * @brief 获取原始角度值
 * @return 原始角度(0-resolution)，未初始化返回0
 */
uint16_t foc_sensor_get_raw_angle(void);

/**
 * @brief 获取机械角度
 * @return 机械角度(弧度)，未初始化返回0.0f
 */
float foc_sensor_get_mechanical_angle(void);

/**
 * @brief 获取角速度
 * @return 速度(rad/s)，未初始化返回0.0f
 */
float foc_sensor_get_velocity(void);

/**
 * @brief 检查传感器是否已校准
 * @return 已校准返回true，未初始化返回false
 */
bool foc_sensor_is_calibrated(void);

/**
 * @brief 启动传感器校准
 * @return 操作结果错误码
 */
foc_sensor_error_t foc_sensor_calibrate(void);

/**
 * @brief 更新传感器（在快循环中调用）
 */
void foc_sensor_update(void);

/**
 * @brief 获取传感器信息
 * @param info 信息结构体指针（输出）
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
 * @return 操作结果错误码
 */
foc_sensor_error_t foc_sensor_switch_type(foc_sensor_type_e type);

/**
 * @brief 获取当前传感器类型
 * @return 当前传感器类型，未初始化返回SENSOR_TYPE_NONE
 */
foc_sensor_type_e foc_sensor_get_type(void);

/**
 * @brief 检查传感器是否激活
 * @return 已激活返回true
 */
bool foc_sensor_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* FOC_SENSOR_H */
