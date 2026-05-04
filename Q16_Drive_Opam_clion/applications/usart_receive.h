/**
 * @file    usart_receive.h
 * @brief   串口数据接收模块 - UART DMA接收与协议解析
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
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,添加中文注释 |
 * | 1.0   | 2024-06-24 | fubingyan| 初始版本           |
 */

#ifndef USART_RECEIVE_H
#define USART_RECEIVE_H

#include "public.h"

/*============================================================================
 * 外部变量声明
 *============================================================================*/

/**
 * @brief UART1接收FIFO缓冲区
 * @note 由usart_receive模块管理,仅供只读访问
 */
extern kfifo_t* fifo_usart1_rx;

/**
 * @brief UART1发送FIFO缓冲区
 * @note 由usart_receive模块管理,仅供只读访问
 */
extern kfifo_t* fifo_usart1_tx;

/*============================================================================
 * 函数声明
 *============================================================================*/

/**
 * @brief 初始化UART1的中断和DMA接收
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 须在系统初始化阶段调用
 * @return void
 */
extern void uart_it_init(void);

/**
 * @brief UART1空闲中断回调函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: YES (在中断上下文调用)
 * @return void
 */
extern void uart1_idle_callback(void);

/**
 * @brief 处理UART数据的任务函数
 * @note ASIL-B | Reentrant: NO | ISR-Safe: NO
 * @note 须在主循环或专用任务中周期性调用
 * @return void
 */
extern void uart_process_task(void);

#endif /* USART_RECEIVE_H */
