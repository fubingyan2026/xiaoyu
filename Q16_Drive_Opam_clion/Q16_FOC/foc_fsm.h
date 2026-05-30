/**
 * @file    foc_fsm.h
 * @brief   FOC 状态机 - 基于通用 FSM 框架
 *
 * 实现 FOC 控制系统的完整状态机，包括：
 *   - IDLE：空闲等待，检测开关信号
 *   - ALIGN：转子预定位（对齐到已知电气角度）
 *   - ALIGNMENT：编码器自动校准（正反向扫描，建立角度映射表）
 *   - RUN：正常运行 FOC 电流闭环
 *   - HALL：线性霍尔传感器辅助启动
 *   - STOP：停机状态
 *
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

/**
 * @def FOC_FSM_PERIOD_SEC
 * @brief 状态机运行周期（秒）
 * @note 状态机以 1ms 为周期在主循环中被调用，
 *       与 PWM 中断中的电流环（约 16.8kHz）解耦
 */
#define FOC_FSM_PERIOD_SEC (0.001f)

/**
 * @def FOC_FSM_PERIOD_Q
 * @brief 状态机运行周期（Q16.16 固定点格式）
 */
#define FOC_FSM_PERIOD_Q FLOAT_TO_Q16_16(FOC_FSM_PERIOD_SEC)

/**
 * @def FOC_FSM_ELEC_ANGLE_STABLE_TIME
 * @brief 电气角度稳定等待时间（状态机步数）
 * @note 在校准过程中，每步施加新的电气角度后，
 *       需要等待 FOC_FSM_ELEC_ANGLE_STABLE_TIME 个周期（约 250ms）
 *       让机械系统稳定后再记录编码器读数
 */
#define FOC_FSM_ELEC_ANGLE_STABLE_TIME (250U)

/**
 * @def FOC_FSM_ALIGN_TIMEOUT_MS
 * @brief 转子预定位超时时间（毫秒）
 */
#define FOC_FSM_ALIGN_TIMEOUT_MS (1500U)

/**
 * @def FOC_FSM_ALIGN_TIMEOUT_CNT
 * @brief 转子预定位超时计数器（以 1ms 为步长）
 */
#define FOC_FSM_ALIGN_TIMEOUT_CNT (FOC_FSM_ALIGN_TIMEOUT_MS / 1U)

/**
 * @def FOC_FSM_CALI_STEPS_EXTRA
 * @brief 编码器校准额外步数
 * @note 在正向和反向扫描时，实际采集点数比理论总步数多 4 步，
 *       以确保覆盖完整的电气周期边界
 */
#define FOC_FSM_CALI_STEPS_EXTRA (4U)

/**
 * @def FOC_FSM_TRANSITION_STEPS
 * @brief 正向→反向过渡步数
 * @note 正向扫描结束后，需要经过 FOC_FSM_TRANSITION_STEPS 步的减速过渡
 *       使转子平稳停下来，再开始反向扫描
 */
#define FOC_FSM_TRANSITION_STEPS (250U)

/**
 * @def FOC_FSM_STOP_TIME
 * @brief 过渡结束后停止等待步数
 * @note 减速到零后额外等待 FOC_FSM_STOP_TIME 步，
 *       确保转子完全静止后再开始反向扫描
 */
#define FOC_FSM_STOP_TIME (10U)

/*============================================================================
 * 公共枚举定义
 *============================================================================*/

/**
 * @brief FOC 状态机状态枚举
 */
typedef enum __attribute__((packed)) {
  FOC_FSM_STATE_IDLE = 0,       /**< 空闲状态：无电流输出，等待开关信号 */
  FOC_FSM_STATE_ALIGN,          /**< 对齐状态：施加定向磁场将转子拉到已知位置 */
  FOC_FSM_STATE_ALIGNMENT,      /**< 校准状态：执行编码器正反向自动校准 */
  FOC_FSM_STATE_RUN,            /**< 运行状态：FOC 电流闭环正常运行 */
  FOC_FSM_STATE_HALL,           /**< 霍尔状态：使用线性霍尔传感器辅助启动 */
  FOC_FSM_STATE_STOP,           /**< 停止状态：PWM 关闭，电机停止 */
  FOC_FSM_STATE_COUNT           /**< 状态总数（用于数组定界，非实际状态） */
} foc_fsm_state_e;

/**
 * @brief FOC 状态机函数返回值枚举
 */
typedef enum __attribute__((packed)) {
  FOC_FSM_RET_OK = 0,            /**< 操作成功 */
  FOC_FSM_RET_ERROR,             /**< 操作失败（参数无效或内部错误） */
  FOC_FSM_RET_INVALID_STATE,     /**< 无效的状态切换请求 */
} foc_fsm_ret_e;

