/**
 * @file    foc_encoder.h
 * @brief   FOC 编码器校准与扇区跟踪模块
 *
 * 提供编码器校准数据的验证、电机旋转方向检测、
 * 以及基于校准查找表（LUT）的电气角度实时跟踪。
 *
 * 设计特点：
 *   - 实例化 API，支持多电机独立使用
 *   - 与 Flash 存储解耦：flash 数据由调用者加载后传入
 *   - 扇区搜索采用预估+邻近验证，避免全表遍历
 *
 * @author  FOC Development Team
 * @date    2026-05-31
 * @version V3.0.0
 */

#ifndef FOC_ENCODER_H
#define FOC_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "foc_config.h"

/*==============================================================================
 * 配置常量
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
 * 类型定义
 *============================================================================*/

/**
 * @brief 编码器旋转方向枚举
 */
typedef enum __attribute__((packed)) {
    FOC_ENCODER_DIR_REVERSE = -1, /**< 逆时针旋转 */
    FOC_ENCODER_DIR_NONE = 0,     /**< 无方向 / 未初始化 */
    FOC_ENCODER_DIR_FORWARD = 1   /**< 顺时针旋转 */
} foc_encoder_dir_t;

/**
 * @brief Flash 持久化的编码器校准数据
 *
 * 包含角度校准查找表和方向信息，由 flash_task.c 通过 EasyFlash 自动管理。
 * 与 foc_encoder_t 分离，仅包含需要持久化到 Flash 的字段。
 */
typedef struct {
    uint16_t angle_map[MAX_MOTOR_STEPS + 1]; /**< 角度校准查找表 */
    foc_encoder_dir_t direction;              /**< 电机旋转方向 */
} foc_encoder_flash_t;

/**
 * @brief 编码器运行时上下文
 *
 * 包含校准运行时状态和内嵌的 Flash 持久化数据。
 * 每个 FOC 实例拥有独立的上下文，支持多电机控制。
 */
typedef struct {
    // === Flash 持久化数据（嵌入而非指针，简化生命周期管理） ===
    uint16_t angle_map[MAX_MOTOR_STEPS + 1]; /**< 角度校准查找表 */
    foc_encoder_dir_t flash_direction;        /**< Flash 中存储的方向值 */

    // === 运行时状态 ===
    float electrical_angle;                   /**< 当前电角度 [0, 2π) */
    foc_encoder_dir_t direction;              /**< 运行时方向 */
    int16_t current_sector;                   /**< 当前电扇区索引 */
    int16_t zero_offset;                      /**< 零位偏移 */
    int16_t step_delta[MAX_MOTOR_STEPS];      /**< 每个扇区的增量 */
    int16_t current_step_delta;               /**< 当前扇区增量 */
    int16_t encoder_lines;                    /**< 总编码器线数 */
    int16_t total_steps;                      /**< 总校准步数 */
    bool is_valid;                            /**< 校准数据有效性标志 */
    int16_t invalid_sector_index;             /**< 第一个无效扇区的索引 */
} foc_encoder_t;

/*==============================================================================
 * 公共 API
 *============================================================================*/

/**
 * @brief 初始化编码器模块
 *
 * 从已加载的 Flash 校准数据中导入 angle_map 和方向，
 * 验证数据完整性并设置运行时状态。
 *
 * @param enc          编码器上下文指针
 * @param angle_map    预加载的 Flash angle_map 指针（foc_encoder_flash_t::angle_map）
 * @param flash_dir    预加载的 Flash 方向值（foc_encoder_flash_t::direction）
 * @return true         初始化成功（校准数据有效）
 * @return false        校准数据无效或参数为空
 */
bool foc_encoder_init(foc_encoder_t* enc, const uint16_t* angle_map, foc_encoder_dir_t flash_dir);

/**
 * @brief 检测并验证编码器旋转方向
 *
 * 分析校准角度缓冲区以确定旋转方向并验证数据的完整性。
 *
 * @param enc          编码器上下文指针
 * @param angle_buffer 校准角度数组（通常为 enc->angle_map）
 * @return foc_encoder_dir_t 检测到的方向
 */
foc_encoder_dir_t foc_encoder_detect_direction(foc_encoder_t* enc, const uint16_t* angle_buffer);

/**
 * @brief 跟踪当前扇区并计算电角度
 *
 * 基于编码器原始读数查找当前扇区，计算精确电角度用于 FOC 控制。
 * 该函数在 PWM 中断中调用，必须保持低延迟。
 *
 * @param enc        编码器上下文指针
 * @param raw_angle  编码器原始读数
 * @return float     电角度（弧度，范围 [0, 2π)）
 */
float foc_encoder_track_sector(foc_encoder_t* enc, uint16_t raw_angle);

/**
 * @brief 反转角度校准查找表
 *
 * 当编码器安装方向为反向时，将校准数据取反：
 *   angle_map[i] = encoder_lines - angle_map[i]
 * 使其与正向编码器的数据格式一致。
 *
 * @param enc 编码器上下文指针
 */
void foc_encoder_reverse_angle_map(foc_encoder_t* enc);

/**
 * @brief 完成编码器校准的收尾处理
 *
 * 在校准数据采集完成后调用，执行以下流程：
 *   1. 检测校准数据方向并验证完整性
 *   2. 若为反向安装，反转 angle_map 使其归一化
 *   3. 重新计算反转后的 step_delta
 *   4. 保留原始方向值用于运行时角度校正
 *
 * @param enc 编码器上下文指针
 */
void foc_encoder_calibration_finalize(foc_encoder_t* enc);

#endif /* FOC_ENCODER_H */
