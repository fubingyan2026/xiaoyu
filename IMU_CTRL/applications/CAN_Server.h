#ifndef CAN_SERVER_H
#define CAN_SERVER_H

#include "fdcan.h"

#define CAN_SLIVER_COUNTS 4

typedef struct  __attribute__((packed))
{
    int16_t velocity_rpm[CAN_SLIVER_COUNTS];
    uint8_t xxx;
} can_send_sliver_data_t;

typedef struct __attribute__((packed))
{
    float velocity_rpm;
    uint8_t reserved;
} can_get_sliver_data_t;

void FDCAN1_Config(void);

void FDCAN_Server_Task(void);

void SetSliverTargetCurrent(uint8_t index,int16_t current);

can_get_sliver_data_t * GetSliverFeedbackMessage(uint8_t index);
#endif
