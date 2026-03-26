#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include "stdint.h"

extern void delay_init(void);

extern void delay_us(uint16_t nus);

extern void delay_ms(uint16_t nms);

extern uint32_t micros(void);

extern uint32_t millis(void);

extern void Get_Time_Hook(uint8_t count);

extern const uint32_t *get_time_tick_pointer(void);

#endif
