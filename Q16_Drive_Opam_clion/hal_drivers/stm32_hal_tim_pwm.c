/**
 * @brief:  STM32 HAL TIM PWM驱动
 * @FilePath: stm32_hal_tim_pwm.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 21:15:44
 * @LastEditTime: 2025-09-23 12:39:01
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
// stm32_hal_tim_pwm.c
#include "hal_gpio.h"
#include "hal_tim_pwm.h"
#include "stm32g4xx_hal.h"

/* 定时器句柄数组 */
extern TIM_HandleTypeDef htim17;

static TIM_HandleTypeDef *timer_handles[] = {NULL,                     //
                                               NULL,   NULL, NULL, NULL, //
                                               NULL,   NULL, NULL, NULL, //
                                               NULL,   NULL, NULL, NULL, //
                                               NULL,   NULL, NULL, NULL, //
                                               &htim17};

/* 定时器通道映射 */
static uint32_t channel_map[HAL_TIM_PWM_CHANNEL_LEN] = {TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4};

/* 定时器实例映射 */
static TIM_TypeDef *timer_instance_map[] = {NULL,                     //
                                              TIM1, TIM2, TIM3,  TIM4,  //
                                              NULL, TIM6, TIM7,  TIM8,  //
                                              NULL, NULL, NULL,  NULL,  //
                                              NULL, NULL, TIM15, TIM16, //
                                              TIM17};
/* GPIO端口映射 */
static GPIO_TypeDef *gpio_port_map[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};

static const hal_tim_pwm_ops_t stm32_tim_pwm_ops;

/* Private function prototypes */
static bool stm32_tim_pwm_init(const hal_tim_pwm_config_t *config);
static bool stm32_tim_pwm_start(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
static bool stm32_tim_pwm_stop(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
static bool stm32_tim_pwm_set_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t duty_cycle);
static bool stm32_tim_pwm_set_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t frequency);
static uint32_t stm32_tim_pwm_get_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
static uint32_t stm32_tim_pwm_get_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
static bool configure_gpio_alternate(const hal_tim_pwm_gpio_config_t *gpio_config);

static void enable_timer_clock(uint8_t timer_instance);
static void enable_gpio_clock(uint8_t port);

/* TIM PWM操作函数结构体 */
static const hal_tim_pwm_ops_t stm32_tim_pwm_ops = {.init = stm32_tim_pwm_init,
                                                    .alternate = configure_gpio_alternate,
                                                    .start = stm32_tim_pwm_start,
                                                    .stop = stm32_tim_pwm_stop,
                                                    .set_duty_cycle = stm32_tim_pwm_set_duty_cycle,
                                                    .set_frequency = stm32_tim_pwm_set_frequency,
                                                    .get_duty_cycle = stm32_tim_pwm_get_duty_cycle,
                                                    .get_frequency = stm32_tim_pwm_get_frequency};

/**
 * @brief  初始化STM32平台TIM PWM
 */
void platform_tim_pwm_init(void)
{
    /* 设置TIM PWM操作函数 */
    hal_tim_pwm_set_ops(&stm32_tim_pwm_ops);
}

/**
 * @brief  获取定时器句柄
 */
static TIM_HandleTypeDef *get_timer_handle(uint8_t timer_instance)
{
    if (timer_instance >= 1                                                          //
        && timer_instance <= (sizeof(timer_handles) / sizeof(timer_handles[0]) - 1)) //
    {
        return timer_handles[timer_instance];
    }
    return NULL;
}

/**
 * @brief  启用定时器时钟
 */
static void enable_timer_clock(uint8_t timer_instance)
{
    switch (timer_instance)
    {
        case 1:
            __HAL_RCC_TIM1_CLK_ENABLE();
            break;
        case 2:
            __HAL_RCC_TIM2_CLK_ENABLE();
            break;
        case 3:
            __HAL_RCC_TIM3_CLK_ENABLE();
            break;
        case 4:
            __HAL_RCC_TIM4_CLK_ENABLE();
            break;
        case 6:
            __HAL_RCC_TIM6_CLK_ENABLE();
            break;
        case 7:
            __HAL_RCC_TIM7_CLK_ENABLE();
            break;
        case 8:
            __HAL_RCC_TIM8_CLK_ENABLE();
            break;
        case 15:
            __HAL_RCC_TIM15_CLK_ENABLE();
            break;
        case 16:
            __HAL_RCC_TIM16_CLK_ENABLE();
            break;
        case 17:
            __HAL_RCC_TIM17_CLK_ENABLE();
            break;
        default:
            break;
    }
}

/**
 * @brief  启用GPIO时钟
 */
