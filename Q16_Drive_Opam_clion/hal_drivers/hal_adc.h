//
// Created by fubingyan on 25-3-30.
//

#ifndef __HAL_ADC_H
#define __HAL_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief ADC 操作错误码枚举
 */
typedef enum {
  HAL_ADC_OK = 0,              /**< 操作成功 */
  HAL_ADC_ERROR_INVALID_PARAM, /**< 无效参数 */
  HAL_ADC_ERROR_UNINITIALIZED, /**< 未初始化 */
  HAL_ADC_ERROR_HARDWARE,      /**< 硬件错误 */
  HAL_ADC_ERROR_BUSY,          /**< 设备忙 */
  HAL_ADC_ERROR_UNSUPPORTED,   /**< 不支持的操作 */
  HAL_ADC_ERROR_TIMEOUT,       /**< 超时 */
  HAL_ADC_ERROR_DMA,           /**< DMA错误 */
  HAL_ADC_ERROR_CALIBRATION,   /**< 校准错误 */
} hal_adc_error_t;

/**
 * @brief ADC实例定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_INSTANCE_1 = 0, /**< ADC实例1 */
  HAL_ADC_INSTANCE_2,     /**< ADC实例2 */
  HAL_ADC_INSTANCE_LEN    /**< ADC实例数量 */
} hal_adc_instance_t;

/**
 * @brief ADC通道定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_CHANNEL_0 = 0, /**< ADC通道0 */
  HAL_ADC_CHANNEL_1,     /**< ADC通道1 */
  HAL_ADC_CHANNEL_2,     /**< ADC通道2 */
  HAL_ADC_CHANNEL_3,     /**< ADC通道3 */
  HAL_ADC_CHANNEL_4,     /**< ADC通道4 */
  HAL_ADC_CHANNEL_5,     /**< ADC通道5 */
  HAL_ADC_CHANNEL_6,     /**< ADC通道6 */
  HAL_ADC_CHANNEL_7,     /**< ADC通道7 */
  HAL_ADC_CHANNEL_8,     /**< ADC通道8 */
  HAL_ADC_CHANNEL_9,     /**< ADC通道9 */
  HAL_ADC_CHANNEL_10,    /**< ADC通道10 */
  HAL_ADC_CHANNEL_11,    /**< ADC通道11 */
  HAL_ADC_CHANNEL_12,    /**< ADC通道12 */
  HAL_ADC_CHANNEL_13,    /**< ADC通道13 */
  HAL_ADC_CHANNEL_14,    /**< ADC通道14 */
  HAL_ADC_CHANNEL_15,    /**< ADC通道15 */
  HAL_ADC_CHANNEL_16,    /**< ADC通道16（通常为内部参考电压） */
  HAL_ADC_CHANNEL_17,    /**< ADC通道17（通常为温度传感器） */
  HAL_ADC_CHANNEL_18,    /**< ADC通道18 */
  HAL_ADC_CHANNEL_LEN    /**< ADC通道数量 */
} hal_adc_channel_t;

/**
 * @brief ADC数据对齐方式定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_DATAALIGN_RIGHT = 0, /**< 右对齐 */
  HAL_ADC_DATAALIGN_LEFT       /**< 左对齐 */
} hal_adc_dataalign_t;

/**
 * @brief ADC分辨率定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_RESOLUTION_12B = 0, /**< 12位分辨率 */
  HAL_ADC_RESOLUTION_10B,     /**< 10位分辨率 */
  HAL_ADC_RESOLUTION_8B,      /**< 8位分辨率 */
  HAL_ADC_RESOLUTION_6B       /**< 6位分辨率 */
} hal_adc_resolution_t;

/**
 * @brief ADC采样周期定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_SAMPLETIME_2CYCLES_5 = 0, /**< 2.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_6CYCLES_5,     /**< 6.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_12CYCLES_5,    /**< 12.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_24CYCLES_5,    /**< 24.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_47CYCLES_5,    /**< 47.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_92CYCLES_5,    /**< 92.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_247CYCLES_5,   /**< 247.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_640CYCLES_5,   /**< 640.5个ADC时钟周期 */
  HAL_ADC_SAMPLETIME_3CYCLES_5      /**< 3.5个ADC时钟周期（特殊模式） */
} hal_adc_sampletime_t;

