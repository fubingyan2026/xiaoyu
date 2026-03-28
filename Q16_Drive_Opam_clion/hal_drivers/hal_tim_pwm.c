/**
 * @brief:  HAL TIM PWM驱动
 * @FilePath: hal_tim_pwm.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 21:15:01
 * @LastEditTime: 2025-09-23 11:13:20
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */
// hal_tim_pwm.c
#include "hal_tim_pwm.h"

static const hal_tim_pwm_ops_t *tim_pwm_ops = NULL;

extern void platform_tim_pwm_init(void);

void hal_tim_pwm_set_ops(const hal_tim_pwm_ops_t *ops) {
  tim_pwm_ops = ops;
}

bool hal_tim_pwm_init(const hal_tim_pwm_config_t *config) {
  static uint8_t init_flag = 1;
  if (init_flag) platform_tim_pwm_init();
  init_flag = 0;

  if (tim_pwm_ops != NULL && tim_pwm_ops->init != NULL) {
    return tim_pwm_ops->init(config);
  }
  return false;
}

bool hal_tim_pwm_gpio_alternate(const hal_tim_pwm_gpio_config_t *gpio_config) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->alternate != NULL) {
    return tim_pwm_ops->alternate(gpio_config);
  }
  return false;
}

bool hal_tim_pwm_start(uint8_t timer_instance, hal_tim_pwm_channel_e channel) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->start != NULL) {
    return tim_pwm_ops->start(timer_instance, channel);
  }
  return false;
}

bool hal_tim_pwm_stop(uint8_t timer_instance, hal_tim_pwm_channel_e channel) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->stop != NULL) {
    return tim_pwm_ops->stop(timer_instance, channel);
  }
  return false;
}

bool hal_tim_pwm_set_duty_cycle(uint8_t timer_instance,
                                hal_tim_pwm_channel_e channel,
                                uint32_t duty_cycle) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->set_duty_cycle != NULL) {
    return tim_pwm_ops->set_duty_cycle(timer_instance, channel, duty_cycle);
  }
  return false;
}

bool hal_tim_pwm_set_frequency(uint8_t timer_instance,
                               hal_tim_pwm_channel_e channel,
                               uint32_t frequency) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->set_frequency != NULL) {
    return tim_pwm_ops->set_frequency(timer_instance, channel, frequency);
  }
  return false;
}

uint32_t hal_tim_pwm_get_duty_cycle(uint8_t timer_instance,
                                    hal_tim_pwm_channel_e channel) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->get_duty_cycle != NULL) {
    return tim_pwm_ops->get_duty_cycle(timer_instance, channel);
  }
  return 0;
}

uint32_t hal_tim_pwm_get_frequency(uint8_t timer_instance,
                                   hal_tim_pwm_channel_e channel) {
  if (tim_pwm_ops != NULL && tim_pwm_ops->get_frequency != NULL) {
    return tim_pwm_ops->get_frequency(timer_instance, channel);
  }
  return 0;
}
