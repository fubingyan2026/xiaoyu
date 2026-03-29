/**
 * @brief:  STM32 HAL TIM PWM驱动 - 平台层
 * @FilePath: stm32_hal_tim_pwm.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 21:15:44
 * @LastEditTime: 2025-09-23 12:39:01
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
#include "hal_gpio.h"
#include "hal_tim_pwm.h"
#include "stm32g4xx_hal.h"

/* 定时器句柄数组 */
extern TIM_HandleTypeDef htim17;

static TIM_HandleTypeDef *timer_handles[] = {NULL,                      //
                                             NULL,   NULL, NULL, NULL,  //
                                             NULL,   NULL, NULL, NULL,  //
                                             NULL,   NULL, NULL, NULL,  //
                                             NULL,   NULL, NULL, NULL,  //
                                             &htim17};

/* 定时器通道映射 */
static uint32_t channel_map[HAL_TIM_PWM_CHANNEL_LEN] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4};

/* 定时器实例映射 */
static TIM_TypeDef *timer_instance_map[] = {NULL,                      //
                                            TIM1, TIM2, TIM3,  TIM4,   //
                                            NULL, TIM6, TIM7,  TIM8,   //
                                            NULL, NULL, NULL,  NULL,   //
                                            NULL, NULL, TIM15, TIM16,  //
                                            TIM17};
/* GPIO端口映射 */
static GPIO_TypeDef *gpio_port_map[] = {GPIOA, GPIOB, GPIOC, GPIOD,
                                        GPIOE, GPIOF, GPIOG};

static const hal_tim_pwm_ops_t stm32_tim_pwm_ops;

/* 私有函数声明 */
static hal_tim_pwm_error_t stm32_tim_pwm_init(hal_tim_pwm_context_t *ctx,
                                               const hal_tim_pwm_config_t *config);
static hal_tim_pwm_error_t stm32_tim_pwm_deinit(hal_tim_pwm_context_t *ctx,
                                                 hal_tim_pwm_instance_t instance,
                                                 hal_tim_pwm_channel_t channel);
static hal_tim_pwm_error_t stm32_tim_pwm_gpio_alternate(hal_tim_pwm_context_t *ctx,
                                                         const hal_tim_pwm_gpio_config_t *gpio_config);
static hal_tim_pwm_error_t stm32_tim_pwm_start(hal_tim_pwm_context_t *ctx,
                                                hal_tim_pwm_instance_t instance,
                                                hal_tim_pwm_channel_t channel);
static hal_tim_pwm_error_t stm32_tim_pwm_stop(hal_tim_pwm_context_t *ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel);
static hal_tim_pwm_error_t stm32_tim_pwm_set_duty_cycle(hal_tim_pwm_context_t *ctx,
                                                         hal_tim_pwm_instance_t instance,
                                                         hal_tim_pwm_channel_t channel,
                                                         uint32_t duty_cycle);
static hal_tim_pwm_error_t stm32_tim_pwm_set_frequency(hal_tim_pwm_context_t *ctx,
                                                        hal_tim_pwm_instance_t instance,
                                                        hal_tim_pwm_channel_t channel,
                                                        uint32_t frequency);
static hal_tim_pwm_error_t stm32_tim_pwm_get_duty_cycle(hal_tim_pwm_context_t *ctx,
                                                         hal_tim_pwm_instance_t instance,
                                                         hal_tim_pwm_channel_t channel,
                                                         uint32_t *duty_cycle);
static hal_tim_pwm_error_t stm32_tim_pwm_get_frequency(hal_tim_pwm_context_t *ctx,
                                                        hal_tim_pwm_instance_t instance,
                                                        hal_tim_pwm_channel_t channel,
                                                        uint32_t *frequency);
static hal_tim_pwm_error_t stm32_tim_pwm_set_polarity(hal_tim_pwm_context_t *ctx,
                                                       hal_tim_pwm_instance_t instance,
                                                       hal_tim_pwm_channel_t channel,
                                                       hal_tim_pwm_polarity_t polarity);
static hal_tim_pwm_error_t stm32_tim_pwm_get_polarity(hal_tim_pwm_context_t *ctx,
                                                       hal_tim_pwm_instance_t instance,
                                                       hal_tim_pwm_channel_t channel,
                                                       hal_tim_pwm_polarity_t *polarity);

static void enable_timer_clock(hal_tim_pwm_instance_t timer_instance);
static void enable_gpio_clock(uint8_t port);
static TIM_HandleTypeDef *get_timer_handle(hal_tim_pwm_instance_t timer_instance);

