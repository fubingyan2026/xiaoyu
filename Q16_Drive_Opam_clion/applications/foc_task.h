/**
 * @file    foc_task.h
 * @brief   FOC平台任务模块 - 硬件初始化与平台集成
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V1.0.0
 */

#ifndef FOC_TASK_H
#define FOC_TASK_H

#include "foc.h"

/* ==================== 全局上下文 ==================== */

extern foc_context_t g_foc_ctx;

/* ==================== 公共API ==================== */

/**
 * @brief FOC平台初始化
 *
 * 执行完整的FOC平台初始化流程：传感器初始化、编码器校准加载、
 * 硬件回调配置、FOC模块初始化。
 */
void foc_task_init(void);

/**
 * @brief 启动ADC DMA转换（供ISR调用）
 */
void foc_task_adc_dma_start(void);

#endif /* FOC_TASK_H */
