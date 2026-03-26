#ifndef __MT6816_H__
#define __MT6816_H__
#include "spi.h"
#include "stdint.h"
#include <stdbool.h>

#define MT6816_SPI_CS_H() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET)
#define MT6816_SPI_CS_L() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET)
#define MT6816_SPI_Get_HSPI (hspi3)
#define MT6816_Mode_SPI (0x03)

typedef struct
{
    uint16_t sample_data;
    uint16_t angle;
    bool no_mag_flag;
    bool pc_flag;
} MT6816_SPI_Signal_Typedef;

typedef struct
{
    uint16_t angle_data;
    uint16_t rectify_angle;
    bool rectify_valid;
} MT6816_Typedef;

typedef struct
{
    uint16_t mt6701_raw_angle : 14;

    struct
    {
        uint8_t magnetic_state : 2;    // 磁场状态（0：正常1：过强2：过弱）
        uint8_t Push_button_state : 1; // 按钮是否被按下
        uint8_t Loss_of_Track : 1;     // 是否超速度
    } Status;

    uint8_t CRC_6Bit : 6;
} mt6701_RAW_t;

void MT6816_Init(void);

/**
 * @brief 获取MT6816的数据
 * @param  None
 * @return
 */
uint16_t REIN_MT6816_Get_AngleData(void);

/**
 * @brief 获取MT6701的数据
 * @param  None
 * @return
 */
uint16_t REIN_MT6701_Get_AngleData(void);

#endif
