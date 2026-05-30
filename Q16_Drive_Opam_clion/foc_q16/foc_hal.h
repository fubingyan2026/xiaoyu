/**
 * @file    foc_hal.h
 * @brief   FOC 硬件抽象层（HAL）- 统一硬件接口
 *
 * 提供 FOC 控制与硬件之间的抽象接口，实现硬件无关的 FOC 核心逻辑。
 * 采用回调机制（函数指针表），支持不同硬件平台的快速移植。
 *
 * 设计思路：
 *   - 所有硬件操作（ADC 采样、PWM 输出、编码器读取等）通过回调函数完成
 *   - foc_hal_config_t 结构体集中管理所有回调指针和硬件参数
 *   - foc_hal_t 管理器封装初始化状态检查和回调分发逻辑
 *   - 移植新平台时只需实现回调函数，FOC 核心算法无需修改
 *
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V4.0.0
 */

#ifndef FOC_HAL_H
#define FOC_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foc_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 回调函数类型定义 ==================== */

/**
 * @brief ADC 电流采样回调函数类型
 *
 * 读取三相电流的 ADC 原始值并转换为 Q16.16 格式。
 * 转换公式：I_q = (ADC_raw - offset) * CURRENT_SAMPLE_FACTOR
 * 其中 offset 为 ADC 零电流偏置，CURRENT_SAMPLE_FACTOR 为转换因子。
 *
 * @param[out] ia A 相电流输出指针（Q16.16 格式）
 * @param[out] ib B 相电流输出指针（Q16.16 格式）
 * @param[out] ic C 相电流输出指针（Q16.16 格式）
 */
typedef void (*foc_hal_adc_read_cb_t)(q16_16_t* ia, q16_16_t* ib, q16_16_t* ic);

/**
 * @brief ADC 初始化回调函数类型
 *
 * 初始化 ADC 外设：配置采样通道、触发源（PWM 定时器）、分辨率、
 * 采样时间等参数。通常在 foc_hal_init 之后调用 foc_hal_adc_init 触发。
 */
typedef void (*foc_hal_adc_init_cb_t)(void);

/**
 * @brief PWM 输出回调函数类型
 *
 * 将占空比计数值写入硬件定时器比较寄存器。
 * 输入值已由 foc_hal_pwm_update 转换为定时器计数值。
 *
 * @param ta A 相占空比计数值（0 ~ PWM_PERIOD）
 * @param tb B 相占空比计数值（0 ~ PWM_PERIOD）
 * @param tc C 相占空比计数值（0 ~ PWM_PERIOD）
 * @param td ADC 同步采样触发点计数值，通常设置为 (MAX - 1)，
 *           在 PWM 周期末触发 ADC 采样以避开开关噪声
 */
typedef void (*foc_hal_pwm_output_cb_t)(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td);

/**
 * @brief PWM 启动回调函数类型
 *
 * 使能 PWM 定时器输出通道，开始输出互补 PWM 波形。
 * 通常包括：使能定时器比较输出、使能死区插入、使能主输出等。
 */
typedef void (*foc_hal_pwm_start_cb_t)(void);

/**
 * @brief PWM 停止回调函数类型
 *
 * 禁用 PWM 定时器输出通道，将所有 PWM 输出置为安全电平。
 * 通常在电机停止或故障保护时调用。
 */
typedef void (*foc_hal_pwm_stop_cb_t)(void);

/**
 * @brief 编码器读取回调函数类型
 *
 * 读取编码器当前计数值。支持增量式编码器（定时器编码器模式）
 * 或绝对值编码器（SPI/并行接口）。
 *
 * @return 编码器原始计数值（0 ~ encoder_lines - 1）
 */
typedef uint16_t (*foc_hal_encoder_read_cb_t)(void);

/**
 * @brief 延时回调函数类型
 *
 * 提供阻塞式毫秒级延时，用于初始化阶段的时序控制。
 *
 * @param ms 延时毫秒数
 */
typedef void (*foc_hal_delay_cb_t)(uint32_t ms);

/* ==================== 配置结构体 ==================== */

/**
 * @brief FOC HAL 配置结构体
 *
 * 包含所有硬件相关的配置参数和回调函数指针。
 * 通过此结构体实现硬件无关的 FOC 核心逻辑。
 * 在移植到新平台时，填写此结构体即可完成硬件适配。
 */
