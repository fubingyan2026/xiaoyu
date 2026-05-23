//
// Created by fubingyan on 24-12-26.
//

#ifndef __DEVICE_MT6701_H
#define __DEVICE_MT6701_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

typedef struct {
    uint16_t mt6701_raw_angle : 14;

    struct {
        uint8_t magnetic_state : 2;
        uint8_t Push_button_state : 1;
        uint8_t Loss_of_Track : 1;
    } Status;

    uint8_t CRC_6Bit : 6;
} device_mt6701_raw_t;

/* Exported functions prototypes ---------------------------------------------*/

uint16_t device_mt6701_get_angle_data(void);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_MT6701_H */