/**
 * @brief 编码器校准步骤枚举
 *
 * 校准流程分为四个阶段：
 *   - FORWARD：正向逐步增加角度，建立正向映射表
 *   - TRANSITION：减速过渡，从正向切换到反向
 *   - REVERSE：反向逐步减小角度，进行反向平均
 *   - COMPLETE：数据处理，检测方向并写入 Flash
 */
typedef enum __attribute__((packed)) {
  FOC_CALI_STEP_FORWARD = 0,     /**< 正向扫描阶段 */
  FOC_CALI_STEP_TRANSITION,      /**< 正反向过渡阶段 */
  FOC_CALI_STEP_REVERSE,         /**< 反向扫描阶段 */
  FOC_CALI_STEP_COMPLETE,        /**< 校准完成阶段（数据写入 Flash） */
  FOC_CALI_STEP_COUNT            /**< 校准步骤总数 */
} foc_cali_step_e;

/*============================================================================
 * 公共类型定义
 *============================================================================*/

/**
 * @brief FOC FSM 上下文结构体的前置声明
 */
typedef struct foc_fsm_context_s foc_fsm_context_t;

/**
 * @brief 编码器校准上下文结构体
 *
 * 存储编码器校准过程中的状态变量，
 * 包括当前采样索引、校准步骤、超时计数器和角度跟踪值。
 */
typedef struct __attribute__((packed)) {
  int16_t capture_idx;            /**< 校准采样索引
                                       正向扫描时递增，反向扫描时递减 */
  foc_cali_step_e step;          /**< 当前校准步骤（FORWARD / TRANSITION / REVERSE / COMPLETE） */
  uint16_t timeout_cnt;          /**< 超时/稳定等待计数器
                                       每个角度台阶需等待机械稳定后再采样 */
  uint16_t transition_cnt;       /**< 正向→反向过渡计数器 */
  q16_16_t last_angle_q;         /**< 上一次电气角度（Q16.16格式）
                                       用于判断角度是否达到下一个台阶 */
} foc_fsm_cali_context_t;

/**
 * @brief FOC FSM 上下文结构体
 */
struct foc_fsm_context_s {
  fsm_t fsm;                      /**< 通用 FSM 实例 */
  foc_fsm_cali_context_t cali_ctx; /**< 编码器校准上下文 */
  void* parent;                   /**< 父级 foc_context_t 指针 */
  void* flash_data;               /**< Flash 校准数据指针（motor_flash_config_t*） */
};

/*============================================================================
 * 公共API声明
 *============================================================================*/

/**
 * @brief 初始化 FOC 状态机
 *
 * 注册所有状态处理器、配置通用 FSM 框架、设置起始状态为 IDLE。
 *
 * @param ctx    FSM 上下文指针
 * @param parent 父级 FOC 上下文指针（foc_context_t*）
 * @return 操作结果
 *   - FOC_FSM_RET_OK：初始化成功
 *   - FOC_FSM_RET_ERROR：参数错误或 FSM 初始化失败
 */
foc_fsm_ret_e foc_fsm_init(foc_fsm_context_t* ctx, void* parent);

/**
 * @brief 执行一步状态机
 *
 * 在主循环中以 STATE_PERIOD（1ms）间隔周期调用。
 * 执行当前状态的处理函数，并根据返回值决定下一次状态。
 *
 * @param ctx FSM 上下文指针
 * @return 操作结果
 *   - FOC_FSM_RET_OK：执行成功
 *   - FOC_FSM_RET_ERROR：执行失败
 */
foc_fsm_ret_e foc_fsm_step(foc_fsm_context_t* ctx);

/**
 * @brief 请求状态切换
 *
 * 通过通用 FSM 框架切换到指定状态。
 * 当外部需要强制切换状态时使用（如上位机发送停机命令）。
 *
 * @param ctx   FSM 上下文指针
 * @param state 目标状态值
 * @return 操作结果
 *   - FOC_FSM_RET_OK：切换成功
 *   - FOC_FSM_RET_ERROR：参数错误
 *   - FOC_FSM_RET_INVALID_STATE：目标状态不可达
 */
foc_fsm_ret_e foc_fsm_request_state(foc_fsm_context_t* ctx, foc_fsm_state_e state);

/**
 * @brief 状态枚举转字符串
 *
 * 用于调试日志输出，便于跟踪状态机运行过程。
 *
 * @param state 状态枚举值
 * @return 状态名称字符串（如 "IDLE"、"RUN"），无效状态返回 "UNKNOWN"
 */
const char* foc_fsm_state_to_string(foc_fsm_state_e state);

/**
 * @brief 设置 Flash 校准数据指针
 *
 * 校准过程中需要将编码器映射数据写入 Flash，
 * 此函数在初始化后由外部调用，传递 Flash 数据结构的地址。
 *
 * @param ctx        FSM 上下文指针
 * @param flash_data Flash 校准数据指针（motor_flash_config_t*）
 */
void foc_fsm_set_flash_data(foc_fsm_context_t* ctx, void* flash_data);

#endif /* FOC_FSM_H */
