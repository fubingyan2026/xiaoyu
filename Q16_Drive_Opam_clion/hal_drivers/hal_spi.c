//
// Created by fubingyan on 25-5-24.
//

/**
 * @file    hal_spi.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-05-24
 * @brief   硬件抽象层 - SPI 实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "hal_spi.h"

/* Exported functions --------------------------------------------------------*/

hal_spi_error_t hal_spi_set_ops(hal_spi_context_t* ctx,
    const hal_spi_ops_t* ops)
{
    if (ctx == NULL || ops == NULL) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (ops->init == NULL || ops->deinit == NULL || ops->transmit_receive == NULL || ops->transmit_dma == NULL || ops->get_state == NULL || ops->get_dma_state == NULL) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    HAL_SPI_ENTER_CRITICAL();
    ctx->ops = ops;
    HAL_SPI_EXIT_CRITICAL();

    return HAL_SPI_OK;
}

hal_spi_error_t hal_spi_init(hal_spi_context_t* ctx,
    const hal_spi_config_t* config)
{
    if (ctx == NULL || config == NULL) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (config->instance >= HAL_SPI_INSTANCE_LEN) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (!ctx->initialized) {
        ctx->initialized = 1;
    }

    if (ctx->ops == NULL || ctx->ops->init == NULL) {
        return HAL_SPI_ERROR_UNINITIALIZED;
    }

    ctx->instance = config->instance;

    HAL_SPI_ENTER_CRITICAL();
    hal_spi_error_t result = ctx->ops->init(ctx, config);
    HAL_SPI_EXIT_CRITICAL();

    return result;
}

hal_spi_error_t hal_spi_deinit(hal_spi_context_t* ctx)
{
    if (ctx == NULL) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (ctx->ops == NULL || ctx->ops->deinit == NULL) {
        return HAL_SPI_ERROR_UNINITIALIZED;
    }

    HAL_SPI_ENTER_CRITICAL();
    hal_spi_error_t result = ctx->ops->deinit(ctx);
    HAL_SPI_EXIT_CRITICAL();

    return result;
}

hal_spi_error_t hal_spi_transmit_receive(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint8_t* rx_data, uint16_t size,
    uint32_t timeout)
{
    if (ctx == NULL || tx_data == NULL || rx_data == NULL || size == 0) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (ctx->ops == NULL || ctx->ops->transmit_receive == NULL) {
        return HAL_SPI_ERROR_UNINITIALIZED;
    }

    HAL_SPI_ENTER_CRITICAL();
    hal_spi_error_t result = ctx->ops->transmit_receive(ctx, tx_data, rx_data, size, timeout);
    HAL_SPI_EXIT_CRITICAL();

    return result;
}

hal_spi_error_t hal_spi_transmit_dma(hal_spi_context_t* ctx,
    const uint8_t* tx_data, uint16_t size)
{
    if (ctx == NULL || tx_data == NULL || size == 0) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    if (ctx->ops == NULL || ctx->ops->transmit_dma == NULL) {
        return HAL_SPI_ERROR_UNINITIALIZED;
    }

    HAL_SPI_ENTER_CRITICAL();
    hal_spi_error_t result = ctx->ops->transmit_dma(ctx, tx_data, size);
    HAL_SPI_EXIT_CRITICAL();

    return result;
}

hal_spi_state_t hal_spi_get_state(hal_spi_context_t* ctx)
{
    if (ctx == NULL) {
        return HAL_SPI_ST_ERROR;
    }

    if (ctx->ops == NULL || ctx->ops->get_state == NULL) {
        return HAL_SPI_ST_ERROR;
    }

    return ctx->ops->get_state(ctx);
}

hal_spi_dma_state_t hal_spi_get_dma_state(hal_spi_context_t* ctx)
{
    if (ctx == NULL) {
        return HAL_SPI_DMA_ST_ERROR;
    }

    if (ctx->ops == NULL || ctx->ops->get_dma_state == NULL) {
        return HAL_SPI_DMA_ST_ERROR;
    }

    return ctx->ops->get_dma_state(ctx);
}
