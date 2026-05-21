/**
 * @file    angle_sensor_linear_hall.h
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   线性霍尔传感器驱动（PLL）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef ANGLE_SENSOR_LINEAR_HALL_H
#define ANGLE_SENSOR_LINEAR_HALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor.h"

/* Exported functions prototypes ---------------------------------------------*/

angle_sensor_error_t angle_sensor_linear_hall_init_context(
    angle_sensor_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ANGLE_SENSOR_LINEAR_HALL_H */
