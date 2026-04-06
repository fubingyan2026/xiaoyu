/**
 * @brief:系统管理
 * @FilePath: app.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-07-09 10:38:13
 * @LastEditTime: 2025-10-21 10:52:00
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "app.h"

#include <cm_backtrace.h>
#include <stdint.h>

#include "CAN_Server.h"
#include "WS2812_SPI.h"
#include "can_comm.h"
#include "controller/pid.h"
#include "debug/debug.h"
#include "easyflash.h"
#include "encoder/line_hall_pll.h"
#include "flash_task.h"
#include "foc_ctrl_q16.h"
#include "hal_uart.h"
#include "key_menu.h"
#include "kfifo/kfifo.h"
#include "lwmem/lwmem.h"
#include "lwshell/lwshell.h"
#include "opamp.h"
#include "protocol_parser.h"
#include "toolkit/toolkit.h"
#include "usart_protocol.h"
#include "usart_receive.h"
#include "warning_task.h"
/* Command to get called */
int32_t mycmd_fn(int32_t argc, char** argv) {
  DEBUG_DEBUG("mycmd_fn called. Number of argv: %d\r\n", (int)argc);
  for (int32_t i = 0; i < argc; ++i) {
    DEBUG_DEBUG("ARG[%d]: %s\r\n", (int)i, argv[i]);
  }
  /* Successful execution */
  return 0;
}

static uint8_t usart1_tx_buffer[1024];

/* UART上下文 */
extern hal_uart_context_t uart1_ctx;

gimbal_PID_t* gimbal_pid_pitch_instance;
pt1Filter_t filter_cog_current;
pid_type_def pid_type_velocity;

lwmem_stats_t lwmem_stats;
/* 事件1句柄 */
tk_event_t* interrupt_event = NULL;
tk_event_t* can_send_event = NULL;
/* 事件1标志 */
const uint8_t TIM_EVENT_FLAG = 1 << 1;
const uint8_t ADC_EVENT_FLAG = 1 << 2;

tk_timer_t* timer_ledTask = NULL;
tk_timer_t* timer_uartTask = NULL;
tk_timer_t* timer_driverTask = NULL;

/* 定时器3超时回调函数 */
void timer_ledTask_timeout_callback(tk_timer_t* timer) {
  led_task_refresh();
  WS2812_SPI_Loop();
  WS2812Flush();

  key_func_task();
  warning_task();
  KeyBaseTask();
  uart_process_task();
  daemon_task();
}

/* 定时器4超时回调函数 */
void timer_uartTask_timeout_callback(tk_timer_t* timer) {
  // BSP_Printf("Received INS:%d\r\n", (int)
  // (can_rx_ins.f_angle[1]*RAD_TO_ANGLE));
  //    BSP_Printf("HALL_ANGLE:%d,%d\r\n", (int)(gFocMachine.pll_phase *
  //    RAD_TO_ANGLE), (int)(gFocMachine.alginIq * 1000));
  // BSP_Printf("Motor Speed:%d\r\n", (int)(Get_Encoder_Velocity_RPM()));

  hal_uart_dma_status_t dma_status;

  const uint16_t length = kfifo_len(fifo_usart1_tx);
  if (length) {
    uint8_t* tx_buffer = usart1_tx_buffer;
    const uint16_t send_len =
        kfifo_peek(fifo_usart1_tx, tx_buffer, sizeof(usart1_tx_buffer), 0);
    /* 打印到串口 */
    if (hal_uart_get_dma_status(&uart1_ctx, HAL_UART_INSTANCE_1, &dma_status) ==
            HAL_UART_OK &&
        !dma_status.tx_busy) {
      hal_uart_send_dma(&uart1_ctx, HAL_UART_INSTANCE_1, usart1_tx_buffer,
                        send_len);
      kfifo_skip(fifo_usart1_tx, send_len);
    }
    /* 打印到Jlink */
    SEGGER_WRITE(0, tx_buffer, send_len);
  }

  flash_task_process();
}

/* 定时器4超时回调函数(1ms) */
void timer_driverTask_timeout_callback(tk_timer_t* timer) {
  FDCAN_Server_Task();
  foc_sm_calc();
}

