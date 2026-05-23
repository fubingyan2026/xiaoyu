//
// Created by fubingyan on 24-12-26.
//

#ifndef __DEVICE_MT6816_H
#define __DEVICE_MT6816_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Exported functions prototypes ---------------------------------------------*/

void device_mt6816_init(void);

uint16_t device_mt6816_get_angle_data(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_MT6816_H */
