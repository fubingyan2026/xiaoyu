/**
 * @file    foc_encoder.c
 * @brief   FOC 编码器校准与扇区跟踪实现
 *
 * 提供编码器校准数据的加载、验证、方向检测，以及基于角度
 * 校准查找表（LUT）的实时扇区跟踪和电角度计算。
 *
 * 核心算法：
 * - 校准阶段：根据编码器在不同电气角度下的读数建立角度映射表
 * - 检测阶段：统计分析确定编码器安装方向（正向/反向）
 * - 跟踪阶段：通过预估+邻近验证快速定位当前扇区，计算精确电角度
 *
 * @author  FOC Development Team
 * @date    2026-05-31
 * @version V3.0.0
 */

#include "foc_encoder.h"

#include <math.h>
#include <string.h>

/*==============================================================================
 * 私有函数声明
 *============================================================================*/

/**
 * @brief 验证扇区增量并处理环绕
 *
 * 计算相邻校准点的角度差（delta），处理编码器 0 点处的环绕跃变，
 * 并检查每个 delta 是否在合理范围内。同时记录环绕发生的位置（zero_offset）。
 *
 * @param[in]  enc           编码器上下文（输出 step_delta[] 和 zero_offset）
 * @param[in]  angle_buffer  角度校准数组，长度为 total_steps + 1
 * @return true  所有扇区增量有效
 * @return false 存在无效扇区增量（invalid_sector_index 记录位置）
 */
static bool validate_sector_deltas(foc_encoder_t* enc, const uint16_t* angle_buffer);

/**
 * @brief 使用多点统计平均检测编码器安装方向
 *
 * 在中间区域分别采样正向和反向两个方向的增量序列并计算平均值，
 * 通过比较正反向平均增量的大小差异确定方向，并加入滞回阈值避免抖动。
 *
 * @param[in]  angle_buffer  角度校准数组
 * @param[in]  enc           编码器上下文（用于获取 total_steps、zero_offset）
 * @return foc_encoder_dir_t 检测到的安装方向
 */
static foc_encoder_dir_t calculate_dir_average(const uint16_t* angle_buffer, const foc_encoder_t* enc);

/**
 * @brief 通过预估+邻近搜索查找当前扇区
 *
 * 先根据编码器角度值预估所在扇区索引，然后在预估位置附近
 * （-1, 0, +1）进行精确匹配验证，避免全表遍历。
 * 处理了编码器边界环绕和扇区分界处的判断。
 *
 * @param[in]  enc     编码器上下文
 * @param[in]  angle   当前角度（已应用方向校正）
 * @param[out] sector  找到的扇区索引
 * @return true  成功定位扇区
 * @return false 未找到匹配扇区（返回最后有效电角度）
 */
static bool find_current_sector(foc_encoder_t* enc, uint16_t angle, int16_t* sector);

/**
 * @brief 计算当前扇区内的归一化角度 [0, 1)
 *
 * 根据编码器在当前扇区内的位置，计算相对于扇区起始点的归一化比例，
 * 处理了扇区跨越编码器 0 点时的环绕情况。
 *
 * @param[in]  enc    编码器上下文
 * @param[in]  angle  当前角度
 * @return float 扇区内归一化角度 [0, 1)
 */
static float calc_sector_angle(const foc_encoder_t* enc, uint16_t angle);

/**
 * @brief 扇区索引环绕到有效范围 [0, max)
 *
 * @param[in] val  输入索引
 * @param[in] max  扇区总数
 * @return int16_t 有效范围内的索引
 */
static inline int16_t wrap_sector(int16_t val, int16_t max);

/*==============================================================================
 * 公共函数实现
 *============================================================================*/

