
#ifndef USART_RECEIVE_H
#define USART_RECEIVE_H

#include "kfifo/kfifo.h"

extern kfifo_t *fifo_usart1_rx;

extern kfifo_t *fifo_usart1_tx;

extern void Uasrt_IT_Init(void);

extern void UART1_IDLECallback(void);

extern void Uart_Process_Task(void);

#endif /* APPLICATION_USART_RECEIVE_H_ */
