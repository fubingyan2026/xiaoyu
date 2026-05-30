/**
 * @file    foc_port.c
 * @brief   FOC硬件抽象层实现
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V4.0.0
 */

#include "foc_port.h"

#include <string.h>

/* ==================== 导出函数实现 ==================== */

/**
 * @brief 初始化 FOC 端口管理器
 *
 * 保存硬件配置和回调函数指针，检查必要回调是否已注册。
 * 如果端口已初始化，先执行反初始化再重新配置。
 *
 * @param port   端口管理器指针
 * @param config 硬件配置结构体指针（含 ADC、PWM、编码器等回调）
 * @return true  初始化成功
 * @return false 参数为空或缺少必要回调（adc_read/adc_init/pwm_output/pwm_start/pwm_stop/encoder_read）
 */
bool foc_port_init(foc_port_t* port, const foc_port_config_t* config)
{
    if (!port || !config) {
        return false;
    }

    /* 检查必要的回调函数是否均已注册 */
    if (!config->adc_read || !config->adc_init || !config->pwm_output || !config->pwm_start || !config->pwm_stop || !config->encoder_read) {
        return false;
    }

    /* 如果已初始化，先反初始化以重置状态 */
    if (port->initialized) {
        foc_port_deinit(port);
    }

    /* 保存配置 */
    port->config = *config;
    port->initialized = true;

    return true;
}

/**
 * @brief 反初始化 FOC 端口管理器
 *
 * 清除初始化标志并清零配置结构体，断开与硬件的连接。
 *
 * @param port 端口管理器指针
 */
void foc_port_deinit(foc_port_t* port)
{
    if (!port) {
        return;
    }

    port->initialized = false;
    memset(&port->config, 0, sizeof(foc_port_config_t));
}

/**
 * @brief 初始化 ADC 硬件
 *
 * 通过回调函数初始化 ADC 外设。
 * 调用者在 foc_port_init 后调用此函数。
 *
 * @param port 端口管理器指针
 */
void foc_port_adc_init(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.adc_init) {
        return;
    }

    port->config.adc_init();
}

/**
 * @brief 读取 ADC 电流采样值
 *
 * 通过注册的回调读取三相电流的 ADC 采样值。
 * 采样值经过 CURRENT_SAMPLE_FACTOR 转换后以 Q16.16 格式返回。
 *
 * @param port 端口管理器指针
 * @param[out] ia A 相电流值指针（Q16.16 格式）
 * @param[out] ib B 相电流值指针（Q16.16 格式）
 * @param[out] ic C 相电流值指针（Q16.16 格式）
 */
void foc_port_adc_read(foc_port_t* port, q16_16_t* ia, q16_16_t* ib, q16_16_t* ic)
{
    if (!port || !port->initialized || !port->config.adc_read) {
        return;
    }

    port->config.adc_read(ia, ib, ic);
}

/**
 * @brief 更新 PWM 输出占空比
 *
 * 将 Q16.16 归一化占空比 [0, 1] 转换为硬件定时器计数值，
 * 然后通过回调函数写入硬件寄存器。
 * td 用于 ADC 同步采样触发点，通常设置为最大占空比对应的时间点。
 *
 * @param port 端口管理器指针
 * @param ta   A 相占空比（Q16.16 归一化值，范围 [0, 1]）
 * @param tb   B 相占空比（Q16.16 归一化值，范围 [0, 1]）
 * @param tc   C 相占空比（Q16.16 归一化值，范围 [0, 1]）
 * @param td   ADC 同步采样触发点（Q16.16 归一化值，范围 [0, 1]）
 */
void foc_port_pwm_update(foc_port_t* port, q16_16_t ta, q16_16_t tb, q16_16_t tc, q16_16_t td)
{
    if (!port || !port->initialized || !port->config.pwm_output) {
        return;
    }

    /* 将归一化 Q16.16 占空比转换为硬件定时器计数值 */
    uint32_t ta_counts = (uint32_t)q16_16_mul(ta, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t tb_counts = (uint32_t)q16_16_mul(tb, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t tc_counts = (uint32_t)q16_16_mul(tc, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t td_counts = (uint32_t)q16_16_mul(td, INT_TO_Q16_16(port->config.pwm_period_counts));

    port->config.pwm_output(ta_counts, tb_counts, tc_counts, td_counts);
}

/**
 * @brief 读取编码器原始角度值
 *
 * 通过注册的回调函数读取编码器当前位置。
 *
 * @param port 端口管理器指针
 * @return 编码器原始角度值（0 ~ encoder_lines - 1）
 *         如果端口未初始化或回调未注册，返回 0
 */
uint16_t foc_port_encoder_read(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.encoder_read) {
        return 0;
    }

    return port->config.encoder_read();
}

/**
 * @brief 启动 PWM 输出
 *
 * 通过回调函数使能 PWM 定时器输出。
 * 通常在 FOC 状态机从 IDLE 切换到 RUN 时调用。
 *
 * @param port 端口管理器指针
 */
void foc_port_pwm_start(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.pwm_start) {
        return;
    }

    port->config.pwm_start();
}

/**
 * @brief 停止 PWM 输出
 *
 * 通过回调函数禁用 PWM 定时器输出，将所有通道置为无效电平。
 * 通常在 FOC 状态机切换到 STOP 或故障保护时调用。
 *
 * @param port 端口管理器指针
 */
void foc_port_pwm_stop(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.pwm_stop) {
        return;
    }

    port->config.pwm_stop();
}
