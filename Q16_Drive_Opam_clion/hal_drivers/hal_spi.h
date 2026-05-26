//
// Created by fubingyan on 25-5-24.
//

#ifndef __HAL_SPI_H
#define __HAL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief SPI 操作错误码枚举
 */
typedef enum {
    HAL_SPI_OK = 0, /**< 操作成功 */
    HAL_SPI_ERROR_INVALID_PARAM, /**< 无效参数 */
    HAL_SPI_ERROR_UNINITIALIZED, /**< 未初始化 */
    HAL_SPI_ERROR_HARDWARE, /**< 硬件错误 */
    HAL_SPI_ERROR_BUSY, /**< 设备忙 */
    HAL_SPI_ERROR_TIMEOUT, /**< 操作超时 */
} hal_spi_error_t;

/**
 * @brief SPI 实例枚举
 */
typedef enum __attribute__((packed)) {
    HAL_SPI_INSTANCE_1 = 0, /**< SPI1 */
    HAL_SPI_INSTANCE_3, /**< SPI3 */
    HAL_SPI_INSTANCE_LEN, /**< 实例数量 */
} hal_spi_instance_t;

/**
 * @brief SPI 状态枚举
 */
typedef enum __attribute__((packed)) {
    HAL_SPI_ST_READY = 0, /**< 就绪 */
    HAL_SPI_ST_BUSY, /**< 忙 */
    HAL_SPI_ST_ERROR, /**< 错误 */
} hal_spi_state_t;

/**
 * @brief SPI DMA 状态枚举
 */
typedef enum __attribute__((packed)) {
    HAL_SPI_DMA_ST_READY = 0, /**< DMA 就绪 */
    HAL_SPI_DMA_ST_BUSY, /**< DMA 忙 */
    HAL_SPI_DMA_ST_ERROR, /**< DMA 错误 */
} hal_spi_dma_state_t;

/**
 * @brief SPI 配置结构体
 */
typedef struct {
    hal_spi_instance_t instance; /**< SPI 实例 */
} hal_spi_config_t;

/**
 * @brief SPI 上下文结构体前向声明
 */
typedef struct hal_spi_context hal_spi_context_t;

/**
 * @brief SPI 操作函数结构体
 */
typedef struct hal_spi_ops {
    hal_spi_error_t (*init)(hal_spi_context_t* ctx,
        const hal_spi_config_t* config);
    hal_spi_error_t (*deinit)(hal_spi_context_t* ctx);
    hal_spi_error_t (*transmit_receive)(hal_spi_context_t* ctx,
        const uint8_t* tx_data, uint8_t* rx_data,
        uint16_t size, uint32_t timeout);
    hal_spi_error_t (*transmit_dma)(hal_spi_context_t* ctx,
        const uint8_t* tx_data, uint16_t size);
    hal_spi_state_t (*get_state)(hal_spi_context_t* ctx);
    hal_spi_dma_state_t (*get_dma_state)(hal_spi_context_t* ctx);
} hal_spi_ops_t;

/**
 * @brief SPI 上下文结构体
 */
struct hal_spi_context {
    const struct hal_spi_ops* ops; /**< 平台特定的操作函数指针 */
    volatile uint8_t initialized; /**< 初始化标志（0=未初始化，1=已初始化） */
    hal_spi_instance_t instance; /**< 当前使用的 SPI 实例 */
};

/* Exported macro ------------------------------------------------------------*/

/**
 * @brief 进入临界区宏
 */
#define HAL_SPI_ENTER_CRITICAL() \
    do {                         \
    } while (0)

/**
 * @brief 退出临界区宏
 */
#define HAL_SPI_EXIT_CRITICAL() \
    do {                        \
    } while (0)

/* Exported functions prototypes ---------------------------------------------*/

hal_spi_error_t hal_spi_init(hal_spi_context_t* ctx,
    const hal_spi_config_t* config);
hal_spi_error_t hal_spi_deinit(hal_spi_context_t* ctx);
hal_spi_error_t hal_spi_transmit_receive(hal_spi_context_t* ctx,
    const uint8_t* tx_data,
    uint8_t* rx_data, uint16_t size,
    uint32_t timeout);
hal_spi_error_t hal_spi_transmit_dma(hal_spi_context_t* ctx,
    const uint8_t* tx_data, uint16_t size);
hal_spi_state_t hal_spi_get_state(hal_spi_context_t* ctx);
hal_spi_dma_state_t hal_spi_get_dma_state(hal_spi_context_t* ctx);

void hal_spi_register_platform_ops(const hal_spi_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_SPI_H */
