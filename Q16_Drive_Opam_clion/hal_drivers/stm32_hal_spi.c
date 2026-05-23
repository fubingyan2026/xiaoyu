//
// Created by fubingyan on 25-5-24.
//

/**
 * @file    stm32_hal_spi.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-05-24
 * @brief   STM32平台硬件抽象层 - SPI实现
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
#include "spi.h"

/* Private variables ---------------------------------------------------------*/
static SPI_HandleTypeDef* spi_handle_map[HAL_SPI_INSTANCE_LEN] = {
    &hspi1,
    &hspi3,
};

/* Private function prototypes -----------------------------------------------*/
static hal_spi_error_t stm32_spi_init(hal_spi_context_t* ctx,
    const hal_spi_config_t* config);
static hal_spi_error_t stm32_spi_deinit(hal_spi_context_t* ctx);
static hal_spi_error_t stm32_spi_transmit_receive(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint8_t* rx_data,
    uint16_t size,
    uint32_t timeout);
static hal_spi_error_t stm32_spi_transmit_dma(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint16_t size);
static hal_spi_state_t stm32_spi_get_state(hal_spi_context_t* ctx);
static hal_spi_dma_state_t stm32_spi_get_dma_state(hal_spi_context_t* ctx);

static SPI_HandleTypeDef* get_handle(const hal_spi_context_t* ctx);

/* SPI操作函数结构体 */
static const hal_spi_ops_t stm32_spi_ops = {
    .init = stm32_spi_init,
    .deinit = stm32_spi_deinit,
    .transmit_receive = stm32_spi_transmit_receive,
    .transmit_dma = stm32_spi_transmit_dma,
    .get_state = stm32_spi_get_state,
    .get_dma_state = stm32_spi_get_dma_state,
};

/* Exported functions --------------------------------------------------------*/

hal_spi_error_t stm32_spi_init_context(hal_spi_context_t* ctx)
{
    if (ctx == NULL) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    ctx->ops = &stm32_spi_ops;
    ctx->initialized = 0;

    return HAL_SPI_OK;
}

/* Private functions ---------------------------------------------------------*/

static SPI_HandleTypeDef* get_handle(const hal_spi_context_t* ctx)
{
    if (ctx->instance >= HAL_SPI_INSTANCE_LEN) {
        return NULL;
    }
    return spi_handle_map[ctx->instance];
}

static hal_spi_error_t stm32_spi_init(hal_spi_context_t* ctx,
    const hal_spi_config_t* config)
{
    if (config == NULL || config->instance >= HAL_SPI_INSTANCE_LEN) {
        return HAL_SPI_ERROR_INVALID_PARAM;
    }

    (void)ctx;
    if (spi_handle_map[config->instance] == NULL) {
        return HAL_SPI_ERROR_HARDWARE;
    }

    return HAL_SPI_OK;
}

static hal_spi_error_t stm32_spi_deinit(hal_spi_context_t* ctx)
{
    (void)ctx;
    return HAL_SPI_OK;
}

static hal_spi_error_t stm32_spi_transmit_receive(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint8_t* rx_data,
    uint16_t size,
    uint32_t timeout)
{
    SPI_HandleTypeDef* hspi = get_handle(ctx);
    if (hspi == NULL) {
        return HAL_SPI_ERROR_HARDWARE;
    }

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(hspi, tx_data, rx_data, size, timeout);
    if (status == HAL_OK) {
        return HAL_SPI_OK;
    } else if (status == HAL_BUSY) {
        return HAL_SPI_ERROR_BUSY;
    } else if (status == HAL_TIMEOUT) {
        return HAL_SPI_ERROR_TIMEOUT;
    }
    return HAL_SPI_ERROR_HARDWARE;
}

static hal_spi_error_t stm32_spi_transmit_dma(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint16_t size)
{
    SPI_HandleTypeDef* hspi = get_handle(ctx);
    if (hspi == NULL) {
        return HAL_SPI_ERROR_HARDWARE;
    }

    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(hspi, tx_data, size);
    if (status == HAL_OK) {
        return HAL_SPI_OK;
    } else if (status == HAL_BUSY) {
        return HAL_SPI_ERROR_BUSY;
    }
    return HAL_SPI_ERROR_HARDWARE;
}

static hal_spi_state_t stm32_spi_get_state(hal_spi_context_t* ctx)
{
    SPI_HandleTypeDef* hspi = get_handle(ctx);
    if (hspi == NULL) {
        return HAL_SPI_ST_ERROR;
    }

    HAL_SPI_StateTypeDef state = HAL_SPI_GetState(hspi);
    if (state == HAL_SPI_STATE_READY) {
        return HAL_SPI_ST_READY;
    } else if (state == HAL_SPI_STATE_BUSY || state == HAL_SPI_STATE_BUSY_TX || state == HAL_SPI_STATE_BUSY_RX || state == HAL_SPI_STATE_BUSY_TX_RX) {
        return HAL_SPI_ST_BUSY;
    }
    return HAL_SPI_ST_ERROR;
}

static hal_spi_dma_state_t stm32_spi_get_dma_state(hal_spi_context_t* ctx)
{
    SPI_HandleTypeDef* hspi = get_handle(ctx);
    if (hspi == NULL || hspi->hdmatx == NULL) {
        return HAL_SPI_DMA_ST_ERROR;
    }

    if (hspi->hdmatx->State == HAL_DMA_STATE_READY) {
        return HAL_SPI_DMA_ST_READY;
    } else if (hspi->hdmatx->State == HAL_DMA_STATE_BUSY) {
        return HAL_SPI_DMA_ST_BUSY;
    }
    return HAL_SPI_DMA_ST_ERROR;
}
