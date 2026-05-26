/**
 * @brief:  Warning task implementation file.
 * @FilePath: warning_task.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 09:49:54
 * @LastEditTime: 2025-12-08 16:06:31
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "warning_task.h"

#include "device_ws2812.h"
#include "key_menu.h"

void warning_Init(void) { }

void warning_task(void)
{
    static uint32_t error_code, last_error_code;
    error_code = 0;
    uint8_t err_bits = 0;

    clist_head_t* head = daemon_get_head();
    if (head == NULL) {
        return;
    }

    daemon_context_t* ctx;
    clist_for_each_entry(ctx, head, node)
    {
        if (!daemon_is_online(ctx)) {
            if (err_bits > 31) {
                err_bits = 31;
            }
            error_code |= (1 << err_bits);
        }
        err_bits++;
    }

    static key_fsm_state_e last_state = KEY_FSM_STATE_NONE;
    if (last_state != key_func_get_state()) {
        last_state = key_func_get_state();
        if (last_state == KEY_FSM_STATE_NONE) {
            goto reset_code;
        }
    }

    if (last_error_code != error_code) {
        // 有错误.
        last_error_code = error_code;
    reset_code:
        if (last_error_code) {
            device_ws2812_mode_set(DEVICE_WS2812_MODE_ERROR_CODE, last_error_code);
        } else // 没有错误.
        {
            device_ws2812_mode_set(DEVICE_WS2812_MODE_FLOW, 0);
        }
    }
}
