#ifndef CAN_SERVER_H
#define CAN_SERVER_H

#include "fdcan.h"

#define CAN_SLIVER_COUNTS 4

typedef struct __attribute__((packed)) {
  int16_t velocity[CAN_SLIVER_COUNTS];
  uint8_t reserved;
} can_get_master_data_t;

typedef struct __attribute__((packed)) {
  float velocity_rpm;
  uint8_t reserved;
} can_send_sliver_data_t;

void FDCAN1_Config(void);

void FDCAN_Server_Task(void);

float FDCAN_Get_Master_Current(void);

#endif
