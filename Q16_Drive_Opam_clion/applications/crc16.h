//
// Created by fubingyan on 25-4-5.
//

#ifndef __CRC16_H
#define __CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 计算CRC16 Modbus校验值
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16校验值
 */
uint16_t crc16_modbus(const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CRC16_H */