/**
 * @brief ADC触发源定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_TRIGGER_SOFTWARE = 0, /**< 软件触发 */
  HAL_ADC_TRIGGER_TIMER1_CC1,   /**< TIM1 CC1触发 */
  HAL_ADC_TRIGGER_TIMER1_CC2,   /**< TIM1 CC2触发 */
  HAL_ADC_TRIGGER_TIMER1_CC3,   /**< TIM1 CC3触发 */
  HAL_ADC_TRIGGER_TIMER2_CC2,   /**< TIM2 CC2触发 */
  HAL_ADC_TRIGGER_TIMER3_TRGO,  /**< TIM3 TRGO触发 */
  HAL_ADC_TRIGGER_TIMER4_CC4,   /**< TIM4 CC4触发 */
  HAL_ADC_TRIGGER_TIMER5_CC1,   /**< TIM5 CC1触发 */
  HAL_ADC_TRIGGER_TIMER5_CC2,   /**< TIM5 CC2触发 */
  HAL_ADC_TRIGGER_TIMER5_CC3,   /**< TIM5 CC3触发 */
  HAL_ADC_TRIGGER_TIMER8_CC1,   /**< TIM8 CC1触发 */
  HAL_ADC_TRIGGER_TIMER8_TRGO,  /**< TIM8 TRGO触发 */
  HAL_ADC_TRIGGER_EXTI_LINE11   /**< EXTI Line11触发 */
} hal_adc_trigger_t;

/**
 * @brief ADC扫描模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_SCAN_MODE_DISABLE = 0, /**< 禁用扫描模式 */
  HAL_ADC_SCAN_MODE_ENABLE       /**< 启用扫描模式 */
} hal_adc_scan_mode_t;

/**
 * @brief ADC连续转换模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_CONTINUOUS_DISABLE = 0, /**< 单次转换模式 */
  HAL_ADC_CONTINUOUS_ENABLE       /**< 连续转换模式 */
} hal_adc_continuous_t;

/**
 * @brief ADC DMA模式定义
 */
typedef enum __attribute__((packed)) {
  HAL_ADC_DMA_DISABLE = 0, /**< 禁用DMA */
  HAL_ADC_DMA_ENABLE       /**< 启用DMA */
} hal_adc_dma_mode_t;

/**
 * @brief ADC通道配置结构体
 */
typedef struct {
  hal_adc_channel_t channel;        /**< ADC通道 */
  hal_adc_sampletime_t sample_time; /**< 采样时间 */
  uint8_t rank;                     /**< 通道在序列中的排名（0-15） */
} hal_adc_channel_config_t;

/**
 * @brief ADC配置结构体
 */
typedef struct {
  hal_adc_instance_t instance;     /**< ADC实例 */
  hal_adc_resolution_t resolution; /**< 分辨率 */
  hal_adc_dataalign_t data_align;  /**< 数据对齐方式 */
  hal_adc_scan_mode_t scan_mode;   /**< 扫描模式 */
  hal_adc_continuous_t continuous; /**< 连续转换模式 */
  hal_adc_trigger_t trigger;       /**< 触发源 */
  hal_adc_dma_mode_t dma_mode;     /**< DMA模式 */
  uint32_t clock_prescaler;        /**< 时钟分频 */
  uint8_t nbr_of_conversion;       /**< 转换通道数量 */
  uint32_t timeout;                /**< 超时时间(ms) */
} hal_adc_config_t;

/**
 * @brief ADC DMA配置结构体
 */
typedef struct {
  uint32_t* buffer;       /**< DMA缓冲区指针 */
  uint32_t buffer_length; /**< 缓冲区长度 */
  bool circular_mode;     /**< 是否循环模式 */
} hal_adc_dma_config_t;

/**
 * @brief ADC校准配置结构体
 */
typedef struct {
  bool single_ended;         /**< 是否单端校准 */
  bool differential;         /**< 是否差分校准 */
  uint8_t calibration_count; /**< 校准次数 */
} hal_adc_calibration_config_t;

