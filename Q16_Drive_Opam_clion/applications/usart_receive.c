/**
 * @file    usart_receive.c
 * @brief   串口数据接收 - UART DMA接收与协议解析实现
 * @author  fubingyan qq:3245784484
 * @date    2024-06-24
 * @version V2.1
 *
 * @note    负责UART1的DMA接收、FIFO缓冲和A5协议解析
 * @note    编码规范: MISRA C:2012, AUTOSAR C++14
 *
 * 修订历史:
 * | 版本  | 日期       | 作者   | 描述                  |
 * |-------|------------|--------|-----------------------|
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,使用静态FIFO |
 * | 1.0   | 2024-06-24 | fubingyan| 初始版本           |
 */

#include "usart_receive.h"
#include "debug/debug.h"
#include "usart.h"
#include "usart_protocol.h"

#include "app.h"
#include "daemon/daemon.h"
#include "hal_uart.h"
#include "kfifo/kfifo.h"
#include "lwshell/lwshell.h"
#include "protocol/b_protocol_core.h"

#include "stm32g4xx_hal.h"
#include <string.h>

extern DMA_HandleTypeDef hdma_usart1_rx;

/*============================================================================
 * CRC16 Modbus表 - 静态存储于ROM
 *============================================================================*/
static const uint16_t crc16_table[256U] = {
    0x0000U, 0xC0C1U, 0xC181U, 0x0140U, 0xC301U, 0x03C0U, 0x0280U, 0xC241U, 0xC601U, 0x06C0U, 0x0780U, 0xC741U, 0x0500U,
    0xC5C1U, 0xC481U, 0x0440U, 0xCC01U, 0x0CC0U, 0x0D80U, 0xCD41U, 0x0F00U, 0xCFC1U, 0xCE81U, 0x0E40U, 0x0A00U, 0xCAC1U,
    0xCB81U, 0x0B40U, 0xC901U, 0x09C0U, 0x0880U, 0xC841U, 0xD801U, 0x18C0U, 0x1980U, 0xD941U, 0x1B00U, 0xDBC1U, 0xDA81U,
    0x1A40U, 0x1E00U, 0xDEC1U, 0xDF81U, 0x1F40U, 0xDD01U, 0x1DC0U, 0x1C80U, 0xDC41U, 0x1400U, 0xD4C1U, 0xD581U, 0x1540U,
    0xD701U, 0x17C0U, 0x1680U, 0xD641U, 0xD201U, 0x12C0U, 0x1380U, 0xD341U, 0x1100U, 0xD1C1U, 0xD081U, 0x1040U, 0xF001U,
    0x30C0U, 0x3180U, 0xF141U, 0x3300U, 0xF3C1U, 0xF281U, 0x3240U, 0x3600U, 0xF6C1U, 0xF781U, 0x3740U, 0xF501U, 0x35C0U,
    0x3480U, 0xF441U, 0x3C00U, 0xFCC1U, 0xFD81U, 0x3D40U, 0xFF01U, 0x3FC0U, 0x3E80U, 0xFE41U, 0xFA01U, 0x3AC0U, 0x3B80U,
    0xFB41U, 0x3900U, 0xF9C1U, 0xF881U, 0x3840U, 0x2800U, 0xE8C1U, 0xE981U, 0x2940U, 0xEB01U, 0x2BC0U, 0x2A80U, 0xEA41U,
    0xEE01U, 0x2EC0U, 0x2F80U, 0xEF41U, 0x2D00U, 0xEDC1U, 0xEC81U, 0x2C40U, 0xE401U, 0x24C0U, 0x2580U, 0xE541U, 0x2700U,
    0xE7C1U, 0xE681U, 0x2640U, 0x2200U, 0xE2C1U, 0xE381U, 0x2340U, 0xE101U, 0x21C0U, 0x2080U, 0xE041U, 0xA001U, 0x60C0U,
    0x6180U, 0xA141U, 0x6300U, 0xA3C1U, 0xA281U, 0x6240U, 0x6600U, 0xA6C1U, 0xA781U, 0x6740U, 0xA501U, 0x65C0U, 0x6480U,
    0xA441U, 0x6C00U, 0xACC1U, 0xAD81U, 0x6D40U, 0xAF01U, 0x6FC0U, 0x6E80U, 0xAE41U, 0xAA01U, 0x6AC0U, 0x6B80U, 0xAB41U,
    0x6900U, 0xA9C1U, 0xA881U, 0x6840U, 0x7800U, 0xB8C1U, 0xB981U, 0x7940U, 0xBB01U, 0x7BC0U, 0x7A80U, 0xBA41U, 0xBE01U,
    0x7EC0U, 0x7F80U, 0xBF41U, 0x7D00U, 0xBDC1U, 0xBC81U, 0x7C40U, 0xB401U, 0x74C0U, 0x7580U, 0xB541U, 0x7700U, 0xB7C1U,
    0xB681U, 0x7640U, 0x7200U, 0xB2C1U, 0xB381U, 0x7340U, 0xB101U, 0x71C0U, 0x7080U, 0xB041U, 0x5000U, 0x90C1U, 0x9181U,
    0x5140U, 0x9301U, 0x53C0U, 0x5280U, 0x9241U, 0x9601U, 0x56C0U, 0x5780U, 0x9741U, 0x5500U, 0x95C1U, 0x9481U, 0x5440U,
    0x9C01U, 0x5CC0U, 0x5D80U, 0x9D41U, 0x5F00U, 0x9FC1U, 0x9E81U, 0x5E40U, 0x5A00U, 0x9AC1U, 0x9B81U, 0x5B40U, 0x9901U,
    0x59C0U, 0x5880U, 0x9841U, 0x8801U, 0x48C0U, 0x4980U, 0x8941U, 0x4B00U, 0x8BC1U, 0x8A81U, 0x4A40U, 0x4E00U, 0x8EC1U,
    0x8F81U, 0x4F40U, 0x8D01U, 0x4DC0U, 0x4C80U, 0x8C41U, 0x4400U, 0x84C1U, 0x8581U, 0x4540U, 0x8701U, 0x47C0U, 0x4680U,
    0x8641U, 0x8201U, 0x42C0U, 0x4380U, 0x8341U, 0x4100U, 0x81C1U, 0x8081U, 0x4040U,
};

