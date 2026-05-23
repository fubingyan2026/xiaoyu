//
// Created by fubingyan on 24-12-26.
//

#ifndef __DEVICE_WS2812_H
#define __DEVICE_WS2812_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported defines ----------------------------------------------------------*/
#define DEVICE_WS2812_LOW_LEVEL 0xC0
#define DEVICE_WS2812_HIGH_LEVEL 0xF8
#define DEVICE_WS2812_LED_COUNT 3

/* Exported types ------------------------------------------------------------*/

typedef enum {
    DEVICE_WS2812_MODE_FLOW = 0,
    DEVICE_WS2812_MODE_ERROR_CODE,
} device_ws2812_mode_t;

typedef struct {
    uint8_t head;
    uint8_t tx_buffer[24 * DEVICE_WS2812_LED_COUNT];
    uint8_t tail[24];
} device_ws2812_buffer_t;

/* Exported functions prototypes ---------------------------------------------*/

void device_ws2812_init(void);

void device_ws2812_ctrl(const uint16_t led_index, const uint8_t r,
    const uint8_t g, const uint8_t b);

void device_ws2812_flush(void);

void device_ws2812_loop(void);

void device_ws2812_mode_set(device_ws2812_mode_t mode, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_WS2812_H */
