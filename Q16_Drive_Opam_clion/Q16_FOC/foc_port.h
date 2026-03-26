/**
 * @file    foc_port.h
 * @brief   FOC硬件抽象层 - 统一端口接口
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V3.0.0
 *
 * @description
 * 提供FOC控制与硬件之间的抽象接口，实现硬件无关的FOC核心逻辑。
 * 采用回调机制，支持不同硬件平台的移植。
 *
 * @design_principles
 * - 单一职责：仅负责硬件抽象，不包含业务逻辑
 * - 接口简洁：提供最小必要的API集合
 * - 类型安全：使用强类型回调函数
 * - 零依赖：不依赖具体硬件实现
 */

#ifndef FOC_PORT_H
#define FOC_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "foc_ctrl_q16.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /* ==================== 回调函数类型定义 ==================== */

    /**
     * @brief ADC电流采样回调（Q16.16格式）
     * @param[out] ia A相电流（Q16.16格式）
     * @param[out] ib B相电流（Q16.16格式）
     * @param[out] ic C相电流（Q16.16格式）
     */
    typedef void (*foc_adc_read_fn)(q16_16_t *ia, q16_16_t *ib, q16_16_t *ic);

    /**
     * @brief ADC初始化回调
     */
    typedef void (*foc_adc_init_fn)(void);

    /**
     * @brief PWM输出回调
     * @param ta A相占空比计数值
     * @param tb B相占空比计数值
     * @param tc C相占空比计数值
     * @param td 同步采样触发点计数值
     */
    typedef void (*foc_pwm_output_fn)(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td);

    /**
     * @brief PWM启动回调
     */
    typedef void (*foc_pwm_start_fn)(void);

    /**
     * @brief PWM停止回调
     */
    typedef void (*foc_pwm_stop_fn)(void);

    /**
     * @brief 编码器读取回调
     * @return 编码器原始角度值
     */
    typedef uint16_t (*foc_encoder_read_fn)(void);

    /**
     * @brief 延时回调
     * @param ms 延时毫秒数
     */
    typedef void (*foc_delay_fn)(uint32_t ms);

    /* ==================== 配置结构体 ==================== */

    /**
     * @brief FOC端口配置
     *
     * 包含所有硬件相关的配置参数和回调函数指针。
     * 通过此结构体实现硬件无关的FOC核心逻辑。
     */
    typedef struct
    {
        /* ADC相关 */
        foc_adc_read_fn adc_read; ///< ADC电流采样回调
        foc_adc_init_fn adc_init; ///< ADC初始化回调

        /* PWM相关 */
        foc_pwm_output_fn pwm_output; ///< PWM输出回调
        foc_pwm_start_fn pwm_start;   ///< PWM启动回调
        foc_pwm_stop_fn pwm_stop;     ///< PWM停止回调

        /* 编码器相关 */
        foc_encoder_read_fn encoder_read; ///< 编码器读取回调

        /* 系统相关 */
        foc_delay_fn delay_ms; ///< 延时回调

        /* 配置参数 */
        uint16_t pwm_period_counts;  ///< PWM周期计数值
        q16_16_t current_sample_factor_q; ///< 电流采样转换因子（Q16.16格式）
    } foc_port_config_t;

    /* ==================== 端口管理器结构体 ==================== */

    /**
     * @brief FOC端口管理器
     *
     * 封装硬件操作和FOC数据，提供统一的硬件访问接口。
     * 此结构体为内部使用，用户通过API函数操作。
     */
    typedef struct
    {
        foc_port_config_t config; ///< 硬件配置
        foc_sample_t *sample;     ///< 采样数据指针
        svpwm_t *svpwm;           ///< SVPWM数据指针
        foc_ctrl_t *ctrl;         ///< FOC控制数据指针
        bool initialized;         ///< 初始化标志
    } foc_port_t;

    /* ==================== 公共API ==================== */

    /**
     * @brief 初始化FOC端口
     *
     * @param port 端口管理器指针
     * @param config 硬件配置指针
     * @param sample 采样数据指针
     * @param svpwm SVPWM数据指针
     * @param ctrl FOC控制数据指针
     * @return true 初始化成功
     * @return false 初始化失败（参数无效或回调未设置）
     */
    bool foc_port_init(foc_port_t *port, const foc_port_config_t *config, foc_sample_t *sample, svpwm_t *svpwm,
                       foc_ctrl_t *ctrl);

    /**
     * @brief 初始化ADC硬件
     *
     * @param port 端口管理器指针
     */
    void foc_port_adc_init(foc_port_t *port);

    /**
     * @brief 读取ADC电流采样
     *
     * 调用硬件回调读取三相电流，并更新到采样数据结构。
     *
     * @param port 端口管理器指针
     */
    void foc_port_adc_read(foc_port_t *port);

    /**
     * @brief 更新PWM输出
     *
     * 将SVPWM计算的占空比转换为硬件计数值并输出。
     *
     * @param port 端口管理器指针
     */
    void foc_port_pwm_update(foc_port_t *port);

    /**
     * @brief 读取编码器角度
     *
     * 调用硬件回调读取编码器原始角度，并更新到控制数据结构。
     *
     * @param port 端口管理器指针
     * @return 编码器原始角度值
     */
    uint16_t foc_port_encoder_read(foc_port_t *port);

    /**
     * @brief 启动PWM输出
     *
     * @param port 端口管理器指针
     */
    void foc_port_pwm_start(foc_port_t *port);

    /**
     * @brief 停止PWM输出
     *
     * @param port 端口管理器指针
     */
    void foc_port_pwm_stop(foc_port_t *port);

    /**
     * @brief 检查端口是否已初始化
     *
     * @param port 端口管理器指针
     * @return true 已初始化
     * @return false 未初始化
     */
    static inline bool foc_port_is_initialized(const foc_port_t *port)
    {
        return (port != NULL) && port->initialized;
    }

    /**
     * @brief 获取全局端口实例
     * @return 端口管理器指针
     */
    foc_port_t *foc_port_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif /* FOC_PORT_H */