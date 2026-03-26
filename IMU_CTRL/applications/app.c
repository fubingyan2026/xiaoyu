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
#include "WS2812_SPI.h"
#include "algorithm/maths.h"
#include "algorithm/utils_math.h"
#include "can_comm/can_comm.h"
#include "controller/pid.h"
#include "debug/debug.h"
#include "easyflash.h"
#include "hal_uart.h"
#include "key_function.h"
#include "protocol/b_protocol_core.h"
#include "stm32g4xx_hal_gpio.h"
#include "toolkit/toolkit.h"
#include "usart_receive.h"
#include "warning_task.h"
#include <cm_backtrace.h>

#include "CAN_Server.h"
#include "gimbal_task.h"
#include "ins_task.h"
#include "lwmem/lwmem.h"
#include "lwshell/lwshell.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"

static uint8_t usart1_tx_buffer[256];
lwmem_stats_t lwmem_stats;

daemon_t *u1_rx;
daemon_t *u1_tx;

void StartNormalTask(void *argument)
{
    for (;;)
    {
        /* code */
        LedTaskRefresh();
        // WS2812_SPI_Loop();
        // WS2812Flush();

        key_func.key_func_task();

        warning_task();

        KeyBaseTask();

        Uart_Process_Task();

        INS_task_Loop();

        DaemonTask();

        vTaskDelay(2);
    }
}

void StartDebugTask(void *argument)
{
    for (;;)
    {
        /* code */
        if (false == hal_uart_get_dma_status(HAL_UART_INSTANCE_1).tx_busy)
        {
            const uint16_t length = kfifo_len(fifo_usart1_tx);
            if (length)
            {
                uint8_t *tx_buffer = usart1_tx_buffer;
                const uint16_t send_len = kfifo_get(fifo_usart1_tx, tx_buffer, sizeof(usart1_tx_buffer));
                /* 打印到串口 */
                // hal_uart_send_dma(HAL_UART_INSTANCE_1, usart1_tx_buffer, send_len);
                /* 打印到Jlink */
                SEGGER_WRITE(0, tx_buffer, send_len);
            }
        }
        DaemonReload(u1_tx);

        vTaskDelay(20);
    }
}

void StartCanCommTask(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
        /* code */
        Gimbal_Task();
        FDCAN_Server_Task();
    }
}

void StartCaculateTask(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // __HAL_RCC_GPIOB_CLK_ENABLE();

    // GPIO_InitTypeDef GPIO_InitStruct = {0};

    // /*Configure GPIO pin : NRST_Pin */
    // GPIO_InitStruct.Pin = GPIO_PIN_9;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_PULLUP;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    // HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    for (;;)
    {
        // 1. 严格的时间同步
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));

        GimbalPidLoop();
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 1);

        CANCommSendDataPackage_V2();

        CANCommGetDataTransmit_V2();

        CANCommSendFlush();
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
    }
}

/* Command to get called */
int32_t mycmd_fn(int32_t argc, char **argv)
{
    BSP_Printf("mycmd_fn called. Number of argv: %d\r\n", (int)argc);
    for (int32_t i = 0; i < argc; ++i)
    {
        BSP_Printf("ARG[%d]: %s\r\n", (int)i, argv[i]);
    }
    /* Successful execution */
    return 0;
}

/* 事件1句柄 */
tk_event_t *interrupt_event = NULL;
/* 事件1标志 */
const uint8_t TIM_EVENT_FLAG = 1 << 1;

tk_timer_t *timer_ledTask = NULL;
tk_timer_t *timer_uartTask = NULL;
tk_timer_t *timer_driverTask = NULL;

/* 定时器3超时回调函数 */
void timer_ledTask_timeout_callback(tk_timer_t *timer)
{
    LedTaskRefresh();
    // WS2812_SPI_Loop();
    // WS2812Flush();

    key_func.key_func_task();
    warning_task();
    KeyBaseTask();
    Uart_Process_Task();
    INS_task_Loop();
    DaemonTask();
}

