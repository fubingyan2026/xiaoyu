/**
 * @file    foc_port.c
 * @brief   FOC硬件抽象层实现
 * @author  FOC Development Team
 * @date    2026-02-06
 * @version V4.0.0
 */

#include "foc_port.h"

#include <string.h>

/* Exported functions --------------------------------------------------------*/

bool foc_port_init(foc_port_t* port, const foc_port_config_t* config)
{
    if (!port || !config) {
        return false;
    }

    // 检查必要的回调函数
    if (!config->adc_read || !config->adc_init || !config->pwm_output || !config->pwm_start || !config->pwm_stop || !config->encoder_read) {
        return false;
    }

    // 如果已初始化，先反初始化
    if (port->initialized) {
        foc_port_deinit(port);
    }

    // 保存配置
    port->config = *config;
    port->initialized = true;

    return true;
}

void foc_port_deinit(foc_port_t* port)
{
    if (!port) {
        return;
    }

    port->initialized = false;
    memset(&port->config, 0, sizeof(foc_port_config_t));
}

void foc_port_adc_init(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.adc_init) {
        return;
    }

    port->config.adc_init();
}

void foc_port_adc_read(foc_port_t* port, q16_16_t* ia, q16_16_t* ib, q16_16_t* ic)
{
    if (!port || !port->initialized || !port->config.adc_read) {
        return;
    }

    port->config.adc_read(ia, ib, ic);
}

void foc_port_pwm_update(foc_port_t* port, q16_16_t ta, q16_16_t tb, q16_16_t tc, q16_16_t td)
{
    if (!port || !port->initialized || !port->config.pwm_output) {
        return;
    }

    // 将归一化Q16.16占空比转换为硬件计数值
    uint32_t ta_counts = (uint32_t)q16_16_mul(ta, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t tb_counts = (uint32_t)q16_16_mul(tb, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t tc_counts = (uint32_t)q16_16_mul(tc, INT_TO_Q16_16(port->config.pwm_period_counts));
    uint32_t td_counts = (uint32_t)q16_16_mul(td, INT_TO_Q16_16(port->config.pwm_period_counts));

    port->config.pwm_output(ta_counts, tb_counts, tc_counts, td_counts);
}

uint16_t foc_port_encoder_read(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.encoder_read) {
        return 0;
    }

    return port->config.encoder_read();
}

void foc_port_pwm_start(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.pwm_start) {
        return;
    }

    port->config.pwm_start();
}

void foc_port_pwm_stop(foc_port_t* port)
{
    if (!port || !port->initialized || !port->config.pwm_stop) {
        return;
    }

    port->config.pwm_stop();
}
