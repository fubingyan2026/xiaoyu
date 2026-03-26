/**
 * @file    foc_port.c
 * @brief   FOC硬件抽象层实现 - 统一端口接口
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V3.0.0
 *
 * @description
 * 实现FOC控制与硬件之间的抽象层，提供硬件无关的接口。
 * 合并了原 foc_port_q16 和 foc_port_q16_improved 的功能。
 */

#include "foc_port.h"
#include "foc_ctrl_q16.h"
#include "foc_svpwm_q16.h"
#include "q16_16_math.h"
#include <string.h>

/**
 * @brief 全局端口管理器实例
 * 提供单例访问方式，方便FOC核心代码调用。
 */
static foc_port_t g_foc_port = {0};

/* ==================== 公共API实现 ==================== */

/**
 * @brief 初始化FOC端口
 *
 * @param port 端口管理器指针
 * @param config 硬件配置指针
 * @param sample 采样数据指针
 * @param svpwm SVPWM数据指针
 * @param ctrl FOC控制数据指针
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool foc_port_init(foc_port_t *port, const foc_port_config_t *config, foc_sample_t *sample, svpwm_t *svpwm,
                   foc_ctrl_t *ctrl)
{
    /* 参数有效性检查 */
    if (port == NULL || config == NULL || sample == NULL || svpwm == NULL || ctrl == NULL)
    {
        return false;
    }

    /* 验证必要的回调函数 */
    if (config->adc_read == NULL || config->pwm_output == NULL || config->encoder_read == NULL ||
        config->delay_ms == NULL)
    {
        return false;
    }

    /* 复制配置 */
    memcpy(&port->config, config, sizeof(foc_port_config_t));

    /* 保存数据指针 */
    port->sample = sample;
    port->svpwm = svpwm;
    port->ctrl = ctrl;

    /* 调用ADC初始化（如果配置了回调） */
    if (config->adc_init != NULL)
    {
        config->adc_init();
    }

    port->initialized = true;
    return true;
}

/**
 * @brief 初始化ADC硬件
 *
 * @param port 端口管理器指针
 */
void foc_port_adc_init(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.adc_init == NULL)
    {
        return;
    }

    port->config.adc_init();
}

/**
 * @brief 读取ADC电流采样
 *
 * @param port 端口管理器指针
 */
void foc_port_adc_read(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.adc_read == NULL || port->sample == NULL)
    {
        return;
    }

    /* 调用硬件回调读取电流 */
    q16_16_t ia, ib, ic;
    port->config.adc_read(&ia, &ib, &ic);

    /* 更新采样数据 */
    port->sample->current_sample_q[0] = ia;
    port->sample->current_sample_q[1] = ib;
    port->sample->current_sample_q[2] = ic;
}

/**
 * @brief 更新PWM输出
 *
 * @param port 端口管理器指针
 */
void foc_port_pwm_update(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.pwm_output == NULL || port->svpwm == NULL)
    {
        return;
    }

    /* 将归一化占空比转换为计数值 */
    uint32_t ta = (uint32_t)(port->config.pwm_period_counts * Q16_16_TO_FLOAT(port->svpwm->ta));
    uint32_t tb = (uint32_t)(port->config.pwm_period_counts * Q16_16_TO_FLOAT(port->svpwm->tb));
    uint32_t tc = (uint32_t)(port->config.pwm_period_counts * Q16_16_TO_FLOAT(port->svpwm->tc));
    uint32_t td = (uint32_t)(port->config.pwm_period_counts * Q16_16_TO_FLOAT(port->svpwm->td));

    /* 调用硬件回调输出PWM */
    port->config.pwm_output(ta, tb, tc, td);
}

/**
 * @brief 读取编码器角度
 *
 * @param port 端口管理器指针
 * @return 编码器原始角度值
 */
uint16_t foc_port_encoder_read(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.encoder_read == NULL)
    {
        return 0;
    }

    uint16_t raw_angle = port->config.encoder_read();

    /* 更新控制数据 */
    if (port->ctrl != NULL)
    {
        port->ctrl->raw_angle_q = raw_angle;
    }

    return raw_angle;
}

/**
 * @brief 启动PWM输出
 *
 * @param port 端口管理器指针
 */
void foc_port_pwm_start(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.pwm_start == NULL)
    {
        return;
    }

    port->config.pwm_start();
}

/**
 * @brief 停止PWM输出
 *
 * @param port 端口管理器指针
 */
void foc_port_pwm_stop(foc_port_t *port)
{
    if (port == NULL || !port->initialized || port->config.pwm_stop == NULL)
    {
        return;
    }

    port->config.pwm_stop();
}

/**
 * @brief 获取全局端口实例
 *
 * @return 端口管理器指针
 */
foc_port_t *foc_port_get_instance(void)
{
    return &g_foc_port;
}