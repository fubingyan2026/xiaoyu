/**
 * @file    foc.h
 * @brief   FOC 主模块 - 磁场定向控制
 *
 * 整合数学库、SVPWM、端口抽象层、状态机等子模块，
 * 提供统一的 FOC 控制接口。支持多实例（多电机控制）。
 *
 * 模块架构：
 *   foc_context_t (主上下文)
 *     ├── foc_config_t (配置参数)
 *     ├── foc_port_t (硬件抽象层)
 *     ├── foc_pi_t [2] (D/Q 轴 PI 控制器)
 *     ├── foc_svpwm_context_t (SVPWM)
 *     ├── foc_fsm_context_t (状态机)
 *     ├── angle_sensor_context_t (角度传感器)
 *     └── 运行时状态变量
 *
 * @author  FOC Development Team
 * @date    2026-05-29
 * @version V4.0.0
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
 * @brief FOC 模块错误码枚举
 */
typedef enum {
    FOC_OK = 0,                         /**< 操作成功 */
    FOC_ERROR_NULL_PTR,                 /**< 空指针错误：传入的 ctx 或 config 为 NULL */
    FOC_ERROR_UNINITIALIZED,            /**< 未初始化：在初始化完成前调用了功能函数 */
    FOC_ERROR_PORT_INIT_FAILED,         /**< 端口初始化失败：回调函数缺失或无效 */
    FOC_ERROR_FSM_INIT_FAILED,          /**< 状态机初始化失败 */
    FOC_ERROR_GENERIC,                  /**< 通用错误 */
} foc_error_t;

/* ==================== 前置声明 ==================== */

typedef struct foc_context foc_context_t;

/* ==================== FOC 配置结构体 ==================== */

/**
 * @brief FOC 模块配置结构体
 *
 * 包含所有初始化时确定的配置参数，包括电机参数、PI 增益、
 * PLL 参数、硬件抽象层配置等。所有浮点参数在初始化时转换为 Q16.16 固定点格式。
 *
 * 使用方式：
 * @code
 *   foc_config_t config = {
 *       .motor_poles = 11,
 *       .pwm_period_s = 0.00005952381f,
 *       .id_kp = 0.314f, .id_ki = 1947.8f,
 *       .iq_kp = 0.314f, .iq_ki = 1947.8f,
 *       .v_bus = 24.0f,
 *       .port_config = { ... },
 *   };
 *   foc_init(&ctx, &config);
 * @endcode
 */
typedef struct {
  // 电机参数
  uint8_t motor_poles;             /**< 电机极对数
                                        @note 用于电气角度 = 极对数 × 机械角度 */

  // 控制周期
  float pwm_period_s;              /**< PWM 控制周期（秒）
                                        @note 电流环执行频率 = 1 / pwm_period_s
                                        影响 PI 控制器的 Ki 预乘 dt 值 */
  float fsm_period_s;              /**< 状态机周期（秒）
                                        @note 状态机以较慢频率运行（通常 1ms） */

  // 电流环 PI (D 轴)
  float id_kp;                     /**< D 轴电流环比例增益
                                        @note Kp = bandwidth * L */
  float id_ki;                     /**< D 轴电流环积分增益
                                        @note Ki = bandwidth * R */
  float id_out_max;                /**< D 轴最大输出电压
                                        @note 通常为 V_bus / √3 */
  float id_integ_sat;              /**< D 轴积分饱和限制
                                        @note 通常等于 id_out_max */

  // 电流环 PI (Q 轴)
  float iq_kp;                     /**< Q 轴电流环比例增益 */
  float iq_ki;                     /**< Q 轴电流环积分增益 */
  float iq_out_max;                /**< Q 轴最大输出电压 */
  float iq_integ_sat;              /**< Q 轴积分饱和限制 */

  // PLL（锁相环）
  float pll_kp;                    /**< PLL 比例增益
                                        @note 越大响应越快，但对噪声更敏感 */
  float pll_ki;                    /**< PLL 积分增益
                                        @note 决定稳态误差消除速度 */
  float pll_speed_limit;           /**< PLL 速度输出限制（rad/s，绝对值）
                                        @note 防止估计速度异常跳变 */

  // SVPWM
  float v_bus;                     /**< 母线电压（伏特）
                                        @note 用于 SVPWM 占空比归一化 */
  float max_duty_ratio;            /**< 最大占空比
                                        @note 通常 < 1.0，保留死区余量 */

  // 硬件抽象层配置
  foc_port_config_t port_config;   /**< 端口配置（含所有硬件回调函数指针）
                                        @note 移植新平台时主要修改此字段 */

  // 传感器类型
  angle_sensor_type_e sensor_type; /**< 角度传感器类型
                                        @note 如 AS5600、MT6816 等 */

  // 校准数据指针
  void* flash_data;                /**< Flash 校准数据指针
                                        @note 指向 motor_flash_config_t 结构体，
                                        用于存储编码器角度映射表 */
} foc_config_t;

