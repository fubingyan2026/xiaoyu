/**
 * @file    foc.h
 * @brief   FOC主模块 - 磁场定向控制
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V4.0.0
 *
 * @description
 * FOC（Field-Oriented Control）主模块，整合数学库、SVPWM、端口抽象层、
 * 状态机等子模块，提供统一的FOC控制接口。支持多实例。
 */

#ifndef FOC_H
#define FOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "angle_sensor.h"
#include "foc_config.h"
#include "foc_core/foc_core.h"
#include "foc_fsm.h"
#include "foc_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============= FOC 错误码枚举 ============= */

/**
 * @brief FOC模块错误码枚举
 */
typedef enum {
    FOC_OK = 0, /**< 操作成功 */
    FOC_ERROR_NULL_PTR, /**< 空指针错误 */
    FOC_ERROR_UNINITIALIZED, /**< 未初始化 */
    FOC_ERROR_PORT_INIT_FAILED, /**< 端口初始化失败 */
    FOC_ERROR_FSM_INIT_FAILED, /**< 状态机初始化失败 */
    FOC_ERROR_GENERIC, /**< 通用错误 */
} foc_error_t;

/* ==================== 前置声明 ==================== */

typedef struct foc_context foc_context_t;

/* ==================== FOC配置结构体 ==================== */

/**
 * @brief FOC模块配置结构体
 *
 * 包含所有初始化时确定的配置参数，包括电机参数、PI增益、PLL参数、
 * 硬件抽象层配置等。回调函数通过port_config成员配置。
 */
typedef struct {
  // 电机参数
  uint8_t motor_poles;             /**< 电机极对数 */

  // 控制周期
  float pwm_period_s;              /**< PWM控制周期（秒） */
  float fsm_period_s;              /**< 状态机周期（秒） */

  // 电流环 PI (D轴)
  float id_kp;                     /**< D轴比例增益 */
  float id_ki;                     /**< D轴积分增益 */
  float id_out_max;                /**< D轴最大输出 */
  float id_integ_sat;              /**< D轴积分饱和限制 */

  // 电流环 PI (Q轴)
  float iq_kp;                     /**< Q轴比例增益 */
  float iq_ki;                     /**< Q轴积分增益 */
  float iq_out_max;                /**< Q轴最大输出 */
  float iq_integ_sat;              /**< Q轴积分饱和限制 */

  // PLL
  float pll_kp;                    /**< PLL比例增益 */
  float pll_ki;                    /**< PLL积分增益 */
  float pll_speed_limit;           /**< PLL速度输出限制（rad/s，绝对值） */

  // SVPWM
  float v_bus;                     /**< 母线电压 */
  float max_duty_ratio;            /**< 最大占空比 */

  // 硬件抽象层配置
  foc_port_config_t port_config;   /**< 端口配置（含回调函数） */

  // 传感器类型
  angle_sensor_type_e sensor_type; /**< 角度传感器类型 */

  // 校准数据指针
  void* flash_data;                /**< Flash校准数据指针 */
} foc_config_t;

/* ==================== FOC上下文结构体 ==================== */

/**
 * @brief FOC模块上下文结构体
 *
 * 包含所有运行时状态，支持多实例。配置参数嵌套存储在config成员中。
 */
struct foc_context {
  foc_config_t config;             /**< 配置参数 */

  // 控制状态
  bool sw;                         /**< FOC开关（0:关, 1:开） */
  q16_16_t target_iq_q;            /**< 目标Q轴电流（Q16.16格式） */
  q16_16_t target_id_q;            /**< 目标D轴电流（Q16.16格式） */
  q16_16_t omega_q;                /**< 角速度（Q16.16格式） */
  uint16_t raw_angle_q;            /**< 原始角度（编码器读数） */
  q16_16_t electrical_angle_q;     /**< 电气角度（Q16.16格式） */
  q16_16_t pll_phase_q;            /**< PLL输出相位（Q16.16格式） */
  q16_16_t pll_velocity_q;         /**< PLL输出速度（Q16.16格式） */
  float pll_velocity_rpm;          /**< PLL输出速度（RPM） */

