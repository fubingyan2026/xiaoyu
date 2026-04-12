//
// Created by fubingyan on 25-9-5.
//

#ifndef KEY_FUNCTION_H
#define KEY_FUNCTION_H

#include "fsm/fsm.h"
#include "key_base/key_base.h"
#include "stdbool.h"
#include "stdint.h"

/* ==================== 状态机状态定义 ==================== */

/**
 * @brief 按键功能状态枚举
 */
typedef enum {
  KEY_FSM_STATE_NONE = 0,     /**< 空闲状态 */
  KEY_FSM_STATE_SHOW_COMMAND, /**< 命令显示状态 */
  KEY_FSM_STATE_SET_VALUE,    /**< 数值设置状态 */
  KEY_FSM_STATE_MAX           /**< 状态数量 */
} key_fsm_state_e;

/* ==================== 数据结构 ==================== */

/**
 * @brief 按键 FSM 上下文结构
 */
typedef struct {
  fsm_context_t fsm;             /**< FSM 上下文 */
  uint32_t time_now;             /**< 当前系统时间 */
  uint32_t time_last;            /**< 上次按键触发的时间 */
  uint32_t time_sys_last;        /**< 上次任务运行的时间 */
  uint32_t time_diff;            /**< 时间差值 */
  uint16_t cmd_entry_timeout_ms; /**< 命令输入超时时间 */
  uint8_t cmd_id;                /**< 当前命令 */
  uint8_t cmd_value;             /**< 当前数值 */
  uint8_t cmd_value_last;        /**< 上次数值 */
  uint8_t last_cmd_flag;         /**< 上次命令标志 */
  bool set_cmd_flag;             /**< 设置命令标志 */
} key_fsm_ctx_t;

void key_func_init(void);
void key_func_task(void);
key_fsm_state_e key_func_get_state(void);
uint32_t key_menu_get_can_id(void);

#endif  // KEY_MENU_H
