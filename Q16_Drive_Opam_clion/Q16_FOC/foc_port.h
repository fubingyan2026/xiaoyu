/**
 * @file    foc_port.h
 * @brief   FOC硬件抽象层 - 统一端口接口
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V4.0.0
 *
 * @description
 * 提供FOC控制与硬件之间的抽象接口，实现硬件无关的FOC核心逻辑。
 * 采用回调机制，支持不同硬件平台的移植。
 */

#ifndef FOC_PORT_H
#define FOC_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foc_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 回调函数类型定义 ==================== */

/**
 * @brief ADC电流采样回调（Q16.16格式）
 * @param[out] ia A相电流（Q16.16格式）
 * @param[out] ib B相电流（Q16.16格式）
 * @param[out] ic C相电流（Q16.16格式）
 */
typedef void (*foc_port_adc_read_cb_t)(q16_16_t* ia, q16_16_t* ib, q16_16_t* ic);

/**
 * @brief ADC初始化回调
 */
typedef void (*foc_port_adc_init_cb_t)(void);

/**
 * @brief PWM输出回调
 * @param ta A相占空比计数值
 * @param tb B相占空比计数值
 * @param tc C相占空比计数值
 * @param td 同步采样触发点计数值
 */
typedef void (*foc_port_pwm_output_cb_t)(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td);

/**
 * @brief PWM启动回调
 */
typedef void (*foc_port_pwm_start_cb_t)(void);

/**
 * @brief PWM停止回调
 */
typedef void (*foc_port_pwm_stop_cb_t)(void);

/**
 * @brief 编码器读取回调
 * @return 编码器原始角度值
 */
typedef uint16_t (*foc_port_encoder_read_cb_t)(void);

/**
 * @brief 延时回调
 * @param ms 延时毫秒数
 */
typedef void (*foc_port_delay_cb_t)(uint32_t ms);

/* ==================== 配置结构体 ==================== */

/**
 * @brief FOC端口配置
 *
 * 包含所有硬件相关的配置参数和回调函数指针。
 * 通过此结构体实现硬件无关的FOC核心逻辑。
 */
typedef struct {
    /* ADC相关 */
    foc_port_adc_read_cb_t adc_read; /**< ADC电流采样回调 */
    foc_port_adc_init_cb_t adc_init; /**< ADC初始化回调 */

    /* PWM相关 */
    foc_port_pwm_output_cb_t pwm_output; /**< PWM输出回调 */
    foc_port_pwm_start_cb_t pwm_start; /**< PWM启动回调 */
    foc_port_pwm_stop_cb_t pwm_stop; /**< PWM停止回调 */

    /* 编码器相关 */
    foc_port_encoder_read_cb_t encoder_read; /**< 编码器读取回调 */

    /* 系统相关 */
    foc_port_delay_cb_t delay_ms; /**< 延时回调 */

    /* 配置参数 */
    uint16_t pwm_period_counts; /**< PWM周期计数值 */
    q16_16_t current_sample_factor_q; /**< 电流采样转换因子（Q16.16格式） */
} foc_port_config_t;

/* ==================== 端口管理器结构体 ==================== */

/**
 * @brief FOC端口管理器
 *
 * 封装硬件操作回调，提供统一的硬件访问接口。
 */
typedef struct {
    foc_port_config_t config; /**< 硬件配置 */
    bool initialized; /**< 初始化标志 */
} foc_port_t;

/* ==================== 公共API ==================== */

/**
 * @brief 初始化FOC端口
 * @param port 端口管理器指针
 * @param config 硬件配置指针
 * @return true 初始化成功
 */
bool foc_port_init(foc_port_t* port, const foc_port_config_t* config);

/**
 * @brief 反初始化FOC端口
 * @param port 端口管理器指针
 */
void foc_port_deinit(foc_port_t* port);

/**
 * @brief 初始化ADC硬件
 * @param port 端口管理器指针
 */
void foc_port_adc_init(foc_port_t* port);

/**
 * @brief 读取ADC电流采样
 * @param port 端口管理器指针
 * @param[out] ia A相电流（Q16.16格式）
 * @param[out] ib B相电流（Q16.16格式）
 * @param[out] ic C相电流（Q16.16格式）
 */
void foc_port_adc_read(foc_port_t* port, q16_16_t* ia, q16_16_t* ib, q16_16_t* ic);

/**
 * @brief 更新PWM输出
 * @param port 端口管理器指针
 * @param ta A相占空比（Q16.16归一化值[0,1]）
 * @param tb B相占空比（Q16.16归一化值[0,1]）
 * @param tc C相占空比（Q16.16归一化值[0,1]）
 * @param td ADC触发点占空比（Q16.16归一化值[0,1]）
 */
void foc_port_pwm_update(foc_port_t* port, q16_16_t ta, q16_16_t tb, q16_16_t tc, q16_16_t td);

/**
 * @brief 读取编码器角度
 * @param port 端口管理器指针
 * @return 编码器原始角度值
 */
uint16_t foc_port_encoder_read(foc_port_t* port);

/**
 * @brief 启动PWM输出
 * @param port 端口管理器指针
 */
void foc_port_pwm_start(foc_port_t* port);

/**
 * @brief 停止PWM输出
 * @param port 端口管理器指针
 */
void foc_port_pwm_stop(foc_port_t* port);

/**
 * @brief 检查端口是否已初始化
 * @param port 端口管理器指针
 * @return true 已初始化
 */
static inline bool foc_port_is_initialized(const foc_port_t* port)
{
    return (port != NULL) && port->initialized;
}

#ifdef __cplusplus
}
#endif

#endif /* FOC_PORT_H */
