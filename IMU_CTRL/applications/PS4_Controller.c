/**
 * @brief: PS4控制器数据接收(通过ESP32中转)
 * @FilePath: PS4_Controller.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-02-27 22:03:47
 * @LastEditTime: 2025-02-27 22:11:28
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "PS4_Controller.h"
#include "string.h"

const j = sizeof(ps4_t);
ps4_t *ps4_rx_pointer;

/**
 * @brief 读取ESP32蓝牙连接PS4通过串口传输过来的数据
 * @param data_buff 串口接收数据缓存
 * @param _ps4 PS4数据结构体
 * @param data_lenth 接收数据长度
 */
extern void Get_PS4_Controller_data(uint8_t *data_buff, ps4_t *_ps4, uint16_t data_lenth)
{
    ps4_rx_pointer = (ps4_t *)data_buff;
}