/**
 * @brief 计算CRC16 Modbus校验
 * @param[in] data 数据指针
 * @param[in] len 数据长度
 * @return uint16_t CRC16值
 */
static uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0U; i < len; i++)
    {
        crc = (crc >> 8U) ^ crc16_table[(crc ^ data[i]) & 0xFFU];
    }
    return crc;
}

/*============================================================================
 * 静态FIFO缓冲区定义 - 避免动态内存分配
 *============================================================================*/
#define UART_RX_FIFO_SIZE 128U
#define UART_TX_FIFO_SIZE 1024U

static uint8_t s_uart1_rx_buffer[UART_RX_FIFO_SIZE];
static uint32_t s_uart1_rx_lock = 0U;

static uint8_t s_uart1_tx_buffer[UART_TX_FIFO_SIZE];
static uint32_t s_uart1_tx_lock = 0U;

static kfifo_t s_fifo_usart1_rx;
static kfifo_t *s_fifo_usart1_rx_ptr = NULL;

static kfifo_t s_fifo_usart1_tx;
static kfifo_t *s_fifo_usart1_tx_ptr = NULL;

/**
 * @brief FIFO缓冲区指针 - 供外部访问
 * @note 使用静态初始化避免NULL检查
 */
kfifo_t *fifo_usart1_rx = &s_fifo_usart1_rx;
kfifo_t *fifo_usart1_tx = &s_fifo_usart1_tx;

/*============================================================================
 * UART1配置 - DMA模式
 *============================================================================*/
static hal_uart_config_t s_uart1_config = {.instance = HAL_UART_INSTANCE_1,
                                           .baudrate = HAL_UART_BAUDRATE_115200,
                                           .wordlength = HAL_UART_WORDLENGTH_8B,
                                           .stopbits = HAL_UART_STOPBITS_1,
                                           .parity = HAL_UART_PARITY_NONE,
                                           .hwcontrol = HAL_UART_HWCONTROL_NONE,
                                           .mode = HAL_UART_MODE_TX_RX,
                                           .timeout = 100U,
                                           .dma_config = {
                                               .dma_mode = HAL_UART_DMA_TX_RX,
                                               .tx_dma_buffer_size = UART_TX_FIFO_SIZE,
                                               .rx_dma_buffer_size = UART_RX_FIFO_SIZE,
                                               .rx_idle_timeout = 10U,
                                               .circular_mode_rx = true,
                                           }};

/*============================================================================
 * A5协议帧长度计算回调
 *============================================================================*/

/**
 * @brief 计算A5协议帧长度
 * @param[in] buffer 数据缓冲区
 * @param[in] len 当前数据长度
 * @return uint16_t 完整帧长度,0表示不完整
 */
/*============================================================================
 * S7协议帧长度计算回调
 *============================================================================*/

/**
 * @brief 计算S7协议帧长度
 * @param[in] buffer 数据缓冲区
 * @param[in] len 当前数据长度
 * @return uint16_t 完整帧长度,0表示不完整
 */
uint16_t s7_get_frame_len(uint8_t *buffer, uint16_t len)
{
    if (len < 8U)
    {
        return 0U;
    }
    /* S7帧格式: head(2) + proto_id(1) + msg_type(1) + func(1) + data(n) + crc(2) + tail(1) */
    /* 数据长度在第5个字节开始,需要至少8字节才能确定 */
    const uint8_t data_len = buffer[5U];
    const uint16_t total = (uint16_t)(2U + 1U + 1U + 1U + data_len + 2U + 1U);
    return total;
}

/*============================================================================
 * S7协议校验回调
 *============================================================================*/

/**
 * @brief S7协议帧校验 - CRC16 Modbus
 * @param[in] buffer 数据缓冲区
 * @param[in] len 数据长度
 * @return uint8_t 校验结果
 */