bool foc_encoder_init(foc_encoder_t* enc, const uint16_t* angle_map, foc_encoder_dir_t flash_dir)
{
    if (enc == NULL || angle_map == NULL) {
        return false;
    }

    /* 清零整个上下文，确保所有字段从确定状态开始 */
    memset(enc, 0, sizeof(foc_encoder_t));

    /* 设置固定参数 */
    enc->encoder_lines = ENCODER_RESOLUTION;
    enc->total_steps = MAX_MOTOR_STEPS;
    enc->is_valid = false;
    enc->direction = FOC_ENCODER_DIR_NONE;

    /* 将 Flash 加载的角度映射表复制到上下文 */
    memcpy(enc->angle_map, angle_map, sizeof(uint16_t) * (MAX_MOTOR_STEPS + 1));
    enc->flash_direction = flash_dir;

    /* 通过方向检测验证校准数据的完整性，同时得到安装方向 */
    if (!foc_encoder_detect_direction(enc, enc->angle_map)) {
        return false;
    }

    /* 从 Flash 配置恢复运行时方向 */
    enc->direction = enc->flash_direction;

    return enc->is_valid;
}

foc_encoder_dir_t foc_encoder_detect_direction(foc_encoder_t* enc, const uint16_t* angle_buffer)
{
    if (angle_buffer == NULL || enc == NULL) {
        if (enc != NULL) {
            enc->is_valid = false;
            enc->direction = FOC_ENCODER_DIR_NONE;
        }
        return FOC_ENCODER_DIR_NONE;
    }

    /* 重置校验状态 */
    enc->is_valid = true;
    enc->invalid_sector_index = -1;

    /* 第一步：验证扇区增量数据，同时建立 step_delta 表 */
    if (!validate_sector_deltas(enc, angle_buffer)) {
        enc->is_valid = false;
        return FOC_ENCODER_DIR_NONE;
    }

    /* 第二步：通过统计平均确定编码器安装方向 */
    enc->direction = calculate_dir_average(angle_buffer, enc);

    return enc->direction;
}

void foc_encoder_reverse_angle_map(foc_encoder_t* enc)
{
    if (enc == NULL) {
        return;
    }
    /* 将每个校准点的角度值取反：angle_map[i] = encoder_lines - angle_map[i]
     * 使得反向安装的编码器数据与正向格式一致 */
    for (int16_t i = 0; i <= enc->total_steps; i++) {
        enc->angle_map[i] = (uint16_t)(enc->encoder_lines - enc->angle_map[i]);
    }
}

void foc_encoder_calibration_finalize(foc_encoder_t* enc)
{
    if (enc == NULL) {
        return;
    }

    /* 第一步：验证校准数据完整性，同时检测编码器安装方向 */
    foc_encoder_detect_direction(enc, enc->angle_map);
    enc->flash_direction = enc->direction;

    /* 第二步：若编码器反向安装，反转角度映射表使其归一化 */
    if (enc->direction == FOC_ENCODER_DIR_REVERSE) {
        foc_encoder_reverse_angle_map(enc);
        /* 反转后步进增量发生变化，重新计算 step_delta，
         * 但不改变 direction（保留原始反向值供运行时校正使用） */
        validate_sector_deltas(enc, enc->angle_map);
    }
}

float foc_encoder_track_sector(foc_encoder_t* enc, uint16_t raw_angle)
{
    if (enc == NULL || enc->direction == FOC_ENCODER_DIR_NONE) {
        return 0.0f;
    }

    /* 根据安装方向对原始角度做校正：
     * - 正向安装：直接截取到编码器分辨率范围
     * - 反向安装：用 encoder_lines - 1 减去原始值实现取反 */
    uint16_t corrected_angle;
    if (enc->direction == FOC_ENCODER_DIR_FORWARD) {
        corrected_angle = raw_angle & (enc->encoder_lines - 1);
    } else {
        corrected_angle = (uint16_t)(enc->encoder_lines - 1 - raw_angle) & (uint16_t)(enc->encoder_lines - 1);
    }

    /* 通过校准查找表定位当前所在的扇区 */
    int16_t sector;
    if (!find_current_sector(enc, corrected_angle, &sector)) {
        return enc->electrical_angle; /* 未找到时返回上次有效值 */
    }

    enc->current_sector = sector;
    enc->current_step_delta = enc->step_delta[sector];

    /* 计算在当前扇区内的归一化位置 [0, 1) */
    float sector_fraction = calc_sector_angle(enc, corrected_angle);

    /* 转换为电角度 [0, 2π)：
     * 每个机械扇区对应 90° 电角度（sector & 0x03 得到 0~3） */
    float sector_offset = (float)(sector & 0x03);
    enc->electrical_angle = (sector_fraction + sector_offset) * ELEC_ANGLE_FACTOR;

    return enc->electrical_angle;
}

