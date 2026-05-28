/**
 * @file    foc_fsm.h
 * @brief   FOC State Machine - 使用通用FSM框架
 * @author  FOC Development Team
 * @date    2026-02-04
 */

#ifndef FOC_FSM_H
#define FOC_FSM_H

#include "foc_math.h"
#include "fsm.h"
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
typedef enum __attribute__((packed)) {
  FOC_FSM_STATE_IDLE = 0,
  FOC_FSM_STATE_ALIGN,
  FOC_FSM_STATE_ALIGNMENT,
  FOC_FSM_STATE_RUN,
  FOC_FSM_STATE_HALL,
  FOC_FSM_STATE_STOP,
  FOC_FSM_STATE_COUNT
} foc_fsm_state_e;

typedef enum __attribute__((packed)) {
  FOC_FSM_RET_OK = 0,
  FOC_FSM_RET_ERROR,
  FOC_FSM_RET_INVALID_STATE,
} foc_fsm_ret_e;

typedef enum __attribute__((packed)) {
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

typedef struct __attribute__((packed)) {
  int16_t capture_idx;          /**< 校准采样索引 */
  foc_cali_step_e step;         /**< 当前校准步骤 */
  uint16_t timeout_cnt;         /**< 超时计数器 */
  uint16_t transition_cnt;      /**< 过渡计数器 */
  q16_16_t last_angle_q;        /**< 上一次电气角度 */
} foc_fsm_cali_context_t;

struct foc_fsm_context_s {
  fsm_t fsm;                    /**< 通用FSM实例 */
  foc_fsm_cali_context_t cali_ctx; /**< 校准上下文 */
  void* parent;                 /**< 父级foc_context_t指针 */
  void* flash_data;             /**< Flash校准数据指针 */
};

/*============================================================================
 * 公共API声明
 *============================================================================*/

/**
 * @brief 初始化FOC状态机
 * @param ctx FSM上下文指针
 * @param parent 父级FOC上下文指针
 * @return 操作结果
 */
foc_fsm_ret_e foc_fsm_init(foc_fsm_context_t* ctx, void* parent);

/**
 * @brief 执行一步状态机
 * @param ctx FSM上下文指针
 * @return 操作结果
 */
foc_fsm_ret_e foc_fsm_step(foc_fsm_context_t* ctx);

/**
 * @brief 请求状态切换
 * @param ctx FSM上下文指针
 * @param state 目标状态
 * @return 操作结果
 */
foc_fsm_ret_e foc_fsm_request_state(foc_fsm_context_t* ctx, foc_fsm_state_e state);

/**
 * @brief 状态枚举转字符串
 * @param state 状态枚举值
 * @return 状态名称字符串
 */
const char* foc_fsm_state_to_string(foc_fsm_state_e state);

/**
 * @brief 设置Flash校准数据指针
 * @param ctx FSM上下文指针
 * @param flash_data Flash数据指针
 */
void foc_fsm_set_flash_data(foc_fsm_context_t* ctx, void* flash_data);

#endif /* FOC_FSM_H */
