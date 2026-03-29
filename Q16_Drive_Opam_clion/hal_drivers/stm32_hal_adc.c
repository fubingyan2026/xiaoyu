//
// Created by fubingyan on 25-3-30.
//

/**
 * @file    stm32_hal_adc.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-03-30
 * @brief   STM32平台硬件抽象层 - ADC实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "hal_adc.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_adc.h"
/* Private variables ---------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

static ADC_HandleTypeDef* adc_handle_map[HAL_ADC_INSTANCE_LEN] = {
    &hadc1,
    &hadc2,
};

static hal_adc_context_t* adc_context_map[HAL_ADC_INSTANCE_LEN] = {NULL};

static DMA_HandleTypeDef* adc_dma_handle_map[HAL_ADC_INSTANCE_LEN] = {NULL};

/* Private function prototypes -----------------------------------------------*/
static hal_adc_error_t stm32_adc_init(hal_adc_context_t* ctx,
                                      const hal_adc_config_t* config);
static hal_adc_error_t stm32_adc_deinit(hal_adc_context_t* ctx,
                                        hal_adc_instance_t instance);
static hal_adc_error_t stm32_adc_config_channel(
    hal_adc_context_t* ctx, const hal_adc_channel_config_t* channel_config);
static hal_adc_error_t stm32_adc_start_conversion(hal_adc_context_t* ctx,
                                                  hal_adc_instance_t instance,
                                                  uint32_t* value);
static hal_adc_error_t stm32_adc_start_conversion_async(
    hal_adc_context_t* ctx, hal_adc_instance_t instance);
static hal_adc_error_t stm32_adc_stop_conversion(hal_adc_context_t* ctx,
                                                 hal_adc_instance_t instance);
static hal_adc_error_t stm32_adc_start_dma(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_dma_config_t* dma_config);
static hal_adc_error_t stm32_adc_stop_dma(hal_adc_context_t* ctx,
                                          hal_adc_instance_t instance);
static hal_adc_error_t stm32_adc_get_value(hal_adc_context_t* ctx,
                                           hal_adc_instance_t instance,
                                           uint32_t* value);
static hal_adc_error_t stm32_adc_calibrate(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_calibration_config_t* calibration_config);
static hal_adc_error_t stm32_adc_set_resolution(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_resolution_t resolution);
static hal_adc_error_t stm32_adc_get_resolution(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_resolution_t* resolution);
static hal_adc_error_t stm32_adc_register_callback(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_conv_callback_t callback, void* user_data);
static hal_adc_error_t stm32_adc_register_dma_callback(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_dma_callback_t callback, void* user_data);
static void stm32_adc_irq_handler(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance);

static void stm32_adc_enable_clock(hal_adc_instance_t instance);
static uint32_t stm32_adc_get_resolution_hal(hal_adc_resolution_t resolution);
static uint32_t stm32_adc_get_dataalign_hal(hal_adc_dataalign_t data_align);
static uint32_t stm32_adc_get_sampletime_hal(hal_adc_sampletime_t sample_time);
static uint32_t stm32_adc_get_channel_hal(hal_adc_channel_t channel);

