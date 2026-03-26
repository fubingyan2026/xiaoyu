//
// Created by fubingyan on 25-9-21.
//
// hal_fdcan.c
#include "hal_fdcan.h"

static const hal_fdcan_ops_t *fdcan_ops = NULL;

extern void platform_fdcan_init(void);

void hal_fdcan_set_ops(const hal_fdcan_ops_t *ops)
{
    fdcan_ops = ops;
}

bool hal_fdcan_init(const hal_fdcan_config_t *config)
{
    static uint8_t init_flag = 1;
    if (init_flag)
        platform_fdcan_init();
    init_flag = 0;

    if (fdcan_ops != NULL && fdcan_ops->init != NULL) {
        return fdcan_ops->init(config);
    }
    return false;
}

bool hal_fdcan_send(hal_fdcan_instance_e instance, const hal_fdcan_message_t *message)
{
    if (fdcan_ops != NULL && fdcan_ops->send != NULL) {
        return fdcan_ops->send(instance, message);
    }
    return false;
}

bool hal_fdcan_receive(hal_fdcan_instance_e instance, hal_fdcan_message_t *message)
{
    if (fdcan_ops != NULL && fdcan_ops->receive != NULL) {
        return fdcan_ops->receive(instance, message);
    }
    return false;
}

uint32_t hal_fdcan_get_receive_level(hal_fdcan_instance_e instance,uint8_t fifo_address)
{
    if (fdcan_ops != NULL && fdcan_ops->receive_level != NULL) {
        return fdcan_ops->receive_level(instance, fifo_address);
    }
    return 0;
}

uint32_t hal_fdcan_get_send_fifo_level(hal_fdcan_instance_e instance)
{
    if (fdcan_ops != NULL && fdcan_ops->get_send_level != NULL)
    {
        return fdcan_ops->get_send_level(instance);
    }
    return 0;
}

bool hal_fdcan_set_filter(hal_fdcan_instance_e instance, const hal_fdcan_filter_t *filter)
{
    if (fdcan_ops != NULL && fdcan_ops->set_filter != NULL) {
        return fdcan_ops->set_filter(instance, filter);
    }
    return false;
}

bool hal_fdcan_set_mode(hal_fdcan_instance_e instance, hal_fdcan_mode_e mode)
{
    if (fdcan_ops != NULL && fdcan_ops->set_mode != NULL) {
        return fdcan_ops->set_mode(instance, mode);
    }
    return false;
}

uint32_t hal_fdcan_get_error_count(hal_fdcan_instance_e instance)
{
    if (fdcan_ops != NULL && fdcan_ops->get_error_count != NULL) {
        return fdcan_ops->get_error_count(instance);
    }
    return 0;
}

void hal_fdcan_irq_handler(hal_fdcan_instance_e instance)
{
    if (fdcan_ops != NULL && fdcan_ops->irq_handler != NULL) {
        fdcan_ops->irq_handler(instance);
    }
}