/* ==================== FOC 上下文结构体 ==================== */

/**
 * @brief FOC 模块上下文结构体
 *
 * 包含所有运行时状态，支持多实例（每个电机对应一个上下文）。
 * 配置参数嵌套存储在 config 成员中，遵循 Middleware Module Pattern。
 *
 * 初始化顺序：foc_init() 会自动初始化所有子模块，
 * 反初始化：foc_deinit() 会按逆序关闭所有子模块。
 */
struct foc_context {
  foc_config_t config;             /**< 配置参数（嵌套，保持 const-correct） */

  // 控制状态
  bool sw;                         /**< FOC 开关（0: 关闭, 1: 开启）
                                        @note 由 foc_fsm_state_t 状态机控制 */
  q16_16_t target_iq_q;            /**< 目标 Q 轴电流（Q16.16 格式，安培）
                                        @note 由状态机或上层速度环设置 */
  q16_16_t target_id_q;            /**< 目标 D 轴电流（Q16.16 格式，安培）
                                        @note 通常设为 0（MTPA 时为负值） */
  q16_16_t omega_q;                /**< 角速度（Q16.16 格式，rad/s）
                                        @note IF 启动时由状态机直接设定 */
  uint16_t raw_angle_q;            /**< 编码器原始读数
                                        @note 编码器计数值（0 ~ encoder_lines - 1） */
  q16_16_t electrical_angle_q;     /**< 电气角度（Q16.16 格式，弧度）
                                        @note 由编码器映射表或状态机提供 */
  q16_16_t pll_phase_q;            /**< PLL 输出相位（Q16.16 格式，弧度）
                                        @note 锁相环对电气角度进行滤波和预测 */
  q16_16_t pll_velocity_q;         /**< PLL 输出角速度（Q16.16 格式，rad/s）
                                        @note 锁相环估计的转速 */
  float pll_velocity_rpm;          /**< PLL 输出速度（RPM）
                                        @note 用于调试输出和用户界面 */

  // 子模块上下文
  foc_pi_t pi_id;                  /**< D 轴 PI 控制器 */
  foc_pi_t pi_iq;                  /**< Q 轴 PI 控制器 */
  foc_svpwm_context_t svpwm;       /**< SVPWM 上下文 */
  foc_port_t port;                 /**< 端口管理器（硬件抽象层） */
  foc_fsm_context_t fsm;           /**< FSM 状态机 */
  angle_sensor_context_t sensor;   /**< 角度传感器上下文 */

  // 滤波器状态
  q16_16_t lpf_id_q;               /**< D 轴电流低通滤波状态（Q16.16 格式）
                                        @note 用于 PI 控制的反馈值平滑 */
  q16_16_t lpf_iq_q;               /**< Q 轴电流低通滤波状态（Q16.16 格式） */

  // 采样数据
  q16_16_t current_sample[3];      /**< 三相电流采样值（Q16.16 格式，安培）
                                        @note [0]: A 相, [1]: B 相, [2]: C 相 */