/* 定时器4超时回调函数 */
void timer_uartTask_timeout_callback(tk_timer_t *timer)
{
    if (false == hal_uart_get_dma_status(HAL_UART_INSTANCE_1).tx_busy)
    {
        const uint16_t length = kfifo_len(fifo_usart1_tx);
        if (length)
        {
            uint8_t *tx_buffer = usart1_tx_buffer;
            const uint16_t send_len = kfifo_get(fifo_usart1_tx, tx_buffer, sizeof(usart1_tx_buffer));
            /* 打印到串口 */
            hal_uart_send_dma(HAL_UART_INSTANCE_1, usart1_tx_buffer, send_len);
            /* 打印到Jlink */
            SEGGER_WRITE(0, tx_buffer, send_len);
        }
    }
    DaemonReload(u1_rx);
    DaemonReload(u1_tx);
}

/* 定时器4超时回调函数 */
void timer_driverTask_timeout_callback(tk_timer_t *timer)
{
    Gimbal_Task();
    FDCAN_Server_Task();
}

void AppInit(void)
{
    if (easyflash_init() != EF_NO_ERR)
    {
        ASSERT(NULL);
    }
    extern void platform_fdcan_init(void);
    platform_fdcan_init();
    warning_Init(); // led任务初始化
    // WS2812_SPI_Init();

    DaemonInit(millis);// 初始化守护进程系统
    FDCAN1_Config();
    Uasrt_IT_Init();
    INS_task_Init();

#if (ENABLE_DEBUG_PRINT)
#if (EANBLE_PRINTF_USING_RTT)
    /* SEGGER_RTT_MODE_NO_BLOCK_SKIP */
    SEGGER_RTT_ConfigDownBuffer(SEGGER_RTT_PRINTF_TERMINAL, "RTTDOWN", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_SetTerminal(SEGGER_RTT_PRINTF_TERMINAL);
#endif
#endif

    cm_backtrace_init("master_controller", "V0.0.2", "V0.0.1");

    key_func.key_func_init();
    Gimbal_Task_Init();
    /* pubic init function */

    u1_rx = get_daemon_instance("uart1_rx");
    u1_tx = get_daemon_instance("uart1_tx");

    /* 初始化软件定时器功能，并配置tick获取回调函数*/
    tk_timer_func_init(millis);
    timer_ledTask = tk_timer_create(timer_ledTask_timeout_callback);
    ASSERT(timer_ledTask);
    timer_uartTask = tk_timer_create(timer_uartTask_timeout_callback);
    ASSERT(timer_uartTask);
    timer_driverTask = tk_timer_create(timer_driverTask_timeout_callback);
    ASSERT(timer_driverTask);

    tk_timer_start(timer_ledTask, TIMER_MODE_LOOP, 5);
    tk_timer_start(timer_uartTask, TIMER_MODE_LOOP, 20);
    tk_timer_start(timer_driverTask, TIMER_MODE_LOOP, 1);

    /* 动态创建事件2 */
    interrupt_event = tk_event_create();
    ASSERT(interrupt_event);

    lwshell_init();
    /* Define shell commands */
    lwshell_register_cmd("mycmd", mycmd_fn, "Adds 2 integer numbers and prints them");

    lwmem_get_stats(&lwmem_stats); /* 默认池 */
    DEBUG_WARN("memory used :%d(percent)\r\n",
               (lwmem_stats.mem_size_bytes - lwmem_stats.mem_available_bytes) * 100 / lwmem_stats.mem_size_bytes);
    ASSERT(lwmem_stats.mem_available_bytes);
}

void AppRunning(void)
{
    /* pubic init function */
    /* 事件1同时接收到了标志1和标志2，清除标志并触发事件处理程序 */
    if (true == tk_event_recv(interrupt_event, TIM_EVENT_FLAG, TK_EVENT_OPTION_AND | TK_EVENT_OPTION_CLEAR, NULL))
    {
    }
    GimbalPidLoop();
    CANCommSendDataPackage_V2();
    CANCommSendFlush();
    CANCommGetDataTransmit_V2();
    tk_timer_loop_handler();
}

// end of the file !