/**
 * @brief ADC 上下文结构体前向声明
 */
typedef struct hal_adc_context hal_adc_context_t;

/**
 * @brief ADC 转换完成回调函数类型
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param value 转换结果值
 * @param user_data 用户自定义数据指针
 */
typedef void (*hal_adc_conv_callback_t)(hal_adc_context_t* ctx,
                                        hal_adc_instance_t instance,
                                        uint32_t value, void* user_data);

/**
 * @brief ADC DMA转换完成回调函数类型
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param buffer 数据缓冲区指针
 * @param length 数据长度
 * @param user_data 用户自定义数据指针
 */
typedef void (*hal_adc_dma_callback_t)(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       uint32_t* buffer, uint32_t length,
                                       void* user_data);

/**
 * @brief ADC 上下文结构体
 *
 * 用于保存 ADC 实例的状态信息，支持多实例操作。
 */
struct hal_adc_context {
  const struct hal_adc_ops* ops; /**< 平台特定的操作函数指针 */
  volatile uint8_t initialized;  /**< 初始化标志（0=未初始化，1=已初始化） */
  hal_adc_conv_callback_t conv_callback; /**< 转换完成回调函数指针 */
  hal_adc_dma_callback_t dma_callback;   /**< DMA完成回调函数指针 */
  void* user_data;                       /**< 用户自定义数据 */
  hal_adc_config_t config;               /**< 当前 ADC 配置 */
  volatile uint8_t busy;                 /**< 忙标志（0=空闲，1=忙） */
};

/**
 * @brief ADC 操作函数结构体
 *
 * 定义了平台特定的 ADC 操作接口，各平台需要实现这些函数。
 */
typedef struct hal_adc_ops {
  /**
   * @brief 初始化 ADC
   * @param ctx ADC 上下文指针
   * @param config ADC 配置结构体指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*init)(hal_adc_context_t* ctx,
                          const hal_adc_config_t* config);

  /**
   * @brief 反初始化 ADC
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @return 操作结果错误码
   */
  hal_adc_error_t (*deinit)(hal_adc_context_t* ctx,
                            hal_adc_instance_t instance);

  /**
   * @brief 配置ADC通道
   * @param ctx ADC 上下文指针
   * @param channel_config 通道配置结构体指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*config_channel)(
      hal_adc_context_t* ctx, const hal_adc_channel_config_t* channel_config);

  /**
   * @brief 启动ADC转换（阻塞）
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param value 输出参数，存储转换结果
   * @return 操作结果错误码
   */
  hal_adc_error_t (*start_conversion)(hal_adc_context_t* ctx,
                                      hal_adc_instance_t instance,
                                      uint32_t* value);

  /**
   * @brief 启动ADC转换（异步）
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @return 操作结果错误码
   */
  hal_adc_error_t (*start_conversion_async)(hal_adc_context_t* ctx,
                                            hal_adc_instance_t instance);

  /**
   * @brief 停止ADC转换
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @return 操作结果错误码
   */
  hal_adc_error_t (*stop_conversion)(hal_adc_context_t* ctx,
                                     hal_adc_instance_t instance);

  /**
   * @brief 启动ADC DMA转换
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param dma_config DMA配置结构体指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*start_dma)(hal_adc_context_t* ctx,
                               hal_adc_instance_t instance,
                               const hal_adc_dma_config_t* dma_config);

  /**
   * @brief 停止ADC DMA转换
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @return 操作结果错误码
   */
  hal_adc_error_t (*stop_dma)(hal_adc_context_t* ctx,
                              hal_adc_instance_t instance);

  /**
   * @brief 获取转换结果
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param value 输出参数，存储转换结果
   * @return 操作结果错误码
   */
  hal_adc_error_t (*get_value)(hal_adc_context_t* ctx,
                               hal_adc_instance_t instance, uint32_t* value);

  /**
   * @brief 校准ADC
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param calibration_config 校准配置结构体指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*calibrate)(
      hal_adc_context_t* ctx, hal_adc_instance_t instance,
      const hal_adc_calibration_config_t* calibration_config);

  /**
   * @brief 设置分辨率
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param resolution 分辨率
   * @return 操作结果错误码
   */
  hal_adc_error_t (*set_resolution)(hal_adc_context_t* ctx,
                                    hal_adc_instance_t instance,
                                    hal_adc_resolution_t resolution);

  /**
   * @brief 获取分辨率
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param resolution 输出参数，返回分辨率
   * @return 操作结果错误码
   */
  hal_adc_error_t (*get_resolution)(hal_adc_context_t* ctx,
                                    hal_adc_instance_t instance,
                                    hal_adc_resolution_t* resolution);

  /**
   * @brief 注册转换完成回调函数
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param callback 回调函数指针
   * @param user_data 用户自定义数据指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*register_callback)(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       hal_adc_conv_callback_t callback,
                                       void* user_data);

  /**
   * @brief 注册DMA完成回调函数
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   * @param callback 回调函数指针
   * @param user_data 用户自定义数据指针
   * @return 操作结果错误码
   */
  hal_adc_error_t (*register_dma_callback)(hal_adc_context_t* ctx,
                                           hal_adc_instance_t instance,
                                           hal_adc_dma_callback_t callback,
                                           void* user_data);

  /**
   * @brief ADC中断处理
   * @param ctx ADC 上下文指针
   * @param instance ADC 实例
   */
  void (*irq_handler)(hal_adc_context_t* ctx, hal_adc_instance_t instance);
} hal_adc_ops_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/**
 * @brief 进入临界区宏
 *
 * 在多线程/中断环境中保护共享资源，防止竞态条件。
 * 当前为空实现，可根据实际平台需求替换为真正的临界区保护代码。
 */
