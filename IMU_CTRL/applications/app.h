#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include "algorithm/maths.h"
#include "algorithm/user_lib.h"
#include "bsp_delay.h"
#include "controller/gimbal_pid.h"
#include "controller/pid.h"
#include "daemon/daemon.h"
#include "keybase/key_base.h"
#include "kfifo/kfifo.h"
#include "ledshow/led.h"
#include "memory_pool/memory_pool.h"
#include "message_center/message_center.h"
#include "toolkit/toolkit.h"
#include "warning_task.h"

typedef enum __attribute__((packed))
{
    DAEMON_CAN_PITCH = DAEMON_BASIS_LEN,
    DAEMON_CAN_ROLL,
    DAEMON_CAN_YAW,
    DAEMON_UART1_RX,
    DAEMON_UART1_TX,
    DAEMON_LEN
} daemon_error_code_e;

extern tk_event_t *interrupt_event;
extern const uint8_t TIM_EVENT_FLAG;

void AppInit(void);
void AppRunning(void);

#endif
