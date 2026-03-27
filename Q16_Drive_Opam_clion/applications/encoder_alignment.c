/**
 * @file    encoder_alignment.c
 * @brief   电机编码器对齐和校准实现
 * @author  fubingyan
 * @contact qq:3245784484
 * @date    2024-08-19
 * @version V2.0.0
 *
 * @copyright Copyright (c) 2024 fubingyan. All Rights Reserved.
 *
 * @note    优化版本具有以下特点：
 *          - 改进的算法效率
 *          - 增强的错误处理
 *          - 更好的代码结构和可读性
 *          - 减少的计算开销
 */

#include "encoder_alignment.h"
#include <math.h>
#include <string.h>
#include "flash_task.h"

/*==============================================================================
 * 私有宏定义
 *============================================================================*/

/** 获取绝对值 */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/** 检查值是否在范围 [min, max] 内 */
#define IN_RANGE(val, min, max) (((val) >= (min)) && ((val) <= (max)))

const float inv_lines_per_step = 1.0f / LINES_PER_STEP;
const float wraparound_threshold = WRAPAROUND_THRESHOLD;
const float direction_threshold = DIRECTION_THRESHOLD;
const float max_delta = LINES_PER_STEP * STEP_SIZE_THRESHOLD;

/*==============================================================================
 * 全局变量
 *============================================================================*/

/** 主编码器校准实例 */
encoder_calibration_t g_encoder_calib = {
    .encoder_lines = ENCODER_RESOLUTION,
    .total_steps = MAX_MOTOR_STEPS,
    .is_valid = false,
    .direction = MOTOR_DIR_NONE,
};

/** Flash配置存储 */
motor_flash_config_t g_motor_flash_cfg;

/*==============================================================================
 * 私有函数声明
 *============================================================================*/

/**
 * @brief 验证和处理扇区角度增量
 *
 * @param[in]  angle_buffer  校准角度数组
 * @param[out] calib         要更新的校准结构
 *
 * @return 如果所有增量都有效返回true，否则返回false
 */
static bool validate_sector_deltas(const uint16_t *angle_buffer, encoder_calibration_t *calib);

/**
 * @brief 使用多个采样点计算平均方向
 *
 * @param[in] angle_buffer  校准角度数组
 * @param[in] calib         校准结构
 *
 * @return 基于统计分析检测到的方向
 */
static motor_direction_t calculate_direction_average(const uint16_t *angle_buffer, const encoder_calibration_t *calib);

/**
 * @brief 使用类似二分查找的方法查找当前扇区
 *
 * @param[in]  angle   当前角度
 * @param[in]  calib   校准结构
 * @param[out] sector  检测到的扇区索引
 *
 * @return 如果找到扇区返回true，否则返回false
 */
static bool find_current_sector(uint16_t angle, const encoder_calibration_t *calib, int16_t *sector);

/**
 * @brief 计算当前扇区内的电角度
 *
 * @param[in] angle  当前角度
 * @param[in] calib  校准结构
 *
 * @return 扇区内归一化的电角度 [0, 1)
 */
static float calculate_sector_angle(uint16_t angle, const encoder_calibration_t *calib);

/**
 * @brief 从Flash读取指定索引的校准角度
 *
 * 提供对角度校准查找表的安全访问，具有用于循环索引的自动环绕功能。
 *
 * @param[in] index  扇区索引（可以为负数或 >= MAX_MOTOR_STEPS）
 *
 * @return 指定索引处的校准角度值
 *
 * @note 索引自动环绕到有效范围 [0, MAX_MOTOR_STEPS)
 */
static inline uint16_t encoder_read_angle_lut(int16_t index)
{
    if (index < 0)
        index += MAX_MOTOR_STEPS;
    else if (index >= MAX_MOTOR_STEPS)
        index -= MAX_MOTOR_STEPS;
    return g_motor_flash_cfg.angle_map[index];
}

/**
 * @brief 获取当前校准有效性状态
 *
 * @param[in] calib  指向校准结构的指针
 *
 * @return 如果校准数据有效返回true，否则返回false
 */
static inline bool encoder_is_calibration_valid(const encoder_calibration_t *calib)
{
    return (calib != NULL) && calib->is_valid;
}
/**
 * @brief 将扇区索引环绕到有效范围 [0, max)
 *
 * @param[in] val  输入扇区索引
 * @param[in] max  扇区总数
 *
 * @return 有效范围内的环绕扇区索引
 */
static inline int16_t wrap_sector(int16_t val, int16_t max)
{
    if (val < 0)
        return val + max;
    if (val >= max)
        return val - max;
    return val;
}
/*==============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化编码器对齐模块
 * @return true 初始化成功 false 初始化失败
 */