/* TIM PWM操作函数结构体 */
static const hal_tim_pwm_ops_t stm32_tim_pwm_ops = {
    .init = stm32_tim_pwm_init,
    .deinit = stm32_tim_pwm_deinit,
    .gpio_alternate = stm32_tim_pwm_gpio_alternate,
    .start = stm32_tim_pwm_start,
    .stop = stm32_tim_pwm_stop,
    .set_duty_cycle = stm32_tim_pwm_set_duty_cycle,
    .set_frequency = stm32_tim_pwm_set_frequency,
    .get_duty_cycle = stm32_tim_pwm_get_duty_cycle,
    .get_frequency = stm32_tim_pwm_get_frequency,
    .set_polarity = stm32_tim_pwm_set_polarity,
    .get_polarity = stm32_tim_pwm_get_polarity};

/**
 * @brief  STM32 平台 TIM PWM 上下文初始化函数
 * @param ctx TIM PWM 上下文指针
 * @return 操作结果错误码
 */
hal_tim_pwm_error_t stm32_tim_pwm_init_context(hal_tim_pwm_context_t *ctx)
{
  if (ctx == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  ctx->ops = &stm32_tim_pwm_ops;
  ctx->initialized = 0;

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  获取定时器句柄
 * @param timer_instance 定时器实例
 * @return 定时器句柄指针，失败返回NULL
 */
static TIM_HandleTypeDef *get_timer_handle(hal_tim_pwm_instance_t timer_instance)
{
  if (timer_instance >= 1  //
      && timer_instance <=
             (sizeof(timer_handles) / sizeof(timer_handles[0]) - 1))  //
  {
    return timer_handles[timer_instance];
  }
  return NULL;
}

/**
 * @brief  启用定时器时钟
 * @param timer_instance 定时器实例
 */
static void enable_timer_clock(hal_tim_pwm_instance_t timer_instance)
{
  switch (timer_instance) {
    case HAL_TIM_PWM_INSTANCE_1:
      __HAL_RCC_TIM1_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_2:
      __HAL_RCC_TIM2_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_3:
      __HAL_RCC_TIM3_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_4:
      __HAL_RCC_TIM4_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_6:
      __HAL_RCC_TIM6_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_7:
      __HAL_RCC_TIM7_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_8:
      __HAL_RCC_TIM8_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_15:
      __HAL_RCC_TIM15_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_16:
      __HAL_RCC_TIM16_CLK_ENABLE();
      break;
    case HAL_TIM_PWM_INSTANCE_17:
      __HAL_RCC_TIM17_CLK_ENABLE();
      break;
    default:
      break;
  }
}

/**
 * @brief  启用GPIO时钟
 * @param port GPIO端口号
 */
static void enable_gpio_clock(uint8_t port)
{
  switch (port) {
    case 0:
      __HAL_RCC_GPIOA_CLK_ENABLE();
      break;
    case 1:
      __HAL_RCC_GPIOB_CLK_ENABLE();
      break;
    case 2:
      __HAL_RCC_GPIOC_CLK_ENABLE();
      break;
    case 3:
      __HAL_RCC_GPIOD_CLK_ENABLE();
      break;
    case 4:
      __HAL_RCC_GPIOE_CLK_ENABLE();
      break;
    case 5:
      __HAL_RCC_GPIOF_CLK_ENABLE();
      break;
    case 6:
      __HAL_RCC_GPIOG_CLK_ENABLE();
      break;
    default:
      break;
  }
}

/**
 * @brief  配置GPIO复用功能
 * @param ctx TIM PWM 上下文指针
 * @param gpio_config GPIO配置结构体指针
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_gpio_alternate(hal_tim_pwm_context_t *ctx,
                                                         const hal_tim_pwm_gpio_config_t *gpio_config)
{
  (void)ctx;

  if (gpio_config == NULL || gpio_config->port >= HAL_GPIO_PORT_LEN) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 启用GPIO时钟 */
  enable_gpio_clock(gpio_config->port);

  /* 配置GPIO引脚 */
  GPIO_InitStruct.Pin = 1 << gpio_config->pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = gpio_config->alternate;
  HAL_GPIO_DeInit(gpio_port_map[gpio_config->port], 1 << gpio_config->pin);
  HAL_GPIO_Init(gpio_port_map[gpio_config->port], &GPIO_InitStruct);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param config TIM PWM 配置结构体指针
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_init(hal_tim_pwm_context_t *ctx,
                                               const hal_tim_pwm_config_t *config)
{
  (void)ctx;

  if (config == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  TIM_HandleTypeDef *htim = get_timer_handle(config->timer_instance);

  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  /* 配置GPIO复用功能 */
  hal_tim_pwm_error_t ret = stm32_tim_pwm_gpio_alternate(ctx, &config->gpio);
  if (ret != HAL_TIM_PWM_OK) {
    return ret;
  }

  /* 启用定时器时钟 */
  enable_timer_clock(config->timer_instance);

  /* 配置定时器基础参数 */
  htim->Instance = timer_instance_map[config->timer_instance];
  htim->Init.Prescaler = 0;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  /* 计算预分频器和重载值以获得所需PWM频率 */
  uint32_t timer_clock = config->timer_frequency;
  uint32_t prescaler = (timer_clock / (config->pwm_frequency * 10000)) - 1;
  uint32_t period = 10000 - 1;  // 10000对应100%占空比分辨率

  htim->Init.Prescaler = prescaler;
  htim->Init.Period = period;

  HAL_TIM_Base_DeInit(htim);
  if (HAL_TIM_Base_Init(htim) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  HAL_TIM_PWM_DeInit(htim);
  /* 初始化定时器 */
  if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  /* 配置PWM通道 */
  TIM_OC_InitTypeDef sConfigOC;
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = (config->duty_cycle * period) / 10000;  // 计算脉冲宽度
  sConfigOC.OCPolarity = (config->polarity == HAL_TIM_PWM_POLARITY_HIGH)
                             ? TIM_OCPOLARITY_HIGH
                             : TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC,
                                channel_map[config->channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

  if (HAL_TIMEx_ConfigBreakDeadTime(htim, &sBreakDeadTimeConfig) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  反初始化 TIM PWM
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_deinit(hal_tim_pwm_context_t *ctx,
                                                 hal_tim_pwm_instance_t instance,
                                                 hal_tim_pwm_channel_t channel)
{
  (void)ctx;
  (void)channel;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  HAL_TIM_PWM_DeInit(htim);
  HAL_TIM_Base_DeInit(htim);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  启动PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_start(hal_tim_pwm_context_t *ctx,
                                                hal_tim_pwm_instance_t instance,
                                                hal_tim_pwm_channel_t channel)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (HAL_TIM_PWM_Start(htim, channel_map[channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  停止PWM输出
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_stop(hal_tim_pwm_context_t *ctx,
                                               hal_tim_pwm_instance_t instance,
                                               hal_tim_pwm_channel_t channel)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (HAL_TIM_PWM_Stop(htim, channel_map[channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  设置PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_set_duty_cycle(hal_tim_pwm_context_t *ctx,
                                                         hal_tim_pwm_instance_t instance,
                                                         hal_tim_pwm_channel_t channel,
                                                         uint32_t duty_cycle)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  if (duty_cycle > 10000) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  /* 计算新的脉冲宽度 */
  uint32_t pulse = (duty_cycle * htim->Init.Period) / 10000;

  /* 更新占空比 */
  switch (channel_map[channel]) {
    case TIM_CHANNEL_1:
      htim->Instance->CCR1 = pulse;
      break;
    case TIM_CHANNEL_2:
      htim->Instance->CCR2 = pulse;
      break;
    case TIM_CHANNEL_3:
      htim->Instance->CCR3 = pulse;
      break;
    case TIM_CHANNEL_4:
      htim->Instance->CCR4 = pulse;
      break;
    default:
      return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  设置PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency PWM频率(Hz)
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_set_frequency(hal_tim_pwm_context_t *ctx,
                                                        hal_tim_pwm_instance_t instance,
                                                        hal_tim_pwm_channel_t channel,
                                                        uint32_t frequency)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL || frequency == 0) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  /* 停止PWM输出 */
  if (HAL_TIM_PWM_Stop(htim, channel_map[channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  /* 重新计算预分频器和重载值 */
  uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();  // 获取定时器时钟频率
  if (instance == HAL_TIM_PWM_INSTANCE_1 || instance == HAL_TIM_PWM_INSTANCE_8) {
    timer_clock = HAL_RCC_GetPCLK2Freq();  // TIM1和TIM8在APB2总线上
  }

  uint32_t prescaler = (timer_clock / (frequency * 10000)) - 1;
  uint32_t period = 10000 - 1;

  htim->Init.Prescaler = prescaler;
  htim->Init.Period = period;

  /* 重新初始化定时器 */
  if (HAL_TIM_PWM_Init(htim) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  /* 重新配置PWM通道 */
  TIM_OC_InitTypeDef sConfigOC;
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = htim->Instance->CCR1;  // 保持当前占空比
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel_map[channel]) !=
      HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  /* 重新启动PWM输出 */
  if (HAL_TIM_PWM_Start(htim, channel_map[channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  获取PWM占空比
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param duty_cycle 输出参数，返回占空比(0-10000对应0%-100%
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_get_duty_cycle(hal_tim_pwm_context_t *ctx,
                                                         hal_tim_pwm_instance_t instance,
                                                         hal_tim_pwm_channel_t channel,
                                                         uint32_t *duty_cycle)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  uint32_t ccr_value = 0;

  /* 获取当前比较寄存器值 */
  switch (channel_map[channel]) {
    case TIM_CHANNEL_1:
      ccr_value = htim->Instance->CCR1;
      break;
    case TIM_CHANNEL_2:
      ccr_value = htim->Instance->CCR2;
      break;
    case TIM_CHANNEL_3:
      ccr_value = htim->Instance->CCR3;
      break;
    case TIM_CHANNEL_4:
      ccr_value = htim->Instance->CCR4;
      break;
    default:
      return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  /* 计算占空比百分比(0-10000对应0%-100%) */
  *duty_cycle = (ccr_value * 10000) / (htim->Init.Period + 1);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  获取PWM频率
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param frequency 输出参数，返回PWM频率(Hz)
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_get_frequency(hal_tim_pwm_context_t *ctx,
                                                        hal_tim_pwm_instance_t instance,
                                                        hal_tim_pwm_channel_t channel,
                                                        uint32_t *frequency)
{
  (void)ctx;
  (void)channel;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  /* 获取定时器时钟频率 */
  uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
  if (instance == HAL_TIM_PWM_INSTANCE_1 || instance == HAL_TIM_PWM_INSTANCE_8) {
    timer_clock = HAL_RCC_GetPCLK2Freq();
  }

  /* 计算PWM频率 */
  uint32_t prescaler = htim->Init.Prescaler + 1;
  uint32_t period = htim->Init.Period + 1;

  *frequency = timer_clock / (prescaler * period);

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  设置PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity PWM极性
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_set_polarity(hal_tim_pwm_context_t *ctx,
                                                       hal_tim_pwm_instance_t instance,
                                                       hal_tim_pwm_channel_t channel,
                                                       hal_tim_pwm_polarity_t polarity)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  TIM_OC_InitTypeDef sConfigOC;
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = htim->Instance->CCR1;
  sConfigOC.OCPolarity = (polarity == HAL_TIM_PWM_POLARITY_HIGH)
                           ? TIM_OCPOLARITY_HIGH
                           : TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel_map[channel]) != HAL_OK) {
    return HAL_TIM_PWM_ERROR_HARDWARE;
  }

  return HAL_TIM_PWM_OK;
}

/**
 * @brief  获取PWM极性
 * @param ctx TIM PWM 上下文指针
 * @param instance 定时器实例
 * @param channel PWM通道
 * @param polarity 输出参数，返回PWM极性
 * @return 操作结果错误码
 */
static hal_tim_pwm_error_t stm32_tim_pwm_get_polarity(hal_tim_pwm_context_t *ctx,
                                                       hal_tim_pwm_instance_t instance,
                                                       hal_tim_pwm_channel_t channel,
                                                       hal_tim_pwm_polarity_t *polarity)
{
  (void)ctx;

  TIM_HandleTypeDef *htim = get_timer_handle(instance);
  if (htim == NULL || polarity == NULL) {
    return HAL_TIM_PWM_ERROR_INVALID_PARAM;
  }

  uint32_t ccer_value = htim->Instance->CCER;

  switch (channel_map[channel]) {
    case TIM_CHANNEL_1:
      *polarity = ((ccer_value & TIM_CCER_CC1P) == 0)
                      ? HAL_TIM_PWM_POLARITY_HIGH
                      : HAL_TIM_PWM_POLARITY_LOW;
      break;
    case TIM_CHANNEL_2:
      *polarity = ((ccer_value & TIM_CCER_CC2P) == 0)
                      ? HAL_TIM_PWM_POLARITY_HIGH
                      : HAL_TIM_PWM_POLARITY_LOW;
      break;
    case TIM_CHANNEL_3:
      *polarity = ((ccer_value & TIM_CCER_CC3P) == 0)
                      ? HAL_TIM_PWM_POLARITY_HIGH
                      : HAL_TIM_PWM_POLARITY_LOW;
      break;
    case TIM_CHANNEL_4:
      *polarity = ((ccer_value & TIM_CCER_CC4P) == 0)
                      ? HAL_TIM_PWM_POLARITY_HIGH
                      : HAL_TIM_PWM_POLARITY_LOW;
      break;
    default:
      return HAL_TIM_PWM_ERROR_UNSUPPORTED;
  }

  return HAL_TIM_PWM_OK;
}
