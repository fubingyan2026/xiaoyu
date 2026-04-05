#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include "algorithm/maths.h"
#include "algorithm/user_lib.h"
#include "bsp_delay.h"
#include "controller/gimbal_pid.h"
#include "controller/pid.h"
#include "daemon.h"
#include "keybase/key_base.h"
#include "kfifo/kfifo.h"
#include "led.h"
#include "memory_pool/memory_pool.h"
#include "message_center/message_center.h"
#include "toolkit/toolkit.h"
#include "warning_task.h"

extern tk_event_t* interrupt_event;

extern const uint8_t TIM_EVENT_FLAG;
extern const uint8_t ADC_EVENT_FLAG;

void AppInit(void);
void AppRunning(void);

#endif
