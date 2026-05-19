//
// Created by fubingyan on 25-8-2.
//

#ifndef HALL_ADJUSTMENT_H
#define HALL_ADJUSTMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "pll.h"

/* Exported constants --------------------------------------------------------*/

/** @brief ADC 通道数量 */
#define ADC_CH_NUM 2

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 霍尔校准状态枚举
 */
typedef enum {
    HALL_ADJUST_STATE_NONE = 0, /**< 空闲 */
    HALL_ADJUST_STATE_FILTER,   /**< 滤波器稳定 */
    HALL_ADJUST_STATE_ALIGN,    /**< 电机对齐 */
    HALL_ADJUST_STATE_ROTATION, /**< 旋转采样 */
    HALL_ADJUST_STATE_PROCESS,  /**< 参数计算并保存 */
    HALL_ADJUST_STATE_DONE,     /**< 校准完成 */
    HALL_ADJUST_STATE_COUNT     /**< 状态总数（FSM 哨兵） */
} hall_adjust_state_t;

/**
 * @brief 霍尔校准保存参数结构体
 */
typedef struct {
    int16_t adcAmplitudeBias[ADC_CH_NUM]; /**< ADC 偏置值 */
    int16_t adcAmplitudeMax[ADC_CH_NUM];  /**< ADC 幅值 */
    uint16_t hall_adjust_flag;            /**< 校准完成标志 */
} hall_save_param_t;

/* Exported variables --------------------------------------------------------*/

extern hall_save_param_t hall_save_param;
extern pll_context_t pll_ctx;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化霍尔校准模块
 */
void hall_adjust_init(void);

/**
 * @brief 执行霍尔校准任务（每个控制周期调用一次）
 */
void hall_adjust_task(void);

/**
 * @brief 获取校准后的霍尔角度
 * @return 角度值 (0-16383)
 */
uint16_t hall_adjust_get_angle(void);

/**
 * @brief 启动 ADC DMA 采样
 */
void hall_adjust_adc_dma_start(void);

/**
 * @brief 检查霍尔校准是否完成
 * @return true 表示已校准完成
 */
bool hall_adjust_is_calibrated(void);

/**
 * @brief 获取当前校准状态
 * @return 当前状态
 */
hall_adjust_state_t hall_adjust_get_state(void);

/**
 * @brief 启动校准流程
 */
void hall_adjust_start_calibration(void);

/**
 * @brief 获取校准过程中的目标电流
 * @return 目标电流值
 */
float hall_adjust_get_target_current(void);

/**
 * @brief 获取校准过程中的目标电角度
 * @return 目标电角度
 */
float hall_adjust_get_target_elec_angle(void);

#ifdef __cplusplus
}
#endif

#endif /* HALL_ADJUSTMENT_H */