/*==============================================================================
 * 私有函数实现
 *============================================================================*/

static bool validate_sector_deltas(foc_encoder_t* enc, const uint16_t* angle_buffer)
{
    int16_t encoder_lines = enc->encoder_lines;
    float max_delta = LINES_PER_STEP * STEP_SIZE_THRESHOLD;

    enc->zero_offset = 0;

    for (int16_t i = 0; i < enc->total_steps; i++) {
        /* 计算相邻校准点的差值 */
        int32_t delta = (int32_t)angle_buffer[i + 1] - (int32_t)angle_buffer[i];

        /* 处理编码器 0 点处的环绕：
         * 如果差值超出正常范围（大于 max_delta 或小于 -max_delta），
         * 说明跨越了编码器的 0 点，需要加减 encoder_lines 还原真实差值 */
        if (delta < -max_delta) {
            delta += encoder_lines;
            enc->zero_offset = i; /* 记录环绕位置 */
        } else if (delta > max_delta) {
            delta -= encoder_lines;
            enc->zero_offset = i;
        }

        enc->step_delta[i] = (int16_t)delta;

        /* 验证增量有效：必须 > 0（单调递增）且不超过编码器总分辨率 */
        if (enc->step_delta[i] <= 0 || enc->step_delta[i] > encoder_lines) {
            enc->invalid_sector_index = i;
            return false;
        }
    }

    return true;
}

static foc_encoder_dir_t calculate_dir_average(const uint16_t* angle_buffer, const foc_encoder_t* enc)
{
    int16_t half_steps = enc->total_steps >> 1;
    int16_t total_steps = enc->total_steps;

    int32_t forward_sum = 0;
    int32_t reverse_sum = 0;

    /* 在中间区域采样正向和反向两个方向的增量：
     * 正向：从 half_steps 往后采 DIRECTION_SAMPLE_COUNT 个点
     * 反向：从 half_steps 往前采 DIRECTION_SAMPLE_COUNT 个点
     * 若正向安装，正向增量大于反向；反向安装则相反 */
    for (int i = 0; i < DIRECTION_SAMPLE_COUNT; i++) {
        int16_t fwd_idx1 = wrap_sector(half_steps + i, total_steps);
        int16_t fwd_idx2 = wrap_sector(half_steps + i + 1, total_steps);
        forward_sum += (int32_t)angle_buffer[fwd_idx2] - (int32_t)angle_buffer[fwd_idx1];

        int16_t rev_idx1 = wrap_sector(half_steps - i - 1, total_steps);
        int16_t rev_idx2 = wrap_sector(half_steps - i, total_steps);
        reverse_sum += (int32_t)angle_buffer[rev_idx2] - (int32_t)angle_buffer[rev_idx1];
    }

    int32_t forward_avg = forward_sum / DIRECTION_SAMPLE_COUNT;
    int32_t reverse_avg = reverse_sum / DIRECTION_SAMPLE_COUNT;

    /* 加入滞回阈值 DIRECTION_THRESHOLD，避免临界值抖动 */
    if (forward_avg > reverse_avg + DIRECTION_THRESHOLD) {
        return FOC_ENCODER_DIR_FORWARD;
    } else if (reverse_avg > forward_avg + DIRECTION_THRESHOLD) {
        return FOC_ENCODER_DIR_REVERSE;
    }

    /* 备选方案：平均值接近时，使用 zero_offset 附近的单点做最终判断 */
    int16_t test_idx = (enc->zero_offset > half_steps) ? (half_steps - 2) : (half_steps + 1);
    int32_t fallback_delta = (int32_t)angle_buffer[test_idx + 1] - (int32_t)angle_buffer[test_idx];

    return (fallback_delta > 0) ? FOC_ENCODER_DIR_FORWARD : FOC_ENCODER_DIR_REVERSE;
}