#define HAL_ADC_ENTER_CRITICAL() \
  do {                           \
  } while (0)

/**
 * @brief 退出临界区宏
 *
 * 与 HAL_ADC_ENTER_CRITICAL 配对使用。
 */
#define HAL_ADC_EXIT_CRITICAL() \
  do {                          \
  } while (0)

/**
 * @brief ADC参考电压（单位：mV）
 */
#define HAL_ADC_VREF_VALUE (3300U)

/**
 * @brief 12位ADC最大值
 */
#define HAL_ADC_12BIT_MAX_VALUE (4095U)

/**
 * @brief 10位ADC最大值
 */
#define HAL_ADC_10BIT_MAX_VALUE (1023U)

/**
 * @brief 8位ADC最大值
 */
#define HAL_ADC_8BIT_MAX_VALUE (255U)

/**
 * @brief 6位ADC最大值
 */
#define HAL_ADC_6BIT_MAX_VALUE (63U)

/**
 * @brief 将ADC原始值转换为电压值（mV）
 * @param raw_value ADC原始值
 * @param resolution ADC分辨率
 * @return 电压值（mV）
 */
#define HAL_ADC_TO_VOLTAGE(raw_value, resolution) \
  ((uint32_t)(((uint64_t)(raw_value) * HAL_ADC_VREF_VALUE) / (resolution)))

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化 ADC
 * @param ctx ADC 上下文指针
 * @param config ADC 配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_init(hal_adc_context_t* ctx,
                             const hal_adc_config_t* config);

