#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include "public.h"

#include "bsp_delay.h"
#include "warning_task.h"

extern tk_event_t* interrupt_event;

extern const uint8_t TIM_EVENT_FLAG;
extern const uint8_t ADC_EVENT_FLAG;

void AppInit(void);
void AppRunning(void);

#endif
