//
// Created by fubingyan on 25-9-5.
//

#ifndef KEY_FUNCTION_H
#define KEY_FUNCTION_H

#include "keybase/key_base.h"
#include "stdint.h"

typedef enum
{
    FUNC_NONE = 0,   ///< 无功能状态
    FUNC_COMMAND,    ///< 命令模式
    FUNC_SAVE_VALUE, ///< 保存值模式
    FUNC_TEST_VALUE, ///< 测试值模式
} key_func_state_e;

void key_func_init(void);
void key_func_task(void);
key_func_state_e key_func_get_state(void);
uint32_t Get_KeyFunc_CAN_ID(void);

#endif // KEY_FUNCTION_H
