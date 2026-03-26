/**
 * @file    foc_sm.h
 * @brief   FOC State Machine - 使用通用FSM框架重构
 * @author  FOC Development Team
 * @date    2026-02-04
 */

#ifndef FOC_SM_H
#define FOC_SM_H

#include "foc_config_q16.h"
#include "foc_ctrl_q16.h"
#include "fsm/fsm.h"
#include "q16_16_math.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*============================================================================
 * 公共宏定义
 *============================================================================*/
#define FOC_SM_PERIOD_SEC (0.001f)
#define FOC_SM_PERIOD_Q FLOAT_TO_Q16_16(FOC_SM_PERIOD_SEC)
#define FOC_SM_ELEC_ANGLE_STABLE_TIME (250U)
#define FOC_SM_ALIGN_TIMEOUT_MS (1500U)
#define FOC_SM_ALIGN_TIMEOUT_CNT (FOC_SM_ALIGN_TIMEOUT_MS / 1U)
#define FOC_SM_CALI_STEPS_EXTRA (4U)
#define FOC_SM_TRANSITION_STEPS (250U)  // 过渡状态步数（约50ms减速时间）
#define FOC_SM_STOP_TIME (10U)         // 停止等待时间（约10ms确保完全停止）

/*============================================================================
 * 公共枚举定义
 *============================================================================*/
typedef enum __attribute__((packed))
{
    FOC_SM_STATE_IDLE = 0,
    FOC_SM_STATE_SELF_CHECK,
    FOC_SM_STATE_ALIGN,
    FOC_SM_STATE_ALIGNMENT,
    FOC_SM_STATE_RUN,
    FOC_SM_STATE_HALL,
    FOC_SM_STATE_STOP,
    FOC_SM_STATE_COUNT
} foc_sm_state_e;

/**
 * @brief FOC状态机返回结果枚举
 */
typedef enum __attribute__((packed))
{
    FOC_SM_RET_OK = 0,
    FOC_SM_RET_ERROR,
    FOC_SM_RET_INVALID_STATE,
    FOC_SM_RET_TIMEOUT,
    FOC_SM_RET_CALI_FAILED
} foc_sm_ret_e;

typedef enum __attribute__((packed))
{
    FOC_CALI_STEP_FORWARD = 0,
    FOC_CALI_STEP_TRANSITION,  // 正反转过渡状态
    FOC_CALI_STEP_REVERSE,
    FOC_CALI_STEP_COMPLETE,
    FOC_CALI_STEP_COUNT
} foc_cali_step_e;

/*============================================================================
 * 公共类型定义
 *============================================================================*/
typedef struct foc_sm_context_s foc_sm_context_t;

/**
 * @brief FOC状态机专用回调类型（1个参数）
 */
typedef void (*foc_sm_on_entry_t)(foc_sm_context_t *ctx);
typedef void (*foc_sm_on_exit_t)(foc_sm_context_t *ctx);

/**
 * @brief FOC状态机校准上下文结构体
 */
typedef struct __attribute__((packed))
{
    int16_t capture_idx;
    foc_cali_step_e step;
    uint16_t timeout_cnt;
    uint16_t transition_cnt;  // 过渡状态计数器
    q16_16_t last_angle_q;
} foc_sm_cali_context_t;

/**
 * @brief FOC状态机上下文结构体（兼容通用FSM框架）
 */
struct foc_sm_context_s
{
    fsm_context_t fsm;              ///< 通用FSM上下文（必须作为第一个成员）
    foc_sm_cali_context_t cali_ctx; ///< 校准上下文
    foc_ctrl_t *ctrl;               ///< FOC控制数据指针
    motor_flash_config_t *flash_data; ///< 电机Flash数据指针
    foc_sm_on_entry_t on_entry;     ///< FOC专用进入回调（1个参数）
    foc_sm_on_exit_t on_exit;       ///< FOC专用退出回调（1个参数）
};

/*============================================================================
 * 公共API声明（完全兼容原有接口）
 *============================================================================*/
foc_sm_ret_e foc_sm_init(foc_sm_context_t *ctx, foc_ctrl_t *ctrl);
foc_sm_ret_e foc_sm_step(foc_sm_context_t *ctx);
foc_sm_ret_e foc_sm_request_state(foc_sm_context_t *ctx, foc_sm_state_e state);
const char *foc_sm_state_to_string(foc_sm_state_e state);
foc_sm_context_t *foc_sm_get_instance(void);
void foc_sm_calc(void);
foc_sm_state_e foc_sm_current_state(void);
/*============================================================================
 * 标准接口函数
 *============================================================================*/

/**
 * @brief 设置Flash数据指针
 * @param ctx 状态机上下文指针
 * @param flash_data Flash数据指针
 */
void foc_sm_set_flash_data(foc_sm_context_t *ctx, motor_flash_config_t *flash_data);

/**
 * @brief 获取Flash数据指针
 * @param ctx 状态机上下文指针
 * @return Flash数据指针
 */
motor_flash_config_t *foc_sm_get_flash_data(foc_sm_context_t *ctx);

/**
 * @brief 设置进入回调
 * @param ctx 状态机上下文指针
 * @param on_entry 进入回调函数
 */
void foc_sm_set_on_entry(foc_sm_context_t *ctx, foc_sm_on_entry_t on_entry);

/**
 * @brief 设置退出回调
 * @param ctx 状态机上下文指针
 * @param on_exit 退出回调函数
 */
void foc_sm_set_on_exit(foc_sm_context_t *ctx, foc_sm_on_exit_t on_exit);

#endif /* FOC_SM_H */