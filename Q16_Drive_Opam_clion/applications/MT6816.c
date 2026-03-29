/**
 * @brief: MT6816 and MT6701编码器读取数据
 * @FilePath: MT6816.c
 * @author: fubingyan qq:3245784484
 * @Date: 2024-12-26 14:31:59
 * @LastEditTime: 2025-02-22 14:55:05
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2024 by fubingyan, All Rights Reserved.
 */

#include "MT6816.h"

#include "daemon/daemon.h"
#include "debug/debug.h"

MT6816_SPI_Signal_Typedef mt6816_spi = {0};
MT6816_Typedef mt6816 = {0};

uint16_t MT6701_data_buffer[2] = {0};
mt6701_RAW_t mt6701_RAW;

daemon_t* daemon_encoder;

static uint16_t RINE_MT6816_SPI_Get_AngleData(void);

static uint8_t crc6_itu(const uint16_t* data, uint32_t length);

static uint16_t MT6701_GetRawData(uint16_t* Raw_data);

/**
 * @brief MT6816初始化
 * @param  None
 */
void MT6816_Init(void) {
  mt6816_spi.sample_data = 0;
  mt6816_spi.angle = 0;

  const daemon_init_config_t daemon_config_encoder = {
      .callback = NULL,
      .owner_pointer = NULL,
      .owner_name = "encoder",
      .reload_time_out = 10,
      .init_wait_time = 1500,
  };
  daemon_encoder = DaemonRegister(&daemon_config_encoder);
  ASSERT(daemon_encoder);
}

/**
 * @brief 通过SPI从MT6816获取角度数据
 * @return 返回计算后的角度值
 */
static uint16_t RINE_MT6816_SPI_Get_AngleData(void) {
  uint16_t data_t[2];
  uint16_t data_r[2];
  data_t[0] = (0x80 | 0x03) << 8;
  data_t[1] = (0x80 | 0x04) << 8;
  for (uint8_t i = 0; i < 1; i++) {
    MT6816_SPI_CS_L();
    HAL_SPI_TransmitReceive(&MT6816_SPI_Get_HSPI, (uint8_t*)&data_t[0],
                            (uint8_t*)&data_r[0], 1, 10);
    MT6816_SPI_CS_H();

    MT6816_SPI_CS_L();
    HAL_SPI_TransmitReceive(&MT6816_SPI_Get_HSPI, (uint8_t*)&data_t[1],
                            (uint8_t*)&data_r[1], 1, 10);
    MT6816_SPI_CS_H();
    mt6816_spi.sample_data = ((data_r[0] & 0x00FF) << 8) | (data_r[1] & 0x00FF);

    uint8_t h_count = 0;
    for (uint8_t j = 0; j < 16; j++) {
      if (mt6816_spi.sample_data & (0x0001 << j)) h_count++;
    }
    if (h_count & 0x01) {
      mt6816_spi.pc_flag = false;
    } else {
      mt6816_spi.pc_flag = true;
      break;
    }
  }
  if (mt6816_spi.pc_flag) {
    mt6816_spi.angle = mt6816_spi.sample_data >> 2;
    mt6816_spi.no_mag_flag = (bool)(mt6816_spi.sample_data & (0x0001 << 1));
  }
  return mt6816_spi.angle;
}

/**
 * @brief 获取MT6701原始数据
 * @param Raw_data (0-16383)
 * @return angle
 */
static uint16_t MT6701_GetRawData(uint16_t* Raw_data) {
  uint16_t txData = 0x3FF;
  uint8_t Retry_count = 2;

  if (HAL_SPI_GetState(&MT6816_SPI_Get_HSPI) != HAL_SPI_STATE_READY) {
    return 0;
  }

Retry:

  Retry_count--;

  MT6816_SPI_CS_L();

  const HAL_StatusTypeDef spiStatus = HAL_SPI_TransmitReceive(
      &MT6816_SPI_Get_HSPI, (uint8_t*)&txData, (uint8_t*)MT6701_data_buffer,
      sizeof(MT6701_data_buffer), 100);
  if (spiStatus != HAL_OK) {
    MT6816_SPI_CS_H();
    return 0;  // 在SPI传输错误时直接返回，避免继续执行后续代码
  }

  MT6816_SPI_CS_H();

  // 数据状态位
  mt6701_RAW.mt6701_raw_angle = (MT6701_data_buffer[0] >> 2) & 0x3FFF;
  mt6701_RAW.Status.magnetic_state =
      ((MT6701_data_buffer[1] >> 14) | (MT6701_data_buffer[0] & 0x03) >> 2) &
      0x03;
  mt6701_RAW.Status.Push_button_state =
      ((MT6701_data_buffer[1] >> 14) | (MT6701_data_buffer[0] & 0x03) >> 1) &
      0x01;
  mt6701_RAW.Status.Loss_of_Track =
      ((MT6701_data_buffer[1] >> 14) | (MT6701_data_buffer[0] & 0x03) >> 0) &
      0x01;

  // 校验位
  mt6701_RAW.CRC_6Bit = (MT6701_data_buffer[1] >> 8) & 0x003F;
  uint8_t mt6701_crc = crc6_itu(MT6701_data_buffer, 3);

  if (mt6701_crc != mt6701_RAW.CRC_6Bit && Retry_count) {
    mt6701_crc = 0;
    mt6701_RAW.CRC_6Bit = 0;
    goto Retry;
  }
  //    if (Retry_count)
  //    {
  //        DaemonReload(daemon_encoder);
  //    }

  if (Raw_data != NULL) {
    *Raw_data = mt6701_RAW.mt6701_raw_angle;
  }
  return mt6701_RAW.mt6701_raw_angle;
}

/**
 * @brief mt6701 crc-6 check
 * @param data  6Bit数据当成8Bit传入(高2Bit不管)
 * @param length
 * @return crc-6 check
 */
static uint8_t crc6_itu(const uint16_t* data, uint32_t length) {
  uint8_t i = 0, j = 0;
  uint8_t crc = 0;  // Initial value

  const uint8_t crc_data[3] = {
      data[0] >> 10 & 0x3F, data[0] >> 4 & 0x3F,
      (data[1] >> 14 | (data[0] & 0x000F) << 2) & 0x3F};

  while (length--) {
    crc ^= crc_data[j++];  // crc ^= *data; data++;
    for (i = 6; i > 0; --i) {
      if (crc & 0x20)
        crc = crc << 1 ^ 0x03;
      else
        crc = crc << 1;
    }
  }
  return crc & 0x3f;
}

/**
 * @brief 获取MT6816角度数据(0-16383)
 * @return 返回从MT6816传感器读取的角度数据
 */
uint16_t REIN_MT6816_Get_AngleData(void) {
  return RINE_MT6816_SPI_Get_AngleData();
}

/**
 * @brief 获取MT6701的角度数据(0-16383)
 * @return 返回角度数据，单位为编码器原始值
 */
uint16_t REIN_MT6701_Get_AngleData(void) { return MT6701_GetRawData(NULL); }
