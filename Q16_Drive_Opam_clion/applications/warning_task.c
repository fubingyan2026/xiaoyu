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

#include "WS2812_SPI.h"
#include "daemon/daemon.h"
#include "key_menu.h"
#include "led.h"

static daemon_t* daemon_warning;
static led_handle_t* led_instance;

#define LED_NAME_TAG "LED0"

/**
 * @brief 初始化警告系统，设置守护进程指针
 *
 * 该函数调用DaemonGetMasterPointer()获取主守护进程的指针，并将其赋值给全局变量daemon_warning。
 * 这是警告系统初始化的一部分，确保后续操作能够访问到正确的守护进程实例。
 *
 * @param None
 * @return None
 */
void warning_Init(void) {
  daemon_warning = NULL;
  led_instance = NULL;
}

/**
 * @brief  执行警告任务，检查守护进程状态并更新LED显示
 *
 * 该函数首先初始化错误代码和优先级，并在首次调用时获取LED实例。
 * 然后遍历所有守护进程列表，查找离线的守护进程，并根据其优先级和错误代码更新当前的错误代码和优先级。
 * 如果功能键的状态发生变化，则重置错误代码。如果错误代码发生变化，根据是否有错误更新LED的闪烁模式和间隔。
 *
 * @param  None
 * @return None
 */
void warning_task(void) {
  static uint32_t error_code, last_error_code;
  error_code = 0;
  uint8_t err_bits = 0;
  // 发现错误
  if (daemon_warning == NULL) {
    daemon_warning = DaemonGetMasterPointer();
  }

  if (led_instance == NULL) {
    led_instance = LedGetInstance(LED_NAME_TAG);
  }

  const daemon_t* this = daemon_warning;
  while (this->next_daemon_lists) {
    this = this->next_daemon_lists;
    if (!DaemonIsOnline(this)) {
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
      WS2812_Mode_Set(WS2812_MODE_ERROR_CODE, last_error_code);
    } else  // 没有错误.
    {
      WS2812_Mode_Set(WS2812_MODE_FLOW, 0);
    }
  }
}