static bool find_current_sector(foc_encoder_t* enc, uint16_t angle, int16_t* sector)
{
    const float inv_lines_per_step = 1.0f / LINES_PER_STEP;
    /* 预估扇区：通过角度值 / 每步线数 + zero_offset 快速定位 */
    int16_t est = (int16_t)(angle * inv_lines_per_step) + enc->zero_offset;

    /* 边界处理：将预估索引限制在 [0, total_steps) 范围内 */
    if (est < 0)
        est += enc->total_steps;
    else if (est >= enc->total_steps)
        est -= enc->total_steps;

    int16_t max_steps = enc->total_steps;

    /* 在预估位置附近（-1, 0, +1）逐一遍历验证，
     * 避免了遍历整个查找表的开销 */
    for (int16_t offset = -1; offset <= 1; offset++) {
        int16_t test_sec = est + offset;
        /* 扇区索引环绕 */
        if (test_sec < 0)
            test_sec += max_steps;
        else if (test_sec >= max_steps)
            test_sec -= max_steps;

        int16_t next_sec = test_sec + 1;
        uint16_t sec_start, sec_end;

        /* 安全检查：防止越界访问 */
        if (test_sec >= 0 && test_sec < max_steps) {
            sec_start = enc->angle_map[test_sec];
            sec_end = enc->angle_map[next_sec];
        } else {
            continue;
        }

        uint16_t wraparound_threshold_val = (uint16_t)WRAPAROUND_THRESHOLD;

        /* 判断角度是否在当前扇区内：
         * - sec_end > sec_start：正常情况，角度在 [sec_start, sec_end]
         * - sec_end <= sec_start：扇区跨越编码器 0 点，用或条件判断 */
        if (sec_end > sec_start) {
            if (angle >= sec_start && angle <= sec_end) {
                *sector = test_sec;
                return true;
            }
        } else {
            if (angle >= sec_start || angle <= wraparound_threshold_val) {
                *sector = test_sec;
                return true;
            }
        }
    }

    return false; /* 三个候选扇区均不匹配 */
}

static float calc_sector_angle(const foc_encoder_t* enc, uint16_t angle)
{
    int16_t sector = enc->current_sector;
    int16_t step_delta = enc->current_step_delta;

    /* 防止除以零 */
    if (step_delta <= 0) {
        return 0.0f;
    }

    uint16_t sec_start = enc->angle_map[sector];
    uint16_t next_sector = sector + 1;
    uint16_t sec_end = enc->angle_map[next_sector];

    int32_t angle_in_sector;

    int16_t encoder_lines = enc->encoder_lines;
    uint16_t wraparound_threshold_val = (uint16_t)WRAPAROUND_THRESHOLD;

    if (sec_end > sec_start) {
        /* 正常情况：角度在 [sec_start, sec_end) 范围内，直接减起始值 */
        angle_in_sector = (int32_t)angle - (int32_t)sec_start;
    } else {
        /* 环绕情况：扇区跨越编码器 0 点
         * 判断角度位于 0 点的哪一侧 */
        if (angle >= wraparound_threshold_val) {
            /* 角度在 0 点之后的末尾侧 */
            angle_in_sector = (int32_t)angle - (int32_t)sec_start;
        } else {
            /* 角度在 0 点之后的起始侧，需要加上编码器总分辨率 */
            angle_in_sector = (int32_t)encoder_lines - (int32_t)sec_start + (int32_t)angle;
        }
    }

    /* 归一化到 [0, 1) */
    return (float)angle_in_sector / (float)step_delta;
}

static inline int16_t wrap_sector(int16_t val, int16_t max)
{
    if (val < 0)
        return val + max;
    if (val >= max)
        return val - max;
    return val;
}