typedef struct {
    /* ADC 相关 */
    foc_hal_adc_read_cb_t adc_read; /**< ADC 电流采样回调
                                          @note 必须注册，否则 foc_hal_init 失败 */
    foc_hal_adc_init_cb_t adc_init; /**< ADC 初始化回调
                                          @note 必须注册 */
    /* PWM 相关 */
    foc_hal_pwm_output_cb_t pwm_output; /**< PWM 输出回调
                                            @note 必须注册 */
    foc_hal_pwm_start_cb_t pwm_start; /**< PWM 启动回调
                                          @note 必须注册 */
    foc_hal_pwm_stop_cb_t pwm_stop; /**< PWM 停止回调
                                        @note 必须注册 */
    /* 编码器相关 */
    foc_hal_encoder_read_cb_t encoder_read; /**< 编码器读取回调
                                                 @note 必须注册 */
    /* 系统相关 */
    foc_hal_delay_cb_t delay_ms; /**< 延时回调（可选） */

    /* 配置参数 */
    uint16_t pwm_period_counts; /**< PWM 周期计数值（ARR 寄存器值）
                                   @note 用于 Q16.16 归一化占空比到硬件计数值的转换 */
    q16_16_t current_sample_factor_q; /**< 电流采样转换因子（Q16.16 格式）
                                         @note 用于 ADC 原始值到实际电流值的标度转换 */
} foc_hal_config_t;

/* ==================== HAL 管理器结构体 ==================== */

/**
 * @brief FOC HAL 管理器
 *
 * 封装硬件操作回调，提供统一的硬件访问接口。
 * 每个 FOC 实例拥有独立的 hal 对象，支持多电机控制。
 *
 * 使用方法：
 *   1. 填充 foc_hal_config_t 配置
 *   2. 调用 foc_hal_init() 初始化
 *   3. 调用 foc_hal_adc_init() 初始化 ADC 硬件
 *   4. 在 FOC 电流环中调用 foc_hal_adc_read()、foc_hal_pwm_update() 等
 */
typedef struct {
    foc_hal_config_t config; /**< 硬件配置（含所有回调指针和硬件参数） */
    bool initialized; /**< 初始化标志
                         @note 由 foc_hal_init() 设置，foc_hal_deinit() 清除 */
} foc_hal_t;

/* ==================== 公共 API ==================== */

/**
 * @brief 初始化 FOC HAL
 *
 * 检查必要回调是否注册，保存配置到 HAL 管理器。
 *
 * @param hal     HAL 管理器指针
 * @param config  硬件配置指针（含所有回调函数和参数）
 * @return true  初始化成功
 * @return false 参数为空或缺少必要回调
 */
bool foc_hal_init(foc_hal_t* hal, const foc_hal_config_t* config);

/**
 * @brief 反初始化 FOC HAL
 *
 * 清除初始化标志并清零配置，断开与硬件的逻辑连接。
 *
 * @param hal HAL 管理器指针
 */
void foc_hal_deinit(foc_hal_t* hal);

/**
 * @brief 初始化 ADC 硬件
 *
 * 通过注册的 adc_init 回调初始化 ADC 外设。
 * 通常在 foc_hal_init 后调用。
 *
 * @param hal HAL 管理器指针
 */
void foc_hal_adc_init(foc_hal_t* hal);

/**
 * @brief 读取 ADC 电流采样
 *
 * 通过注册的 adc_read 回调读取三相电流。
 *
 * @param hal HAL 管理器指针
 * @param[out] ia A 相电流输出（Q16.16 格式）
 * @param[out] ib B 相电流输出（Q16.16 格式）
 * @param[out] ic C 相电流输出（Q16.16 格式）
 */
void foc_hal_adc_read(foc_hal_t* hal, q16_16_t* ia, q16_16_t* ib, q16_16_t* ic);

/**
 * @brief 更新 PWM 输出占空比
 *
 * 将 Q16.16 归一化占空比转换为计数值后通过回调输出。
 *
 * @param hal HAL 管理器指针
 * @param ta   A 相占空比（Q16.16 归一化 [0, 1]）
 * @param tb   B 相占空比（Q16.16 归一化 [0, 1]）
 * @param tc   C 相占空比（Q16.16 归一化 [0, 1]）
 * @param td   ADC 触发点占空比（Q16.16 归一化 [0, 1]）
 */
void foc_hal_pwm_update(foc_hal_t* hal, q16_16_t ta, q16_16_t tb, q16_16_t tc, q16_16_t td);

/**
 * @brief 读取编码器角度
 *
 * @param hal HAL 管理器指针
 * @return 编码器原始角度值（0 ~ encoder_lines - 1）
 */
uint16_t foc_hal_encoder_read(foc_hal_t* hal);

/**
 * @brief 启动 PWM 输出
 *
 * @param hal HAL 管理器指针
 */
void foc_hal_pwm_start(foc_hal_t* hal);

/**
 * @brief 停止 PWM 输出
 *
 * @param hal HAL 管理器指针
 */
void foc_hal_pwm_stop(foc_hal_t* hal);

/**
 * @brief 检查 HAL 是否已初始化
 *
 * @param hal HAL 管理器指针
 * @return true 已初始化
 * @return false 未初始化或指针为空
 */
static inline bool foc_hal_is_initialized(const foc_hal_t* hal)
{
    return (hal != NULL) && hal->initialized;
}

#ifdef __cplusplus
}
#endif

#endif /* FOC_HAL_H */
