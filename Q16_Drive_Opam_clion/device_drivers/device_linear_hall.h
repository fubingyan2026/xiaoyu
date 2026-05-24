//
// Created by fubingyan on 25-8-2.
//

#ifndef __DEVICE_LINEAR_HALL_H
#define __DEVICE_LINEAR_HALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "pll.h"

/* Exported defines ----------------------------------------------------------*/
#define DEVICE_LINEAR_HALL_ADC_CH_NUM 2

/* Exported types ------------------------------------------------------------*/

typedef struct {
    int16_t adcAmplitudeBias[DEVICE_LINEAR_HALL_ADC_CH_NUM];
    int16_t adcAmplitudeMax[DEVICE_LINEAR_HALL_ADC_CH_NUM];
    uint16_t calibrated_flag;
} hall_save_param_t;

/* Exported variables --------------------------------------------------------*/

extern pll_context_t pll_ctx;
extern hall_save_param_t hall_save_param;

/* Exported functions prototypes ---------------------------------------------*/

void device_linear_hall_init(void);

void device_linear_hall_start_dma(void);

const uint32_t* device_linear_hall_get_raw_buffer(void);

float device_linear_hall_get_angle_rad(void);

float device_linear_hall_get_velocity_rads(void);

uint16_t device_linear_hall_get_angle(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_LINEAR_HALL_H */
