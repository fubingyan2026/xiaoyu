//
// Created by fubingyan on 25-9-5.
//

#ifndef KEY_FUNCTION_H
#define KEY_FUNCTION_H

#include "keybase/key_base.h"
#include "stdint.h"

typedef enum
{
    FUNC_NONE = 0,
    FUNC_COMMAND,
    FUNC_SAVE_VALUE,
    FUNC_TEST_VALUE,

} key_func_state_e;

typedef struct
{
    key_func_state_e key_func_state;
    uint8_t key_command;
    uint8_t key_values;
    uint32_t times;
    uint32_t timesSysLast;
    uint32_t diffTimes;
    uint32_t timesLast;
    uint32_t key_valuesLast;
    uint32_t key_entry_command_times_ms;
    key_event_e key_event;
    void (*key_func_init)(void);
    void (*key_func_task)(void);
    void (*key_func_callback)(uint8_t comm, uint8_t value);
} key_func_t;

extern key_func_t key_func;
extern uint32_t can_save_id;

key_func_state_e key_func_get_state(void);

uint32_t Get_KeyFunc_CAN_ID(void);
#endif // KEY_FUNCTION_H