  // ADC DMA 缓冲区（供硬件回调使用）
  uint32_t adc_dma_buffer[3];      /**< ADC DMA 采样值缓冲区
                                        @note 供底层 HAL 层 DMA 传输使用 */
  q16_16_t adc_offset[3];          /**< ADC 初始偏置值（Q16.16 格式）
                                        @note 零电流时的 ADC 读数，用于去偏置 */

  // 状态
  bool initialized;                /**< 初始化标志
                                        @note 由 foc_init() 置位，foc_deinit() 清零 */
};

/* ==================== 公共 API ==================== */

/**
 * @brief 初始化 FOC 模块
 *
 * 初始化所有子模块并按正确顺序配置：
 * 端口 → ADC → PI 控制器 → SVPWM → 状态机 → Flash → PWM
 *
 * @param ctx    FOC 上下文指针（由调用者分配内存）
 * @param config FOC 配置指针（包含所有电机和硬件参数）
 * @return 操作结果错误码
 */
foc_error_t foc_init(foc_context_t* ctx, const foc_config_t* config);

/**
 * @brief 反初始化 FOC 模块
 *
 * 停止 PWM、关闭端口、清除初始化标志。
 * 调用后需重新 foc_init() 才能继续使用。
 *
 * @param ctx FOC 上下文指针
 */
void foc_deinit(foc_context_t* ctx);

/**
 * @brief 检查 FOC 模块是否已初始化
 *
 * @param ctx FOC 上下文指针
 * @return true  已初始化
 * @return false 未初始化或 ctx 为 NULL
 */
bool foc_is_initialized(const foc_context_t* ctx);

/**
 * @brief FOC 中断处理函数
 *
 * 在 ADC DMA 完成中断后调用，执行完整的 FOC 控制流水线：
 * 编码器读取 → PLL → Clarke/Park → PI → 逆 Park → SVPWM → PWM
 *
 * 必须在中断上下文中以 PWM 频率（约 16.8kHz）周期调用。
 *
 * @param ctx FOC 上下文指针
 */
void foc_irq_handler(foc_context_t* ctx);

/**
 * @brief 请求状态切换
 *
 * 通过 FOC 状态机切换到指定运行状态。
 *
 * @param ctx   FOC 上下文指针
 * @param state 目标状态值
 * @return 操作结果错误码
 */
foc_error_t foc_request_state(foc_context_t* ctx, foc_fsm_state_e state);

/**
 * @brief 获取当前状态机状态
 *
 * @param ctx FOC 上下文指针
 * @return 当前状态值，未初始化时返回 FOC_FSM_STATE_IDLE
 */
foc_fsm_state_e foc_current_state(const foc_context_t* ctx);

/* ==================== 访问器函数 ==================== */

/**
 * @name  设置器
 * @brief 设置 FOC 运行时参数（带空指针保护）
 */
/**@{*/
void foc_set_target_iq(foc_context_t* ctx, q16_16_t iq);
void foc_set_target_id(foc_context_t* ctx, q16_16_t id);
void foc_set_sw(foc_context_t* ctx, bool sw);
void foc_set_omega(foc_context_t* ctx, q16_16_t omega);
void foc_set_pll_phase(foc_context_t* ctx, q16_16_t phase);
void foc_set_electrical_angle(foc_context_t* ctx, q16_16_t angle);
/**@}*/

/**
 * @name  获取器
 * @brief 获取 FOC 运行时参数（带空指针保护，失败返回 0/false）
 */
/**@{*/
q16_16_t foc_get_target_iq(const foc_context_t* ctx);
q16_16_t foc_get_target_id(const foc_context_t* ctx);
q16_16_t foc_get_omega(const foc_context_t* ctx);
q16_16_t foc_get_pll_phase(const foc_context_t* ctx);
q16_16_t foc_get_electrical_angle(const foc_context_t* ctx);
float foc_get_velocity_rpm(const foc_context_t* ctx);
bool foc_get_sw(const foc_context_t* ctx);
uint16_t foc_get_raw_angle(const foc_context_t* ctx);
/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