bool encoder_alignment_init(void)
{
    // 从Flash加载校准数据
    const size_t bytes_read =
        ef_get_env_blob(FLASH_MAGIC_ENCODER, &g_motor_flash_cfg, sizeof(motor_flash_config_t), NULL);

    if (bytes_read != sizeof(motor_flash_config_t))
    {
        g_encoder_calib.is_valid = false;
        g_encoder_calib.direction = MOTOR_DIR_NONE;
        return false;
    }

    // 验证并处理校准数据
    if (!encoder_detect_direction(g_motor_flash_cfg.angle_map, &g_encoder_calib))
    {
        return false;
    }

    // 从Flash配置设置方向
    g_encoder_calib.direction = g_motor_flash_cfg.direction;

    return g_encoder_calib.is_valid;
}

/**
 * @brief 从校准数据检测电机旋转方向
 * @param[in] angle_buffer 校准角度数组
 * @param[out] calib 编码器校准数据结构
 * @return motor_direction_t 电机方向
 */
motor_direction_t encoder_detect_direction(const uint16_t *angle_buffer, encoder_calibration_t *calib)
{
    // 输入验证
    if (angle_buffer == NULL || calib == NULL)
    {
        if (calib != NULL)
        {
            calib->is_valid = false;
            calib->direction = MOTOR_DIR_NONE;
        }
        return MOTOR_DIR_NONE;
    }

    // 重置有效性标志
    calib->is_valid = true;
    calib->invalid_sector_index = -1;

    // 验证并计算扇区增量
    if (!validate_sector_deltas(angle_buffer, calib))
    {
        calib->is_valid = false;
        return MOTOR_DIR_NONE;
    }

    // 使用统计方法确定方向
    calib->direction = calculate_direction_average(angle_buffer, calib);

    return calib->direction;
}

/**
 * @brief 跟踪扇区并计算电角度
 * @param[in] raw_angle  原始编码器角度
 * @param[in] calib  编码器校准数据
 * @return float  归一化电角度 [0, 2π)
 */
float encoder_track_sector(uint16_t raw_angle, encoder_calibration_t *calib)
{
    // 输入验证
    if (calib == NULL || calib->direction == MOTOR_DIR_NONE)
    {
        return 0.0f;
    }

    // 对原始角度应用方向校正
    uint16_t corrected_angle;
    if (calib->direction == MOTOR_DIR_FORWARD)
    {
        corrected_angle = raw_angle & (calib->encoder_lines - 1);
    }
    else
    {
        corrected_angle = (calib->encoder_lines - 1 - raw_angle) & (calib->encoder_lines - 1);
    }

    // 查找当前扇区
    int16_t sector;
    if (!find_current_sector(corrected_angle, calib, &sector))
    {
        return calib->electrical_angle; // 返回最后有效的角度
    }

    calib->current_sector = sector;
    calib->current_step_delta = calib->step_delta[sector];

    // 计算扇区内的电角度
    const float sector_fraction = calculate_sector_angle(corrected_angle, calib);

    // 转换为电角度 [0, 2π)
    // Sector & 0x03 给出0-3，对应每个机械旋转的4个电周期
    const float sector_offset = (float)(sector & 0x03);
    calib->electrical_angle = (sector_fraction + sector_offset) * ELEC_ANGLE_FACTOR;

    return calib->electrical_angle;
}

/*==============================================================================
 * 私有函数实现
 *============================================================================*/

/**
 * @brief 验证扇区增量并处理环绕
 * @param angle_buffer 角度缓冲区
 * @param calib 编码器校准数据
 * @return true 有效扇区增量
 * @return false 无效扇区增量
 */
static bool validate_sector_deltas(const uint16_t *angle_buffer, encoder_calibration_t *calib)
{
    const int16_t encoder_lines = calib->encoder_lines;

    calib->zero_offset = 0;

    for (int16_t i = 0; i < calib->total_steps; i++)
    {
        // 计算原始增量
        int32_t delta = (int32_t)angle_buffer[i + 1] - (int32_t)angle_buffer[i];

        // 处理编码器边界处的环绕
        if (delta < -max_delta)
        {
            delta += encoder_lines;
            calib->zero_offset = i; // 记录环绕位置
        }
        else if (delta > max_delta)
        {
            delta -= encoder_lines;
            calib->zero_offset = i;
        }

        calib->step_delta[i] = (int16_t)delta;

        // 验证增量是否在合理范围内
        if (calib->step_delta[i] <= 0 || calib->step_delta[i] > encoder_lines)
        {
            calib->invalid_sector_index = i;
            return false;
        }
    }

    return true;
}

/**
 * @brief 使用稳健的统计方法计算方向
 * @param angle_buffer 角度缓冲区
 * @param calib 编码器校准数据
 * @return motor_direction_t 电机方向
 */
