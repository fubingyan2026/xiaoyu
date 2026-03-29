//
// Created by fubingyan on 25-8-2.
//

#ifndef HALL_ADJUSTMENT_H
#define HALL_ADJUSTMENT_H
#include "stdint.h"

#define ADC_CH_NUM 2

typedef enum {
  ADJUST_NONE = 0,
  ADJUST_FILTER,
  ADJUST_ALIGN,
  ADJUST_ROTATION,
  ADJUST_PROCESS,
  ADJUST_DONE,
} hall_adjust_e;

typedef struct {
  int16_t adcAmplitudeBias[ADC_CH_NUM];
  int16_t adcAmplitudeMax[ADC_CH_NUM];
  uint16_t hall_adjust_flag;
} hall_save_param_t;

typedef struct {
  hall_adjust_e hall_adjust_state;
  uint32_t adcOriginalValue[ADC_CH_NUM];
  uint8_t motor_poles;
  float angle;
  float motorCurrentMax;
  float motorTargetCurrent;
  float motorTargetElectricalAngle;

  void (*hall_adc_DMA_Start)(void);

  void (*hall_adjust_init)(void);

  void (*hall_adjust_task)(void);

  uint16_t (*hall_adjust_get_angle)(void);
} hall_adjust_t;

extern hall_adjust_t hall_adjust;

#endif  // HALL_ADJUSTMENT_H
