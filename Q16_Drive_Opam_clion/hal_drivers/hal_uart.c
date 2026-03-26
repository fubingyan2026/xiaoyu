//
// Created by fubingyan on 25-9-27.
//
// hal_uart.c
// hal_uart.c
#include "hal_uart.h"

static const hal_uart_ops_t* uart_ops = NULL;

extern void platform_uart_init(void);

void hal_uart_set_ops(const hal_uart_ops_t* ops)
{
    uart_ops = ops;
}

bool hal_uart_init(const hal_uart_config_t* config)
{
    static uint8_t init_flag = 1;
    if (init_flag)
        platform_uart_init();
    init_flag = 0;

    if (uart_ops != NULL && uart_ops->init != NULL)
    {
        return uart_ops->init(config);
    }
    return false;
}

bool hal_uart_deinit(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->deinit != NULL)
    {
        return uart_ops->deinit(instance);
    }
    return false;
}

bool hal_uart_send(hal_uart_instance_e instance, const uint8_t* data, uint16_t size)
{
    if (uart_ops != NULL && uart_ops->send != NULL)
    {
        return uart_ops->send(instance, data, size);
    }
    return false;
}

bool hal_uart_receive(hal_uart_instance_e instance, uint8_t* data, uint16_t size)
{
    if (uart_ops != NULL && uart_ops->receive != NULL)
    {
        return uart_ops->receive(instance, data, size);
    }
    return false;
}

uint16_t hal_uart_send_async(hal_uart_instance_e instance, const uint8_t* data, uint16_t size)
{
    if (uart_ops != NULL && uart_ops->send_async != NULL)
    {
        return uart_ops->send_async(instance, data, size);
    }
    return 0;
}

uint16_t hal_uart_receive_async(hal_uart_instance_e instance, uint8_t* data, uint16_t size)
{
    if (uart_ops != NULL && uart_ops->receive_async != NULL)
    {
        return uart_ops->receive_async(instance, data, size);
    }
    return 0;
}

bool hal_uart_send_dma(hal_uart_instance_e instance, const uint8_t* data, uint32_t size)
{
    if (uart_ops != NULL && uart_ops->send_dma != NULL)
    {
        return uart_ops->send_dma(instance, data, size);
    }
    return false;
}

bool hal_uart_receive_dma(hal_uart_instance_e instance, uint8_t* data, uint32_t size)
{
    if (uart_ops != NULL && uart_ops->receive_dma != NULL)
    {
        return uart_ops->receive_dma(instance, data, size);
    }
    return false;
}

bool hal_uart_receive_dma_to_idle(hal_uart_instance_e instance, uint8_t* data, uint32_t size)
{
    if (uart_ops != NULL && uart_ops->receive_dma_to_idle != NULL)
    {
        return uart_ops->receive_dma_to_idle(instance, data, size);
    }
    return false;
}

bool hal_uart_abort_send(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->abort_send != NULL)
    {
        return uart_ops->abort_send(instance);
    }
    return false;
}

bool hal_uart_abort_receive(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->abort_receive != NULL)
    {
        return uart_ops->abort_receive(instance);
    }
    return false;
}

bool hal_uart_abort_send_dma(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->abort_send_dma != NULL)
    {
        return uart_ops->abort_send_dma(instance);
    }
    return false;
}

bool hal_uart_abort_receive_dma(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->abort_receive_dma != NULL)
    {
        return uart_ops->abort_receive_dma(instance);
    }
    return false;
}

uint32_t hal_uart_get_tx_count(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->get_tx_count != NULL)
    {
        return uart_ops->get_tx_count(instance);
    }
    return 0;
}

uint32_t hal_uart_get_rx_count(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->get_rx_count != NULL)
    {
        return uart_ops->get_rx_count(instance);
    }
    return 0;
}

hal_uart_dma_status_t hal_uart_get_dma_status(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->get_dma_status != NULL)
    {
        return uart_ops->get_dma_status(instance);
    }

    hal_uart_dma_status_t status = {0};
    return status;
}

bool hal_uart_set_baudrate(hal_uart_instance_e instance, hal_uart_baudrate_e baudrate)
{
    if (uart_ops != NULL && uart_ops->set_baudrate != NULL)
    {
        return uart_ops->set_baudrate(instance, baudrate);
    }
    return false;
}

bool hal_uart_set_rx_timeout(hal_uart_instance_e instance, uint32_t timeout)
{
    if (uart_ops != NULL && uart_ops->set_rx_timeout != NULL)
    {
        return uart_ops->set_rx_timeout(instance, timeout);
    }
    return false;
}

void hal_uart_irq_handler(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->irq_handler != NULL)
    {
        uart_ops->irq_handler(instance);
    }
}

void hal_uart_dma_irq_handler(hal_uart_instance_e instance)
{
    if (uart_ops != NULL && uart_ops->dma_irq_handler != NULL)
    {
        uart_ops->dma_irq_handler(instance);
    }
}