void AppInit(void) {
  HAL_OPAMP_Start(&hopamp1);
  HAL_OPAMP_Start(&hopamp3);
#if (ENABLE_DEBUG_PRINT)
#if (ENABLE_PRINTF_USING_RTT)
  /* SEGGER_RTT_MODE_NO_BLOCK_SKIP */
  SEGGER_RTT_ConfigDownBuffer(SEGGER_RTT_PRINTF_TERMINAL, "RTTDOWN", NULL, 0,
                              SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  SEGGER_RTT_SetTerminal(SEGGER_RTT_PRINTF_TERMINAL);
#endif
#endif
  if (easyflash_init() == EF_NO_ERR) {
    ef_print_env();
  }

  daemon_init(millis);  // 初始化守护进程系统

  warning_Init();  // led任务初始化
  WS2812_SPI_Init();
  FDCAN1_Config();
  uart_it_init();

  foc_init();  // FOC初始化(包含传感器初始化)

  cm_backtrace_init("odrive-foc", "V0.0.2", "V0.0.1");
  key_func_init();

  pt1FilterInit(&filter_cog_current, pt1FilterGain(5, 0.001f));
  flash_task_init();

  const float pid_velocity_param[4] = {0.005f, 0.0000025f, 0.0125f, 0.00f};
  PID_init(&pid_type_velocity, PID_POSITION, pid_velocity_param, 0.35f, 0.10f);
  /* pubic init function */

  /* 初始化软件定时器功能，并配置tick获取回调函数*/
  tk_timer_func_init(millis);
  timer_ledTask = tk_timer_create(timer_ledTask_timeout_callback);
  ASSERT(timer_ledTask);
  timer_uartTask = tk_timer_create(timer_uartTask_timeout_callback);
  ASSERT(timer_uartTask);
  timer_driverTask = tk_timer_create(timer_driverTask_timeout_callback);
  ASSERT(timer_driverTask);

  tk_timer_start(timer_ledTask, TIMER_MODE_LOOP, 10);
  tk_timer_start(timer_uartTask, TIMER_MODE_LOOP, 100);
  tk_timer_start(timer_driverTask, TIMER_MODE_LOOP,
                 (uint32_t)(STATE_PERIOD * 1000));

  /* 动态创建事件2 */
  interrupt_event = tk_event_create();
  ASSERT(interrupt_event);
  /* 动态创建事件2 */
  can_send_event = tk_event_create();
  ASSERT(can_send_event);

  lwshell_init();
  /* Define shell commands */
  lwshell_register_cmd("mycmd", mycmd_fn,
                       "Adds 2 integer numbers and prints them");

  lwmem_get_stats(&lwmem_stats); /* 默认池 */
  DEBUG_WARN("memory used :%d(percent)\r\n",
             (lwmem_stats.mem_size_bytes - lwmem_stats.mem_available_bytes) *
                 100 / lwmem_stats.mem_size_bytes);
  ASSERT(lwmem_stats.mem_available_bytes);
}

void AppRunning(void) {
  /* pubic init function */

  /* 事件1同时接收到了标志1和标志2，清除标志并触发事件处理程序 */
  if (true == tk_event_recv(interrupt_event, TIM_EVENT_FLAG | ADC_EVENT_FLAG,
                            TK_EVENT_OPTION_AND | TK_EVENT_OPTION_CLEAR,
                            NULL)) {
    foc_adc_irq_calc();
    Get_Time_Hook(0);
    tk_event_send(can_send_event, TIM_EVENT_FLAG);
  }

  if (true == tk_event_recv(can_send_event, TIM_EVENT_FLAG,
                            TK_EVENT_OPTION_OR | TK_EVENT_OPTION_CLEAR, NULL)) {
    /* NM状态机处理 */
    // can_nm_task();
    // CANNmStateMachine();
    GimbalPidLoop();
    CANCommGetDataTransmit_V2();  // 解析接受的数据包
    CANCommSendDataPackage_V2();  // 将要发送的数据打包
    CANCommSendFlush();           // can发送数据包
    tk_timer_loop_handler();
  }
}

// end of the file !
