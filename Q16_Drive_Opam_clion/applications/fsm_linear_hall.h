//
// Created by fubingyan on 25-8-2.
//

#ifndef FSM_LINEAR_HALL_H
#define FSM_LINEAR_HALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "device_linear_hall.h"

/* Exported constants --------------------------------------------------------*/

/** @brief ADC 通道数量（引用设备层定义） */
#define ADC_CH_NUM DEVICE_LINEAR_HALL_ADC_CH_NUM

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 霍尔校准状态枚举
 */
typedef enum {
    FSM_LINEAR_HALL_STATE_NONE = 0, /**< 空闲 */
    FSM_LINEAR_HALL_STATE_FILTER,   /**< 滤波器稳定 */
    FSM_LINEAR_HALL_STATE_ALIGN,    /**< 电机对齐 */
    FSM_LINEAR_HALL_STATE_ROTATION, /**< 旋转采样 */
    FSM_LINEAR_HALL_STATE_PROCESS,  /**< 参数计算并保存 */
    FSM_LINEAR_HALL_STATE_DONE,     /**< 校准完成 */
    FSM_LINEAR_HALL_STATE_COUNT     /**< 状态总数（FSM 哨兵） */
} fsm_linear_hall_state_t;

/* Exported functions prototypes ---------------------------------------------*/

void fsm_linear_hall_init(void);

void fsm_linear_hall_task(void);

bool fsm_linear_hall_is_done(void);

fsm_linear_hall_state_t fsm_linear_hall_get_state(void);

void fsm_linear_hall_start(void);

float fsm_linear_hall_get_current(void);

float fsm_linear_hall_get_elec_angle(void);

#ifdef __cplusplus
}
#endif

#endif /* FSM_LINEAR_HALL_H */
