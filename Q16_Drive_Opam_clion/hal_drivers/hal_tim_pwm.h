// hal_tim_pwm.h
#ifndef __HAL_TIM_PWM_H
#define __HAL_TIM_PWM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hal_gpio.h"
    /* Exported types ------------------------------------------------------------*/
    /**
     * @brief TIM PWM通道定义
     */
    typedef enum
    {
        HAL_TIM_PWM_CHANNEL_1 = 0, /**< PWM通道1 */
        HAL_TIM_PWM_CHANNEL_2,     /**< PWM通道2 */
        HAL_TIM_PWM_CHANNEL_3,     /**< PWM通道3 */
        HAL_TIM_PWM_CHANNEL_4,     /**< PWM通道4 */
        HAL_TIM_PWM_CHANNEL_LEN    /**< 通道数量 */
    } hal_tim_pwm_channel_e;

    /**
     * @brief TIM PWM极性定义
     */
    typedef enum
    {
        HAL_TIM_PWM_POLARITY_HIGH = 0, /**< 高电平有效 */
        HAL_TIM_PWM_POLARITY_LOW       /**< 低电平有效 */
    } hal_tim_pwm_polarity_e;

    /**
     * @brief GPIO引脚复用配置
     */
    typedef struct
    {
        gpio_port_e port;  /**< GPIO端口 */
        uint8_t pin;       /**< GPIO引脚 */
        uint8_t alternate; /**< 复用功能 */
    } hal_tim_pwm_gpio_config_t;

    /**
     * @brief TIM PWM配置结构体
     */
    typedef struct
    {
        uint32_t timer_frequency;        /**< 定时器时钟频率(Hz) */
        uint32_t pwm_frequency;          /**< PWM频率(Hz) */
        uint32_t duty_cycle;             /**< 占空比(0-10000对应0%-100%) */
        hal_tim_pwm_polarity_e polarity; /**< PWM极性 */
        uint8_t timer_instance;          /**< 定时器实例(TIM1-TIM8等) */
        hal_tim_pwm_channel_e channel;   /**< PWM通道 */
        hal_tim_pwm_gpio_config_t gpio;  /**< GPIO引脚配置 */
    } hal_tim_pwm_config_t;

    /**
     * @brief TIM PWM操作函数指针结构体
     */
    typedef struct
    {
        bool (*init)(const hal_tim_pwm_config_t *config);
        bool (*alternate)(const hal_tim_pwm_gpio_config_t *gpio_config);
        bool (*start)(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
        bool (*stop)(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
        bool (*set_duty_cycle)(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t duty_cycle);
        bool (*set_frequency)(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t frequency);
        uint32_t (*get_duty_cycle)(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
        uint32_t (*get_frequency)(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
    } hal_tim_pwm_ops_t;

    /* Exported functions prototypes */
    void hal_tim_pwm_set_ops(const hal_tim_pwm_ops_t *ops);
    bool hal_tim_pwm_init(const hal_tim_pwm_config_t *config);
    bool hal_tim_pwm_gpio_alternate(const hal_tim_pwm_gpio_config_t *gpio_config);
   
    bool hal_tim_pwm_start(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
    bool hal_tim_pwm_stop(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
    bool hal_tim_pwm_set_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t duty_cycle);
    bool hal_tim_pwm_set_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel, uint32_t frequency);
    uint32_t hal_tim_pwm_get_duty_cycle(uint8_t timer_instance, hal_tim_pwm_channel_e channel);
    uint32_t hal_tim_pwm_get_frequency(uint8_t timer_instance, hal_tim_pwm_channel_e channel);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_TIM_PWM_H */