#ifndef FOC_CONTROLLER_Q16_H // 防止头文件重复包含
#define FOC_CONTROLLER_Q16_H

#include "encoder_alignment.h"
#include "foc_config_q16.h" // 包含FOC配置头文件
#include "foc_svpwm_q16.h"  // 包含FOC SVPWM头文件
#include "q16_16_math.h"    // 包含Q16.16定点数学库头文件

/**
 * @brief FOC控制结构体
 * 包含了FOC控制所需的所有状态变量和参数
 */
typedef struct
{
    uint8_t sw;                ///< FOC开关（0:关, 1:开）
    q16_16_t foc_ctrl_cycle_s; ///< foc 的控制周期（Q16.16格式 s）
    q16_16_t target_iq_q;      ///< 目标Q轴电流（Q16.16格式）
    q16_16_t target_id_q;      ///< 目标D轴电流（Q16.16格式）

    q16_16_t omega_q;            ///< 角速度（Q16.16格式）
    uint16_t raw_angle_q;        ///< 原始角度（编码器读数）
    q16_16_t electrical_angle_q; ///< 电气角度（Q16.16格式）

    q16_16_t pll_phase_q;    ///< PLL输出相位（Q16.16格式）
    q16_16_t pll_velocity_q; ///< PLL输出速度（Q16.16格式）
    float pll_velocity_rpm;  ///< PLL输出速度（RPM）
} foc_ctrl_t;

/**
 * @brief FOC采样数据结构体
 * 包含了ADC采样的电流值（Q16.16格式）
 */
typedef struct
{
    q16_16_t current_sample_q[3]; ///< 三相电流采样值（Q16.16格式）
} foc_sample_t;

extern svpwm_t g_svpwm;           ///< 外部SVPWM结构体实例
extern foc_sample_t g_adc_sample; ///< 外部ADC采样数据结构体实例
extern foc_ctrl_t foc_ctrl;       ///< 外部FOC控制结构体实例

/**
 * @brief FOC初始化函数
 */
extern void foc_init(void);

/**
 * @brief FOC状态机计算函数
 */
void foc_sm_calc(void);

/**
 * @brief FOC ADC中断计算函数
 */
void foc_adc_irq_calc(void);

/**
 * @brief 启动ADC DMA转换
 */
void adc_dma_start_convert(void);

#endif // FOC_CONTROLLER_Q16_H
