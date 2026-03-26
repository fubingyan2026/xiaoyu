/**
 * @brief: 串口数据接收
 * @FilePath: usart_receive.c
 * @author: fubingyan qq:3245784484
 * @Date: 2024-06-24 21:50:37
 * @LastEditTime: 2025-09-28 21:16:50
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "usart_receive.h"

#include "debug/debug.h"
#include "usart.h"

#include "PS4_Controller.h"
#include "app.h"
#include "daemon/daemon.h"
#include "hal_uart.h"
#include "kfifo/kfifo.h"
#include "lwshell/lwshell.h"
#include "protocol/b_protocol_core.h"
extern DMA_HandleTypeDef hdma_usart1_rx;

kfifo_t *fifo_usart1_rx = NULL;
kfifo_t *fifo_usart1_tx = NULL;

daemon_t *daemon_usart1_rx = NULL;
#define UART_RX_FIFO_SIZE 128
static uint8_t rx_data[UART_RX_FIFO_SIZE] = {0};

// UART1配置（带DMA）
static hal_uart_config_t uart1_config = {.instance = HAL_UART_INSTANCE_1,
                                         .baudrate = HAL_UART_BAUDRATE_115200,
                                         .wordlength = HAL_UART_WORDLENGTH_8B,
                                         .stopbits = HAL_UART_STOPBITS_1,
                                         .parity = HAL_UART_PARITY_NONE,
                                         .hwcontrol = HAL_UART_HWCONTROL_NONE,
                                         .mode = HAL_UART_MODE_TX_RX,
                                         .timeout = 100,
                                         .dma_config = {
                                             .dma_mode = HAL_UART_DMA_TX_RX,
                                             .tx_dma_buffer_size = 1024,
                                             .rx_dma_buffer_size = UART_RX_FIFO_SIZE,
                                             .rx_idle_timeout = 10, // 10ms空闲超时
                                             .circular_mode_rx = true,
                                         }};

/* get_frame_len_cb
 * buffer[2] == cmd, buffer[3] == len
 */
uint16_t a5_get_frame_len(uint8_t *buffer, uint16_t len)
{
    if (len < 4)
        return 0; // head(2) + cmd(1) + len(1)
    const uint8_t payload_len = buffer[3];
    const uint16_t total = 2 + 1 + 1 + payload_len + 1 + 2; // head + cmd + len + payload + xor + tail
    return total;
}

// check_cb
uint8_t a5_check_cb(uint8_t *buffer, uint16_t len)
{
    if (len < 7)
        return B_ERROR_BUFFER_TOO_SMALL; // minimal frame length
    uint8_t calc = 0;
    for (uint16_t i = 2; i < len - 3; ++i)
    {
        // cmd .. last data byte
        calc ^= buffer[i];
    }
    const uint8_t cs = buffer[len - 3];
    return calc == cs ? B_SUCCESS : B_ERROR_CHECKSUM;
}

b_frame_type test_frame;

void init_a5_protocol(b_frame_type *frame)
{
    static uint8_t out_buf[UART_RX_FIFO_SIZE]; // 或者堆分配，需要满足最大帧
    b_frame_init_type cfg = {
        .pname = "A5_Protocol",
        .head = "\xA5\x5A",
        .head_len = 2,
        .end = "\x0D\x0A",
        .end_len = 2,
        .get_frame_len_cb = a5_get_frame_len,
        .check_cb = a5_check_cb,
        .in_buffer_len = UART_RX_FIFO_SIZE * 2,
        .out_frame_buffer = out_buf,
        .out_buffer_len = sizeof(out_buf),
    };
    b_frame_init(frame, &cfg);
}

/**
 * @brief 初始化UART1的中断和DMA接收
 *
 * 该函数初始化UART1的DMA接收功能，并使能空闲中断。具体操作包括：
 * - 使用HAL库的`HAL_UART_Receive_DMA`函数开始DMA传输，从UART1接收到的数据将被存储在预先定义的环形缓冲区中。
 * - 通过`__HAL_UART_ENABLE_IT`宏使能UART1的空闲中断，以便在没有数据传输时触发中断处理。
 *
 * @note
 * - 确保在调用此函数之前已经正确配置了UART1及其相关硬件资源。
 * - 此函数依赖于HAL库和特定的硬件配置，请确保所有必要的初始化已完成。
 * - FIFO缓冲区大小由`FIFO_MAX_BUFFER`定义，确保其值符合系统需求。
 * @retval None
 */