  // 子模块上下文
  foc_pi_t pi_id;                  /**< D轴PI控制器 */
  foc_pi_t pi_iq;                  /**< Q轴PI控制器 */
  foc_svpwm_context_t svpwm;       /**< SVPWM上下文 */
  foc_port_t port;                 /**< 端口管理器 */
  foc_fsm_context_t fsm;           /**< FSM状态机 */
  angle_sensor_context_t sensor;   /**< 角度传感器上下文 */

  // 滤波器状态
  q16_16_t lpf_id_q;               /**< D轴电流低通滤波状态（Q16.16格式） */
  q16_16_t lpf_iq_q;               /**< Q轴电流低通滤波状态（Q16.16格式） */

  // 采样数据
  q16_16_t current_sample[3];      /**< 三相电流采样值（Q16.16格式） */

  // ADC DMA 缓冲区（供硬件回调使用）
  uint32_t adc_dma_buffer[3];      /**< ADC DMA采样值缓冲区 */
  q16_16_t adc_offset[3];          /**< ADC初始偏置值（Q16.16格式） */

  // 状态
  bool initialized;                /**< 初始化标志 */
};

/* ==================== 公共API ==================== */

/**
 * @brief 初始化FOC模块
 * @param ctx FOC上下文指针
 * @param config FOC配置指针
 * @return 操作结果错误码
 */
foc_error_t foc_init(foc_context_t* ctx, const foc_config_t* config);

/**
 * @brief 反初始化FOC模块
 * @param ctx FOC上下文指针
 */
void foc_deinit(foc_context_t* ctx);

/**
 * @brief 检查FOC模块是否已初始化
 * @param ctx FOC上下文指针
 * @return true表示已初始化，false表示未初始化
 */
bool foc_is_initialized(const foc_context_t* ctx);

/**
 * @brief FOC中断处理函数（替代 foc_adc_irq_calc）
 *
 * 在ADC DMA完成中断后调用，执行完整的FOC电流环计算：
 * 编码器读取 → PLL → Clarke/Park变换 → PI控制 → 逆Park → SVPWM → PWM输出
 *
 * @param ctx FOC上下文指针
 */
void foc_irq_handler(foc_context_t* ctx);

/**
 * @brief 请求状态切换
 * @param ctx FOC上下文指针
 * @param state 目标状态
 * @return 操作结果错误码
 */
foc_error_t foc_request_state(foc_context_t* ctx, foc_fsm_state_e state);

/**
 * @brief 获取当前状态机状态
 * @param ctx FOC上下文指针
 * @return 当前状态
 */
foc_fsm_state_e foc_current_state(const foc_context_t* ctx);

/* ==================== 访问器函数 ==================== */

void foc_set_target_iq(foc_context_t* ctx, q16_16_t iq);
void foc_set_target_id(foc_context_t* ctx, q16_16_t id);
void foc_set_sw(foc_context_t* ctx, bool sw);
void foc_set_omega(foc_context_t* ctx, q16_16_t omega);
void foc_set_pll_phase(foc_context_t* ctx, q16_16_t phase);
void foc_set_electrical_angle(foc_context_t* ctx, q16_16_t angle);

q16_16_t foc_get_target_iq(const foc_context_t* ctx);
q16_16_t foc_get_target_id(const foc_context_t* ctx);
q16_16_t foc_get_omega(const foc_context_t* ctx);
q16_16_t foc_get_pll_phase(const foc_context_t* ctx);
q16_16_t foc_get_electrical_angle(const foc_context_t* ctx);
float foc_get_velocity_rpm(const foc_context_t* ctx);
bool foc_get_sw(const foc_context_t* ctx);
uint16_t foc_get_raw_angle(const foc_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
