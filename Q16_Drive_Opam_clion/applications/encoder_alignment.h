/**
 * @file    encoder_alignment.h
 * @brief   电机编码器对齐和校准模块
 * @author  fubingyan
 * @contact qq:3245784484
 * @date    2024-08-19
 * @version V2.0.0
 *
 * @copyright Copyright (c) 2024 fubingyan. All Rights Reserved.
 *
 * @note    优化版本，具有改进的性能和代码结构
 */

#ifndef ENCODER_ALIGNMENT_H
#define ENCODER_ALIGNMENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "foc_config_q16.h"
#include <stdbool.h>
#include <stdint.h>

/*==============================================================================
 * Configuration Constants
 *============================================================================*/

/** 编码器分辨率（线数） */
#define ENCODER_RESOLUTION 0x4000

/** 电机运动最大步数 */
#define MAX_MOTOR_STEPS (int16_t)(MOVE_STEP_NUM)

/** 每步对应的线数（预计算常量） */
#define LINES_PER_STEP ((float)ENCODER_RESOLUTION / MAX_MOTOR_STEPS)

/** 步长验证的阈值乘数 */
#define STEP_SIZE_THRESHOLD 2.0f

/** 方向检测的采样数量 */
#define DIRECTION_SAMPLE_COUNT 4

/** 方向检测阈值（步长的1/4） */
#define DIRECTION_THRESHOLD (LINES_PER_STEP * 0.25f)

/** 电角度转换因子（π/2） */
#define ELEC_ANGLE_FACTOR 1.57079632679489661923f

/** 环绕检测阈值 */
#define WRAPAROUND_THRESHOLD (ENCODER_RESOLUTION - (int)(LINES_PER_STEP * 2))

    /*==============================================================================
     * Type Definitions
     *============================================================================*/

    /**
     * @brief 电机旋转方向枚举
     */
    typedef enum __attribute__((packed))
    {
        MOTOR_DIR_REVERSE = -1, /**< 逆时针旋转 */
        MOTOR_DIR_NONE = 0,     /**< 无方向/未初始化 */
        MOTOR_DIR_FORWARD = 1   /**< 顺时针旋转 */
    } motor_direction_t;

    /**
     * @brief 存储在Flash中的电机校准数据
     */
    typedef struct
    {
        uint16_t angle_map[MAX_MOTOR_STEPS + 1]; /**< 角度校准查找表 */
        motor_direction_t direction;             /**< 电机旋转方向 */
    } motor_flash_config_t;

    /**
     * @brief FOC校准运行时数据结构
     */
    typedef struct
    {
        float electrical_angle;              /**< 当前电角度 [0, 2π) */
        motor_direction_t direction;         /**< 编码器安装方向 */
        int16_t current_sector;              /**< 当前电扇区索引 */
        int16_t zero_offset;                 /**< 零位偏移（偏置） */
        int16_t step_delta[MAX_MOTOR_STEPS]; /**< 每个扇区的角度增量 */
        int16_t current_step_delta;          /**< 当前扇区增量值 */
        int16_t encoder_lines;               /**< 总编码器线数 */
        int16_t total_steps;                 /**< 总校准步数 */
        bool is_valid;                       /**< 校准数据有效性标志 */
        int16_t invalid_sector_index;        /**< 第一个无效扇区的索引 */
    } encoder_calibration_t;

    /*==============================================================================
     * 全局变量
     *============================================================================*/

    /** 主编码器校准实例 */
    extern encoder_calibration_t g_encoder_calib;

    /** Flash配置数据 */
    extern motor_flash_config_t g_motor_flash_cfg;

    /*==============================================================================
     * 公共函数声明
     *============================================================================*/

    /**
     * @brief 初始化编码器对齐模块
     *
     * 从Flash存储器加载校准数据并初始化编码器校准结构。
     *
     * @return 初始化成功返回true，否则返回false
     */
    bool encoder_alignment_init(void);

    /**
     * @brief 检测并验证电机旋转方向
     *
     * 分析校准角度缓冲区以确定电机的旋转方向并验证校准数据的完整性。
     *
     * @param[in]  angle_buffer  指向校准角度数组的指针
     * @param[out] calib         指向要更新的校准结构的指针
     *
     * @return 检测到的电机方向（MOTOR_DIR_FORWARD 或 MOTOR_DIR_REVERSE）
     * @retval MOTOR_DIR_NONE 如果检测失败或参数无效
     *
     * @note 根据数据验证设置 calib->is_valid 标志
     */
    motor_direction_t encoder_detect_direction(const uint16_t *angle_buffer, encoder_calibration_t *calib);

    /**
     * @brief 跟踪当前扇区并计算电角度
     *
     * 基于编码器位置确定当前电扇区，并计算用于FOC控制的精确电角度。
     *
     * @param[in]     raw_angle  原始编码器角度读数
     * @param[in,out] calib      指向校准结构的指针
     *
     * @return 计算的电角度，单位为弧度 [0, 2π)
     * @retval 0.0f 如果方向未初始化或参数无效
     *
     * @note 更新 calib->current_sector 和 calib->electrical_angle
     */
    float encoder_track_sector(uint16_t raw_angle, encoder_calibration_t *calib);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_ALIGNMENT_H */