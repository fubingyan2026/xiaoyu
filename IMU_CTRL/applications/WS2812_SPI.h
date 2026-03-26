
#ifndef WS2812_SPI_H
#define WS2812_SPI_H

#include "stdint.h"

#define WS2812_LowLevel 0xC0  // 0码
#define WS2812_HighLevel 0xF8 // 1码
#define WS2812_SPI_UNIT hspi1
#define LED_AMOUNT (3) // LED数量

typedef enum
{
    WS2812_MODE_FLOW = 0,
    WS2812_MODE_ERROR_CODE,
} ws2812_mode_e;

typedef struct
{
    uint8_t head;
    uint8_t tx_buffer[24 * LED_AMOUNT];
    uint8_t tail[24]; // WS2812 的“断开时间”固定为 ≥300 µs，与帧率无关
} ws2812b_t;

void WS2812_Ctrl(const uint16_t led_index, const uint8_t r, const uint8_t g, const uint8_t b);

void WS2812_SPI_Init(void);

void WS2812_SPI_Loop(void);

void WS2812Flush(void);

void WS2812_Mode_Set(ws2812_mode_e _mode, uint8_t _num);

#endif /* ws2812Frame_h */