/* ADC操作函数结构体 */
static const hal_adc_ops_t stm32_adc_ops = {
    .init = stm32_adc_init,
    .deinit = stm32_adc_deinit,
    .config_channel = stm32_adc_config_channel,
    .start_conversion = stm32_adc_start_conversion,
    .start_conversion_async = stm32_adc_start_conversion_async,
    .stop_conversion = stm32_adc_stop_conversion,
    .start_dma = stm32_adc_start_dma,
    .stop_dma = stm32_adc_stop_dma,
    .get_value = stm32_adc_get_value,
    .calibrate = stm32_adc_calibrate,
    .set_resolution = stm32_adc_set_resolution,
    .get_resolution = stm32_adc_get_resolution,
    .register_callback = stm32_adc_register_callback,
    .register_dma_callback = stm32_adc_register_dma_callback,
    .irq_handler = stm32_adc_irq_handler,
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  初始化ADC上下文并设置操作函数
 * @param  ctx: ADC上下文指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
hal_adc_error_t stm32_adc_init_context(hal_adc_context_t* ctx) {
  if (ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ctx->ops = &stm32_adc_ops;
  ctx->initialized = 0;
  ctx->conv_callback = NULL;
  ctx->dma_callback = NULL;
  ctx->user_data = NULL;
  ctx->busy = 0;

  return HAL_ADC_OK;
}

/**
 * @brief  注册ADC句柄
 * @param  instance: ADC实例
 * @param  hadc: ADC句柄指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
hal_adc_error_t stm32_adc_register_handle(hal_adc_instance_t instance,
                                          ADC_HandleTypeDef* hadc) {
  if (instance >= HAL_ADC_INSTANCE_LEN || hadc == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  adc_handle_map[instance] = hadc;

  return HAL_ADC_OK;
}

/**
 * @brief  注册DMA句柄
 * @param  instance: ADC实例
 * @param  hdma: DMA句柄指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
hal_adc_error_t stm32_adc_register_dma_handle(hal_adc_instance_t instance,
                                              DMA_HandleTypeDef* hdma) {
  if (instance >= HAL_ADC_INSTANCE_LEN || hdma == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  adc_dma_handle_map[instance] = hdma;

  return HAL_ADC_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  使能ADC时钟
 * @param  instance: ADC实例索引
 */
static void stm32_adc_enable_clock(hal_adc_instance_t instance) {
  (void)instance;
  __HAL_RCC_ADC12_CLK_ENABLE();
}

/**
 * @brief  将HAL分辨率转换为STM32 HAL分辨率
 * @param  resolution: HAL分辨率
 * @return STM32 HAL分辨率值
 */
static uint32_t stm32_adc_get_resolution_hal(hal_adc_resolution_t resolution) {
  switch (resolution) {
    case HAL_ADC_RESOLUTION_12B:
      return ADC_RESOLUTION_12B;
    case HAL_ADC_RESOLUTION_10B:
      return ADC_RESOLUTION_10B;
    case HAL_ADC_RESOLUTION_8B:
      return ADC_RESOLUTION_8B;
    case HAL_ADC_RESOLUTION_6B:
      return ADC_RESOLUTION_6B;
    default:
      return ADC_RESOLUTION_12B;
  }
}

/**
 * @brief  将HAL数据对齐转换为STM32 HAL数据对齐
 * @param  data_align: HAL数据对齐
 * @return STM32 HAL数据对齐值
 */
static uint32_t stm32_adc_get_dataalign_hal(hal_adc_dataalign_t data_align) {
  switch (data_align) {
    case HAL_ADC_DATAALIGN_RIGHT:
      return ADC_DATAALIGN_RIGHT;
    case HAL_ADC_DATAALIGN_LEFT:
      return ADC_DATAALIGN_LEFT;
    default:
      return ADC_DATAALIGN_RIGHT;
  }
}

/**
 * @brief  将HAL采样时间转换为STM32 HAL采样时间
 * @param  sample_time: HAL采样时间
 * @return STM32 HAL采样时间值
 */
static uint32_t stm32_adc_get_sampletime_hal(hal_adc_sampletime_t sample_time) {
  switch (sample_time) {
    case HAL_ADC_SAMPLETIME_2CYCLES_5:
      return ADC_SAMPLETIME_2CYCLES_5;
    case HAL_ADC_SAMPLETIME_6CYCLES_5:
      return ADC_SAMPLETIME_6CYCLES_5;
    case HAL_ADC_SAMPLETIME_12CYCLES_5:
      return ADC_SAMPLETIME_12CYCLES_5;
    case HAL_ADC_SAMPLETIME_24CYCLES_5:
      return ADC_SAMPLETIME_24CYCLES_5;
    case HAL_ADC_SAMPLETIME_47CYCLES_5:
      return ADC_SAMPLETIME_47CYCLES_5;
    case HAL_ADC_SAMPLETIME_92CYCLES_5:
      return ADC_SAMPLETIME_92CYCLES_5;
    case HAL_ADC_SAMPLETIME_247CYCLES_5:
      return ADC_SAMPLETIME_247CYCLES_5;
    case HAL_ADC_SAMPLETIME_640CYCLES_5:
      return ADC_SAMPLETIME_640CYCLES_5;
    case HAL_ADC_SAMPLETIME_3CYCLES_5:
      return ADC_SAMPLETIME_3CYCLES_5;
    default:
      return ADC_SAMPLETIME_2CYCLES_5;
  }
}

/**
 * @brief  将HAL通道转换为STM32 HAL通道
 * @param  channel: HAL通道
 * @return STM32 HAL通道值
 */
static uint32_t stm32_adc_get_channel_hal(hal_adc_channel_t channel) {
  switch (channel) {
    case HAL_ADC_CHANNEL_0:
      return ADC_CHANNEL_0;
    case HAL_ADC_CHANNEL_1:
      return ADC_CHANNEL_1;
    case HAL_ADC_CHANNEL_2:
      return ADC_CHANNEL_2;
    case HAL_ADC_CHANNEL_3:
      return ADC_CHANNEL_3;
    case HAL_ADC_CHANNEL_4:
      return ADC_CHANNEL_4;
    case HAL_ADC_CHANNEL_5:
      return ADC_CHANNEL_5;
    case HAL_ADC_CHANNEL_6:
      return ADC_CHANNEL_6;
    case HAL_ADC_CHANNEL_7:
      return ADC_CHANNEL_7;
    case HAL_ADC_CHANNEL_8:
      return ADC_CHANNEL_8;
    case HAL_ADC_CHANNEL_9:
      return ADC_CHANNEL_9;
    case HAL_ADC_CHANNEL_10:
      return ADC_CHANNEL_10;
    case HAL_ADC_CHANNEL_11:
      return ADC_CHANNEL_11;
    case HAL_ADC_CHANNEL_12:
      return ADC_CHANNEL_12;
    case HAL_ADC_CHANNEL_13:
      return ADC_CHANNEL_13;
    case HAL_ADC_CHANNEL_14:
      return ADC_CHANNEL_14;
    case HAL_ADC_CHANNEL_15:
      return ADC_CHANNEL_15;
    case HAL_ADC_CHANNEL_16:
      return ADC_CHANNEL_16;
    case HAL_ADC_CHANNEL_17:
      return ADC_CHANNEL_17;
    case HAL_ADC_CHANNEL_18:
      return ADC_CHANNEL_18;
    default:
      return ADC_CHANNEL_0;
  }
}

/**
 * @brief  初始化ADC
 * @param  ctx: ADC上下文指针
 * @param  config: ADC配置结构体指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_init(hal_adc_context_t* ctx,
                                      const hal_adc_config_t* config) {
  if (config == NULL || config->instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[config->instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  stm32_adc_enable_clock(config->instance);

  hadc->Instance = (config->instance == HAL_ADC_INSTANCE_1) ? ADC1 : ADC2;

  hadc->Init.Resolution = stm32_adc_get_resolution_hal(config->resolution);
  hadc->Init.DataAlign = stm32_adc_get_dataalign_hal(config->data_align);
  hadc->Init.ScanConvMode =
      (config->scan_mode == HAL_ADC_SCAN_MODE_ENABLE) ? ENABLE : DISABLE;
  hadc->Init.ContinuousConvMode =
      (config->continuous == HAL_ADC_CONTINUOUS_ENABLE) ? ENABLE : DISABLE;
  hadc->Init.DiscontinuousConvMode = DISABLE;
  hadc->Init.NbrOfDiscConversion = 0;
  hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc->Init.NbrOfConversion = config->nbr_of_conversion;
  hadc->Init.DMAContinuousRequests =
      (config->dma_mode == HAL_ADC_DMA_ENABLE) ? ENABLE : DISABLE;

  if (HAL_ADC_Init(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  adc_context_map[config->instance] = ctx;

  return HAL_ADC_OK;
}

/**
 * @brief  反初始化 ADC
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_deinit(hal_adc_context_t* ctx,
                                        hal_adc_instance_t instance) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  if (HAL_ADC_DeInit(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  adc_context_map[instance] = NULL;

  return HAL_ADC_OK;
}

/**
 * @brief  配置ADC通道
 * @param  ctx: ADC上下文指针
 * @param  channel_config: 通道配置结构体指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_config_channel(
    hal_adc_context_t* ctx, const hal_adc_channel_config_t* channel_config) {
  if (channel_config == NULL || ctx == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[ctx->config.instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  ADC_ChannelConfTypeDef sConfig = {0};

  sConfig.Channel = stm32_adc_get_channel_hal(channel_config->channel);
  sConfig.Rank = channel_config->rank + 1;
  sConfig.SamplingTime =
      stm32_adc_get_sampletime_hal(channel_config->sample_time);

  if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  启动ADC转换（阻塞）
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  value: 输出参数，存储转换结果
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_start_conversion(hal_adc_context_t* ctx,
                                                  hal_adc_instance_t instance,
                                                  uint32_t* value) {
  if (instance >= HAL_ADC_INSTANCE_LEN || value == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  if (HAL_ADC_Start(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  if (HAL_ADC_PollForConversion(hadc, ctx->config.timeout) != HAL_OK) {
    return HAL_ADC_ERROR_TIMEOUT;
  }

  *value = HAL_ADC_GetValue(hadc);

  if (HAL_ADC_Stop(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  启动ADC转换（异步）
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_start_conversion_async(
    hal_adc_context_t* ctx, hal_adc_instance_t instance) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  if (HAL_ADC_Start_IT(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  停止ADC转换
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_stop_conversion(hal_adc_context_t* ctx,
                                                 hal_adc_instance_t instance) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  if (HAL_ADC_Stop_IT(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  启动ADC DMA转换
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  dma_config: DMA配置结构体指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_start_dma(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_dma_config_t* dma_config) {
  if (instance >= HAL_ADC_INSTANCE_LEN || dma_config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  uint32_t length = dma_config->buffer_length;

  if (HAL_ADC_Start_DMA(hadc, dma_config->buffer, length) != HAL_OK) {
    return HAL_ADC_ERROR_DMA;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  停止ADC DMA转换
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_stop_dma(hal_adc_context_t* ctx,
                                          hal_adc_instance_t instance) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  if (HAL_ADC_Stop_DMA(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_DMA;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  获取转换结果
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  value: 输出参数，存储转换结果
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_get_value(hal_adc_context_t* ctx,
                                           hal_adc_instance_t instance,
                                           uint32_t* value) {
  if (instance >= HAL_ADC_INSTANCE_LEN || value == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  *value = HAL_ADC_GetValue(hadc);

  return HAL_ADC_OK;
}

/**
 * @brief  校准ADC
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  calibration_config: 校准配置结构体指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_calibrate(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    const hal_adc_calibration_config_t* calibration_config) {
  if (instance >= HAL_ADC_INSTANCE_LEN || calibration_config == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  uint32_t calibration_mode = ADC_SINGLE_ENDED;
  if (calibration_config != NULL && calibration_config->differential) {
    calibration_mode = ADC_DIFFERENTIAL_ENDED;
  }

  if (HAL_ADCEx_Calibration_Start(hadc, calibration_mode) != HAL_OK) {
    return HAL_ADC_ERROR_CALIBRATION;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  设置分辨率
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  resolution: 分辨率
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_set_resolution(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_resolution_t resolution) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  hadc->Init.Resolution = stm32_adc_get_resolution_hal(resolution);

  if (HAL_ADC_Init(hadc) != HAL_OK) {
    return HAL_ADC_ERROR_HARDWARE;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  获取分辨率
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  resolution: 输出参数，返回分辨率
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_get_resolution(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_resolution_t* resolution) {
  if (instance >= HAL_ADC_INSTANCE_LEN || resolution == NULL) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return HAL_ADC_ERROR_UNINITIALIZED;
  }

  (void)ctx;

  switch (hadc->Init.Resolution) {
    case ADC_RESOLUTION_12B:
      *resolution = HAL_ADC_RESOLUTION_12B;
      break;
    case ADC_RESOLUTION_10B:
      *resolution = HAL_ADC_RESOLUTION_10B;
      break;
    case ADC_RESOLUTION_8B:
      *resolution = HAL_ADC_RESOLUTION_8B;
      break;
    case ADC_RESOLUTION_6B:
      *resolution = HAL_ADC_RESOLUTION_6B;
      break;
    default:
      *resolution = HAL_ADC_RESOLUTION_12B;
      break;
  }

  return HAL_ADC_OK;
}

/**
 * @brief  注册转换完成回调函数
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  callback: 回调函数指针
 * @param  user_data: 用户数据指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_register_callback(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_conv_callback_t callback, void* user_data) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  (void)callback;
  (void)user_data;

  return HAL_ADC_OK;
}

/**
 * @brief  注册DMA完成回调函数
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 * @param  callback: 回调函数指针
 * @param  user_data: 用户数据指针
 * @return HAL_ADC_OK 成功，其他值为错误码
 */
static hal_adc_error_t stm32_adc_register_dma_callback(
    hal_adc_context_t* ctx, hal_adc_instance_t instance,
    hal_adc_dma_callback_t callback, void* user_data) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return HAL_ADC_ERROR_INVALID_PARAM;
  }

  (void)ctx;
  (void)callback;
  (void)user_data;

  return HAL_ADC_OK;
}