uint8_t s7_check_cb(uint8_t *buffer, uint16_t len)
{
    if (len < 8U)
    {
        return B_ERROR_CHECKSUM;
    }
    /* CRC16计算范围: 从协议ID到CRC之前 */
    uint16_t calc = crc16_modbus(&buffer[2U], (uint16_t)(len - 5U));
    uint16_t recv = (uint16_t)buffer[len - 3U] | ((uint16_t)buffer[len - 2U] << 8);
    return (calc == recv) ? B_SUCCESS : B_ERROR_CHECKSUM;
}

b_frame_type s_s7_protocol_frame;

/**
 * @brief 初始化S7协议
 * @param[in] frame 协议帧结构指针
 */
void init_s7_protocol(b_frame_type *frame)
{
    static uint8_t out_buf[UART_RX_FIFO_SIZE];
    b_frame_init_type cfg = {
        .pname = "S7_Protocol",
        .head = (const uint8_t *)"\x68\x68",
        .head_len = 2U,
        .end = (const uint8_t *)"\x16",
        .end_len = 1U,
        .get_frame_len_cb = s7_get_frame_len,
        .check_cb = s7_check_cb,
        .in_buffer_len = UART_RX_FIFO_SIZE,
        .out_frame_buffer = out_buf,
        .out_buffer_len = sizeof(out_buf),
    };
    (void)b_frame_init(frame, &cfg);
}

/*============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化UART1的中断和DMA接收
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 使用静态FIFO缓冲区,避免动态内存分配
 */
void uart_it_init(void)
{
    s_fifo_usart1_rx_ptr = kfifo_init(s_uart1_rx_buffer, UART_RX_FIFO_SIZE, 0, &s_uart1_rx_lock);

    if (s_fifo_usart1_rx_ptr != NULL)
    {
        (void)memcpy(&s_fifo_usart1_rx, s_fifo_usart1_rx_ptr, sizeof(kfifo_t));
    }
    fifo_usart1_rx = &s_fifo_usart1_rx;


    s_fifo_usart1_tx_ptr = kfifo_init(s_uart1_tx_buffer, UART_TX_FIFO_SIZE, 0, &s_uart1_tx_lock);

    if (s_fifo_usart1_tx_ptr != NULL)
    {
        (void)memcpy(&s_fifo_usart1_tx, s_fifo_usart1_tx_ptr, sizeof(kfifo_t));
    }
    fifo_usart1_tx = &s_fifo_usart1_tx;

    const daemon_init_config_t daemon_config_rx = {
        .callback = NULL,
        .owner_pointer = NULL,
        .owner_name = "uart1_rx",
        .reload_time_out = 1000U,
        .init_wait_time = 1500U,
    };
    (void)DaemonRegister(&daemon_config_rx);

    const daemon_init_config_t daemon_config_tx = {
        .callback = NULL,
        .owner_pointer = NULL,
        .owner_name = "uart1_tx",
        .reload_time_out = 1000U,
        .init_wait_time = 1500U,
    };
    (void)DaemonRegister(&daemon_config_tx);

    if (hal_uart_init(&s_uart1_config) != 0)
    {
        return;
    }

    if (fifo_usart1_rx != NULL)
    {
        (void)hal_uart_receive_dma(HAL_UART_INSTANCE_1, fifo_usart1_rx->buffer, fifo_usart1_rx->size);
    }

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);

    (void)init_s7_protocol(&s_s7_protocol_frame);
}

/**
 * @brief 处理UART数据的任务函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 须在主循环中周期性调用
 */
void uart_process_task(void)
{
    static uint8_t rx_data[UART_RX_FIFO_SIZE];
    uint16_t flen;
    const uint8_t *p;

    if (fifo_usart1_rx == NULL)
    {
        return;
    }

    const uint16_t rx_len = kfifo_get(fifo_usart1_rx, rx_data, (uint16_t)sizeof(rx_data));
    if (rx_len > 0U)
    {
        (void)b_frame_put(&s_s7_protocol_frame, rx_data, rx_len);
        lwshell_input(rx_data, rx_len);
    }

    p = b_frame_check_get(&s_s7_protocol_frame, &flen);
    if (p != NULL)
    {
        usart_protocol_process_frame((uint8_t *)p, flen);

        DEBUG_INFO("%s:", s_s7_protocol_frame.frame_init.pname);
        for (uint16_t i = 0U; i < flen; i++)
        {
            BSP_Printf(" 0x%02x", p[i]);
        }
        BSP_Printf("\r\n");
    }
    (void)b_frame_idie_timer(&s_s7_protocol_frame);
}

/**
 * @brief UART1空闲中断回调函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: YES
 */
void uart1_idle_callback(void)
{
    if (RESET != __HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))
    {
        (void)__HAL_UART_CLEAR_IDLEFLAG(&huart1);

        if ((fifo_usart1_rx != NULL) && (fifo_usart1_rx->size > 0U))
        {
            const uint32_t rx_dataDMALength = (uint32_t)(fifo_usart1_rx->size - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx));
            (void)kfifo_move_in(fifo_usart1_rx, rx_dataDMALength);
        }
    }
}