void Uasrt_IT_Init(void)
{
    static uint32_t lock = 0;

    fifo_usart1_rx = kfifo_alloc(UART_RX_FIFO_SIZE, &lock);
    ASSERT(fifo_usart1_rx);
    const daemon_init_config_t daemon_config_rx = {
        .callback = NULL,
        .owner_pointer = NULL,
        .owner_name = "uart1_rx",
        .reload_time_out = 1000,
        .init_wait_time = 1500,
        .warn_priority = 2,
        .warn_error_code = DAEMON_UART1_RX,
    };
    daemon_usart1_rx = DaemonRegister(&daemon_config_rx);
    ASSERT(daemon_usart1_rx);

    const daemon_init_config_t daemon_config_tx = {
        .callback = NULL,
        .owner_pointer = NULL,
        .owner_name = "uart1_tx",
        .reload_time_out = 1000,
        .init_wait_time = 1500,
        .warn_priority = 2,
        .warn_error_code = DAEMON_UART1_TX,
    };
    ASSERT(DaemonRegister(&daemon_config_tx));

    // 初始化UART1（带DMA）
    ASSERT(hal_uart_init(&uart1_config));

    ASSERT(hal_uart_receive_dma(HAL_UART_INSTANCE_1, fifo_usart1_rx->buffer, fifo_usart1_rx->size));
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);

    // init_a5_protocol(&test_frame);
}

/**
 * @brief 处理UART数据的任务函数
 *
 * 该函数用于处理通过UART接收到的数据。它从接收FIFO中读取数据，并将其传递给协议解析器进行处理。
 * 具体操作包括：
 * - 从`fifo_usart1_rx`中获取可用数据，并存储在临时缓冲区`rx_data`中。
 * - 如果接收到数据，则调用`b_frame_put`函数将数据传递给协议解析器`test_frame`进行处理。
 *
 * @note
 * - 确保在调用此函数之前已经正确初始化了UART和相关的FIFO缓冲区。
 * - 此函数应定期调用，以确保及时处理接收到的数据。
 * @retval None
 */
void Uart_Process_Task(void)
{
    // 处理接收到的数据
    uint16_t flen;
    const uint8_t *p;
    const uint16_t rx_len = kfifo_get(fifo_usart1_rx, rx_data, sizeof(rx_data));
    if (rx_len > 0)
    {
        // b_frame_put(&test_frame, rx_data, rx_len);
        // lwshell_input(rx_data, rx_len);
        Get_PS4_Controller_data(rx_data, NULL, sizeof(rx_data));

        static uint8_t send_counts = 0;
        if (++send_counts >= 50)
        {
            send_counts = 0;
            BSP_Printf("usart1rx%d,ps4:%d\r\n", rx_len, ps4_rx_pointer->analog.stick.lx);
        }
    }
    else
    {

        if (!DaemonIsOnline(daemon_usart1_rx))
        {
            HAL_UART_DMAStop(&huart1);
            __HAL_UART_CLEAR_IDLEFLAG(&huart1); // 清楚空闲中断标志（否则会一直不断进入中断）
            hal_uart_receive_dma(HAL_UART_INSTANCE_1, fifo_usart1_rx->buffer, fifo_usart1_rx->size);
            __HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);
        }
    }
    // p = b_frame_check_get(&test_frame, &flen);
    // if (p)
    // {
    //     DEBUG_INFO("%s:", test_frame.frame_init.pname);
    //     for (uint16_t i = 0; i < flen; i++)
    //     {
    //         BSP_Printf(" 0x%02x", p[i]);
    //     }
    //     BSP_Printf("\r\n");
    // }
}

/**
 * @brief : 处理UART1的空闲中断回调函数
 *
 * 该函数在UART1进入空闲中断时被调用。主要功能包括：
 * - 检查是否是空闲中断。
 * - 清除空闲中断标志，防止重复进入中断。
 * - 计算并更新接收缓冲区FIFO的头位置，以便正确处理接收到的数据。
 *
 * @note: 该函数依赖于HAL库和特定硬件配置，确保相关初始化已完成。
 * @retval: None
 */
void UART1_IDLECallback(void)
{
    if (RESET != __HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) // 判断是否是空闲中断
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1); // 清楚空闲中断标志（否则会一直不断进入中断）
        // HAL_UART_DMAStop(&huart1);
        const uint32_t rx_dataDMALength =
            fifo_usart1_rx->size - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx); // 计算接收到的数据长度
        kfifo_move_in(fifo_usart1_rx, rx_dataDMALength);
        DaemonReload(daemon_usart1_rx);

        // hal_uart_receive_dma(HAL_UART_INSTANCE_1, fifo_usart1_rx->buffer, fifo_usart1_rx->size);
    }
}

// end of the file