/**
 * @brief  ADC中断处理
 * @param  ctx: ADC上下文指针
 * @param  instance: ADC实例
 */
static void stm32_adc_irq_handler(hal_adc_context_t* ctx,
                                  hal_adc_instance_t instance) {
  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return;
  }

  ADC_HandleTypeDef* hadc = adc_handle_map[instance];
  if (hadc == NULL) {
    return;
  }

  (void)ctx;

  HAL_ADC_IRQHandler(hadc);
}

/**
 * @brief  ADC转换完成回调函数
 * @param  hadc: ADC句柄指针
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  hal_adc_instance_t instance = HAL_ADC_INSTANCE_LEN;

  for (uint8_t i = 0; i < HAL_ADC_INSTANCE_LEN; i++) {
    if (adc_handle_map[i] == hadc) {
      instance = (hal_adc_instance_t)i;
      break;
    }
  }

  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return;
  }

  hal_adc_context_t* ctx = adc_context_map[instance];
  if (ctx == NULL) {
    return;
  }

  ctx->busy = 0;

  if (ctx->conv_callback != NULL) {
    uint32_t value = HAL_ADC_GetValue(hadc);
    ctx->conv_callback(ctx, instance, value, ctx->user_data);
  }
}

/**
 * @brief  ADC DMA转换完成回调函数
 * @param  hadc: ADC句柄指针
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) { (void)hadc; }

/**
 * @brief  ADC错误回调函数
 * @param  hadc: ADC句柄指针
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef* hadc) {
  hal_adc_instance_t instance = HAL_ADC_INSTANCE_LEN;

  for (uint8_t i = 0; i < HAL_ADC_INSTANCE_LEN; i++) {
    if (adc_handle_map[i] == hadc) {
      instance = (hal_adc_instance_t)i;
      break;
    }
  }

  if (instance >= HAL_ADC_INSTANCE_LEN) {
    return;
  }

  hal_adc_context_t* ctx = adc_context_map[instance];
  if (ctx == NULL) {
    return;
  }

  ctx->busy = 0;
}