/**
 * @brief 反初始化 ADC
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_deinit(hal_adc_context_t* ctx,
                               hal_adc_instance_t instance);

/**
 * @brief 配置ADC通道
 * @param ctx ADC 上下文指针
 * @param channel_config 通道配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_config_channel(
    hal_adc_context_t* ctx, const hal_adc_channel_config_t* channel_config);

/**
 * @brief 启动ADC转换（阻塞）
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param value 输出参数，存储转换结果
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_conversion(hal_adc_context_t* ctx,
                                         hal_adc_instance_t instance,
                                         uint32_t* value);

/**
 * @brief 启动ADC转换（异步）
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_conversion_async(hal_adc_context_t* ctx,
                                               hal_adc_instance_t instance);

/**
 * @brief 停止ADC转换
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_stop_conversion(hal_adc_context_t* ctx,
                                        hal_adc_instance_t instance);

/**
 * @brief 启动ADC DMA转换
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param dma_config DMA配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_start_dma(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance,
                                  const hal_adc_dma_config_t* dma_config);

/**
 * @brief 停止ADC DMA转换
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_stop_dma(hal_adc_context_t* ctx,
                                 hal_adc_instance_t instance);

/**
 * @brief 获取转换结果
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param value 输出参数，存储转换结果
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_value(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance, uint32_t* value);

/**
 * @brief 校准ADC
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param calibration_config 校准配置结构体指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_calibrate(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_calibration_config_t* calibration_config);

/**
 * @brief 设置分辨率
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param resolution 分辨率
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_set_resolution(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       hal_adc_resolution_t resolution);

/**
 * @brief 获取分辨率
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param resolution 输出参数，返回分辨率
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_resolution(hal_adc_context_t* ctx,
                                       hal_adc_instance_t instance,
                                       hal_adc_resolution_t* resolution);

/**
 * @brief 注册转换完成回调函数
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param callback 回调函数指针
 * @param user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_register_callback(hal_adc_context_t* ctx,
                                          hal_adc_instance_t instance,
                                          hal_adc_conv_callback_t callback,
                                          void* user_data);

/**
 * @brief 注册DMA完成回调函数
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 * @param callback 回调函数指针
 * @param user_data 用户自定义数据指针
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_register_dma_callback(hal_adc_context_t* ctx,
                                              hal_adc_instance_t instance,
                                              hal_adc_dma_callback_t callback,
                                              void* user_data);

/**
 * @brief ADC中断处理
 * @param ctx ADC 上下文指针
 * @param instance ADC 实例
 */
void hal_adc_irq_handler(hal_adc_context_t* ctx, hal_adc_instance_t instance);

/**
 * @brief 设置 ADC 操作函数
 * @param ctx ADC 上下文指针
 * @param ops ADC 操作函数结构体指针
 * @return 操作结果错误码
 *
 * @note 通常不需要直接调用此函数，使用平台特定的初始化函数即可。
 *       此函数主要用于多平台切换或单元测试场景。
 */
hal_adc_error_t hal_adc_set_ops(hal_adc_context_t* ctx,
                                const hal_adc_ops_t* ops);

/**
 * @brief STM32 平台 ADC 上下文初始化函数
 * @param ctx ADC 上下文指针
 * @return 操作结果错误码
 *
 * 使用此函数初始化 STM32 平台的 ADC 上下文，会自动设置好平台特定的操作函数。
 */
hal_adc_error_t stm32_adc_init_context(hal_adc_context_t* ctx);

/**
 * @brief 检查ADC是否已初始化
 * @param ctx ADC 上下文指针
 * @param initialized 输出参数，返回初始化状态（true=已初始化，false=未初始化）
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_is_initialized(hal_adc_context_t* ctx,
                                       bool* initialized);

/**
 * @brief 获取当前ADC配置
 * @param ctx ADC 上下文指针
 * @param config 输出参数，返回当前配置
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_get_config(hal_adc_context_t* ctx,
                                   hal_adc_config_t* config);

/**
 * @brief 更新ADC配置
 * @param ctx ADC 上下文指针
 * @param config 新的配置结构体指针
 * @return 操作结果错误码
 *
 * @note 此函数会更新所有配置参数
 */
hal_adc_error_t hal_adc_update_config(hal_adc_context_t* ctx,
                                      const hal_adc_config_t* config);

/**
 * @brief 检查ADC是否忙
 * @param ctx ADC 上下文指针
 * @param busy 输出参数，返回忙状态（true=忙，false=空闲）
 * @return 操作结果错误码
 */
hal_adc_error_t hal_adc_is_busy(hal_adc_context_t* ctx, bool* busy);

/**
 * @brief 将ADC原始值转换为电压值
 * @param raw_value ADC原始值
 * @param resolution ADC分辨率
 * @param vref_mv 参考电压（mV）
 * @return 电压值（mV）
 */
uint32_t hal_adc_raw_to_voltage(uint32_t raw_value,
                                hal_adc_resolution_t resolution,
                                uint32_t vref_mv);

/**
 * @brief 获取ADC最大值
 * @param resolution ADC分辨率
 * @return 最大值
 */
uint32_t hal_adc_get_max_value(hal_adc_resolution_t resolution);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_ADC_H */