static void enable_gpio_clock(uint8_t port)
{
    switch (port)
    {
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
 */
static bool configure_gpio_alternate(const hal_tim_pwm_gpio_config_t *gpio_config)
{
    if (gpio_config == NULL || gpio_config->port >= HAL_GPIO_PORT_LEN)
    {
        return false;
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
    return true;
}

static bool stm32_tim_pwm_init(const hal_tim_pwm_config_t *config)
{
    if (config == NULL                                                                     //
        || config->timer_instance < 1                                                      //
        || config->timer_instance > (sizeof(timer_handles) / sizeof(timer_handles[0]) - 1) //
        || config->channel >= HAL_TIM_PWM_CHANNEL_LEN)                                     //
    {
        return false;
    }

    TIM_HandleTypeDef *htim = get_timer_handle(config->timer_instance);

    if (htim == NULL)
    {
        return false;
    }
    /* 配置GPIO复用功能 */
    configure_gpio_alternate(&config->gpio);

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
    uint32_t period = 10000 - 1; // 10000对应100%占空比分辨率

    htim->Init.Prescaler = prescaler;
    htim->Init.Period = period;

    HAL_TIM_Base_DeInit(htim);
    if (HAL_TIM_Base_Init(htim) != HAL_OK)
    {
        return false;
    }

    HAL_TIM_PWM_DeInit(htim);
    /* 初始化定时器 */
    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
        return false;
    }

    /* 配置PWM通道 */
    TIM_OC_InitTypeDef sConfigOC;
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = (config->duty_cycle * period) / 10000; // 计算脉冲宽度
    sConfigOC.OCPolarity = (config->polarity == HAL_TIM_PWM_POLARITY_HIGH) ? TIM_OCPOLARITY_HIGH : TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel_map[config->channel]) != HAL_OK)
    {
        return false;
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
    if (HAL_TIMEx_ConfigBreakDeadTime(htim, &sBreakDeadTimeConfig) != HAL_OK)
    {
        return false;
    }

    return true;
}

static bool stm32_tim_pwm_start(uint8_t timer_instance, hal_tim_pwm_channel_e channel)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL)
    {
        return false;
    }

    if (HAL_TIM_PWM_Start(htim, channel_map[channel]) != HAL_OK)
    {
        return false;
    }

    return true;
}

static bool stm32_tim_pwm_stop(uint8_t timer_instance, hal_tim_pwm_channel_e channel)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL)
    {
        return false;
    }

    if (HAL_TIM_PWM_Stop(htim, channel_map[channel]) != HAL_OK)
    {
        return false;
    }

    return true;
}

static bool stm32_tim_pwm_set_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t duty_cycle)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL || duty_cycle > 10000)
    {
        return false;
    }

    /* 计算新的脉冲宽度 */
    uint32_t pulse = (duty_cycle * htim->Init.Period) / 10000;

    /* 更新占空比 */
    switch (channel_map[channel])
    {
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
        return false;
    }

    return true;
}

static bool stm32_tim_pwm_set_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t frequency)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL || frequency == 0)
    {
        return false;
    }

    /* 停止PWM输出 */
    if (HAL_TIM_PWM_Stop(htim, channel_map[channel]) != HAL_OK)
    {
        return false;
    }

    /* 重新计算预分频器和重载值 */
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq(); // 获取定时器时钟频率
    if (timer_instance == 1 || timer_instance == 8)
    {
        timer_clock = HAL_RCC_GetPCLK2Freq(); // TIM1和TIM8在APB2总线上
    }

    uint32_t prescaler = (timer_clock / (frequency * 10000)) - 1;
    uint32_t period = 10000 - 1;

    htim->Init.Prescaler = prescaler;
    htim->Init.Period = period;

    /* 重新初始化定时器 */
    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
        return false;
    }

    /* 重新配置PWM通道 */
    TIM_OC_InitTypeDef sConfigOC;
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = htim->Instance->CCR1; // 保持当前占空比
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel_map[channel]) != HAL_OK)
    {
        return false;
    }

    /* 重新启动PWM输出 */
    if (HAL_TIM_PWM_Start(htim, channel_map[channel]) != HAL_OK)
    {
        return false;
    }

    return true;
}

static uint32_t stm32_tim_pwm_get_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL)
    {
        return 0;
    }

    uint32_t ccr_value = 0;

    /* 获取当前比较寄存器值 */
    switch (channel_map[channel])
    {
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
        return 0;
    }

    /* 计算占空比百分比(0-10000对应0%-100%) */
    return (ccr_value * 10000) / (htim->Init.Period + 1);
}

static uint32_t stm32_tim_pwm_get_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel)
{
    TIM_HandleTypeDef *htim = get_timer_handle(timer_instance);
    if (htim == NULL)
    {
        return 0;
    }

    /* 获取定时器时钟频率 */
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if (timer_instance == 1 || timer_instance == 8)
    {
        timer_clock = HAL_RCC_GetPCLK2Freq();
    }

    /* 计算PWM频率 */
    uint32_t prescaler = htim->Init.Prescaler + 1;
    uint32_t period = htim->Init.Period + 1;

    return timer_clock / (prescaler * period);
}