static motor_direction_t calculate_direction_average(const uint16_t *angle_buffer, const encoder_calibration_t *calib)
{
    const int16_t half_steps = calib->total_steps >> 1;
    const int16_t total_steps = calib->total_steps;

    int32_t forward_sum = 0;
    int32_t reverse_sum = 0;

    // 采样多个点以进行稳健的方向检测
    for (int i = 0; i < DIRECTION_SAMPLE_COUNT; i++)
    {
        // 正向采样（从中间区域向前）
        const int16_t fwd_idx1 = wrap_sector(half_steps + i, total_steps);
        const int16_t fwd_idx2 = wrap_sector(half_steps + i + 1, total_steps);
        forward_sum += (int32_t)angle_buffer[fwd_idx2] - (int32_t)angle_buffer[fwd_idx1];

        // 反向采样（从中间区域向后）
        const int16_t rev_idx1 = wrap_sector(half_steps - i - 1, total_steps);
        const int16_t rev_idx2 = wrap_sector(half_steps - i, total_steps);
        reverse_sum += (int32_t)angle_buffer[rev_idx2] - (int32_t)angle_buffer[rev_idx1];
    }

    // 计算平均值
    const int32_t forward_avg = forward_sum / DIRECTION_SAMPLE_COUNT;
    const int32_t reverse_avg = reverse_sum / DIRECTION_SAMPLE_COUNT;

    // 带滞回的方向确定
    if (forward_avg > reverse_avg + direction_threshold)
    {
        return MOTOR_DIR_FORWARD;
    }
    else if (reverse_avg > forward_avg + direction_threshold)
    {
        return MOTOR_DIR_REVERSE;
    }

    // 备用方案：如果平均值太接近，使用单点比较
    const int16_t test_idx = (calib->zero_offset > half_steps) ? (half_steps - 2) : (half_steps + 1);
    const int32_t fallback_delta = (int32_t)angle_buffer[test_idx + 1] - (int32_t)angle_buffer[test_idx];

    return (fallback_delta > 0) ? MOTOR_DIR_FORWARD : MOTOR_DIR_REVERSE;
}

/**
 * @brief 使用优化搜索查找扇区
 * @param angle 原始角度
 * @param calib 编码器校准数据
 * @param sector 输出当前扇区索引
 * @return true 找到有效扇区
 * @return false 未找到有效扇区
 */
static bool find_current_sector(uint16_t angle, const encoder_calibration_t *calib, int16_t *sector)
{
    // 预估扇区 - 只做一次除法
    int16_t est = (int16_t)(angle * inv_lines_per_step) + calib->zero_offset;

    // 快速边界处理 - 用条件替代取余
    if (est < 0)
        est += calib->total_steps;
    else if (est >= calib->total_steps)
        est -= calib->total_steps;

    const int16_t max_steps = calib->total_steps;

    // 循环展开 + 缓存 LUT 值
    for (int16_t offset = -1; offset <= 1; offset++)
    {
        int16_t test_sec = est + offset;
        // 一次性读取两个 LUT 值
        int16_t next_sec = test_sec + 1;

        const uint16_t sec_start = encoder_read_angle_lut(test_sec);
        const uint16_t sec_end = encoder_read_angle_lut(next_sec);

        if (sec_end > sec_start)
        {
            if (IN_RANGE(angle, sec_start, sec_end))
            {
                *sector = test_sec;
                return true;
            }
        }
        else
        {
            if (angle >= sec_start || angle <= sec_end)
            {
                *sector = test_sec;
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief 计算扇区内的归一化角度
 * @param angle 原始角度
 * @param calib 编码器校准数据
 * @return float 归一化角度
 */
static float calculate_sector_angle(uint16_t angle, const encoder_calibration_t *calib)
{
    const int16_t sector = calib->current_sector;
    const uint16_t sector_start = encoder_read_angle_lut(sector);
    const uint16_t sector_end = encoder_read_angle_lut(sector + 1);
    const int16_t step_delta = calib->current_step_delta;

    // 防止除以零
    if (step_delta <= 0)
    {
        return 0.0f;
    }

    int32_t angle_in_sector;

    if (sector_end > sector_start)
    {
        // 正常情况：无环绕
        angle_in_sector = (int32_t)angle - (int32_t)sector_start;
    }
    else
    {
        // 环绕情况：检查边界哪一侧
        if (angle >= wraparound_threshold)
        {
            // 角度接近编码器范围的末尾
            angle_in_sector = (int32_t)angle - (int32_t)sector_start;
        }
        else
        {
            // 角度环绕到开头
            angle_in_sector = (int32_t)calib->encoder_lines - (int32_t)sector_start + (int32_t)angle;
        }
    }

    // 归一化到扇区内的 [0, 1) 范围
    return (float)angle_in_sector / (float)step_delta;
}

// end of the file !