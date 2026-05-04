#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include "debug.h"
#define DEBUG_H

#include "maths.h"
#include "user_lib.h"
#include "bsp_delay.h"
#include "controller/gimbal_pid.h"
#include "controller/pid.h"
#include "daemon.h"
#include "key_base.h"
#include "kfifo.h"
#include "led.h"
#include "memory_pool.h"
#include "toolkit.h"
#include "warning_task.h"

extern tk_event_t* interrupt_event;

extern const uint8_t TIM_EVENT_FLAG;
extern const uint8_t ADC_EVENT_FLAG;

void AppInit(void);
void AppRunning(void);

#endif
