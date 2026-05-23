/**
 * @file    angle_sensor_mt6816.h
 * @author  fubingyan
 * @version V2.0.0
 * @date    2026-05-22
 * @brief   MT6816 磁编码器传感器驱动（SPI）
 * @attention
 *
 * Copyright (c) 2026 by fubingyan, All Rights Reserved.
 */

#ifndef ANGLE_SENSOR_MT6816_H
#define ANGLE_SENSOR_MT6816_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "angle_sensor.h"

/* Exported functions prototypes ---------------------------------------------*/

angle_sensor_error_t angle_sensor_mt6816_init_context(
    angle_sensor_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* ANGLE_SENSOR_MT6816_H */
