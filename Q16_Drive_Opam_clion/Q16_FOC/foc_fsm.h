/**
 * @file    foc_fsm.h
 * @brief   FOC State Machine - 使用通用FSM框架
 * @author  FOC Development Team
 * @date    2026-02-04
 */

#ifndef FOC_FSM_H
#define FOC_FSM_H

#include "foc_config_q16.h"
#include "foc_ctrl_q16.h"
#include "fsm.h"
#include "q16_16_math.h"
#include <stdbool.h>
#include <stdint.h>

/*============================================================================
 * 公共宏定义
 *============================================================================*/
#define FOC_FSM_PERIOD_SEC (0.001f)
#define FOC_FSM_PERIOD_Q FLOAT_TO_Q16_16(FOC_FSM_PERIOD_SEC)
#define FOC_FSM_ELEC_ANGLE_STABLE_TIME (250U)
#define FOC_FSM_ALIGN_TIMEOUT_MS (1500U)
#define FOC_FSM_ALIGN_TIMEOUT_CNT (FOC_FSM_ALIGN_TIMEOUT_MS / 1U)
#define FOC_FSM_CALI_STEPS_EXTRA (4U)
#define FOC_FSM_TRANSITION_STEPS (250U)
#define FOC_FSM_STOP_TIME (10U)

/*============================================================================
 * 公共枚举定义
 *============================================================================*/
typedef enum __attribute__((packed))
{
    FOC_FSM_STATE_IDLE = 0,
    FOC_FSM_STATE_ALIGN,
    FOC_FSM_STATE_ALIGNMENT,
    FOC_FSM_STATE_RUN,
    FOC_FSM_STATE_HALL,
    FOC_FSM_STATE_STOP,
    FOC_FSM_STATE_COUNT
} foc_fsm_state_e;

typedef enum __attribute__((packed))
{
    FOC_FSM_RET_OK = 0,
    FOC_FSM_RET_ERROR,
    FOC_FSM_RET_INVALID_STATE,
} foc_fsm_ret_e;

typedef enum __attribute__((packed))
{
    FOC_CALI_STEP_FORWARD = 0,
    FOC_CALI_STEP_TRANSITION,
    FOC_CALI_STEP_REVERSE,
    FOC_CALI_STEP_COMPLETE,
    FOC_CALI_STEP_COUNT
} foc_cali_step_e;

/*============================================================================
 * 公共类型定义
 *============================================================================*/
typedef struct foc_fsm_context_s foc_fsm_context_t;

typedef struct __attribute__((packed))
{
    int16_t capture_idx;
    foc_cali_step_e step;
    uint16_t timeout_cnt;
    uint16_t transition_cnt;
    q16_16_t last_angle_q;
} foc_fsm_cali_context_t;

struct foc_fsm_context_s
{
    fsm_t fsm;
    foc_fsm_cali_context_t cali_ctx;
    foc_ctrl_t *ctrl;
    motor_flash_config_t *flash_data;
};

/*============================================================================
 * 公共API声明
 *============================================================================*/
foc_fsm_ret_e foc_fsm_init(foc_fsm_context_t *ctx, foc_ctrl_t *ctrl);
foc_fsm_ret_e foc_fsm_step(foc_fsm_context_t *ctx);
foc_fsm_ret_e foc_fsm_request_state(foc_fsm_context_t *ctx, foc_fsm_state_e state);
const char *foc_fsm_state_to_string(foc_fsm_state_e state);
foc_fsm_context_t *foc_fsm_get_instance(void);
void foc_fsm_calc(void);
foc_fsm_state_e foc_fsm_current_state(void);
void foc_fsm_set_flash_data(foc_fsm_context_t *ctx, motor_flash_config_t *flash_data);

#endif /* FOC_FSM_H */
