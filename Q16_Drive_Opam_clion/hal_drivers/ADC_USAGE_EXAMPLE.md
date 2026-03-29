# ADC HAL 层使用示例

本文档演示如何使用 ADC HAL 层进行 DMA 中断方式的模数转换。

## 1. 初始化 ADC 上下文

```c
#include "hal_adc.h"

static hal_adc_context_t adc_ctx;
static uint32_t adc_dma_buffer[4];

void adc_init(void) {
    hal_adc_error_t ret;

    ret = stm32_adc_init_context(&adc_ctx);
    if (ret != HAL_ADC_OK) {
        return;
    }

    hal_adc_config_t config = {
        .instance = HAL_ADC_INSTANCE_1,
        .resolution = HAL_ADC_RESOLUTION_12B,
        .data_align = HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = HAL_ADC_SCAN_MODE_ENABLE,
        .continuous = HAL_ADC_CONTINUOUS_ENABLE,
        .dma_mode = HAL_ADC_DMA_ENABLE,
        .nbr_of_conversion = 4,
        .timeout = 100
    };

    ret = hal_adc_init(&adc_ctx, &config);
    if (ret != HAL_ADC_OK) {
        return;
    }
}
```

## 2. 配置 ADC 通道

```c
void adc_config_channels(void) {
    hal_adc_channel_config_t channel_config;

    channel_config.sample_time = HAL_ADC_SAMPLETIME_47CYCLES_5;

    channel_config.channel = HAL_ADC_CHANNEL_1;
    channel_config.rank = 0;
    hal_adc_config_channel(&adc_ctx, &channel_config);

    channel_config.channel = HAL_ADC_CHANNEL_2;
    channel_config.rank = 1;
    hal_adc_config_channel(&adc_ctx, &channel_config);

    channel_config.channel = HAL_ADC_CHANNEL_3;
    channel_config.rank = 2;
    hal_adc_config_channel(&adc_ctx, &channel_config);

    channel_config.channel = HAL_ADC_CHANNEL_4;
    channel_config.rank = 3;
    hal_adc_config_channel(&adc_ctx, &channel_config);
}
```

## 3. ADC 校准

```c
void adc_calibrate(void) {
    hal_adc_calibration_config_t calib_config = {
        .single_ended = true,
        .differential = false,
        .calibration_count = 1
    };

    hal_adc_calibrate(&adc_ctx, HAL_ADC_INSTANCE_1, &calib_config);
}
```

## 4. 注册 DMA 回调函数

```c
static void adc_dma_callback(hal_adc_context_t* ctx,
                             hal_adc_instance_t instance,
                             uint32_t* buffer,
                             uint32_t length,
                             void* user_data) {
    (void)ctx;
    (void)instance;
    (void)user_data;

    for (uint32_t i = 0; i < length; i++) {
        uint32_t voltage_mv = hal_adc_raw_to_voltage(
            buffer[i],
            HAL_ADC_RESOLUTION_12B,
            3300
        );
    }
}

void adc_register_callbacks(void) {
    hal_adc_register_dma_callback(
        &adc_ctx,
        HAL_ADC_INSTANCE_1,
        adc_dma_callback,
        NULL
    );
}
```

## 5. 启动 DMA 转换

```c
void adc_start_dma_conversion(void) {
    hal_adc_dma_config_t dma_config = {
        .buffer = adc_dma_buffer,
        .buffer_length = 4,
        .circular_mode = true
    };

    hal_adc_start_dma(&adc_ctx, HAL_ADC_INSTANCE_1, &dma_config);
}
```

## 6. 停止 DMA 转换

```c
void adc_stop_dma_conversion(void) {
    hal_adc_stop_dma(&adc_ctx, HAL_ADC_INSTANCE_1);
}
```

## 7. 完整示例

```c
#include "hal_adc.h"

static hal_adc_context_t adc_ctx;
static uint32_t adc_dma_buffer[4];
static volatile bool conversion_complete = false;

static void adc_dma_callback(hal_adc_context_t* ctx,
                             hal_adc_instance_t instance,
                             uint32_t* buffer,
                             uint32_t length,
                             void* user_data) {
    (void)ctx;
    (void)instance;
    (void)user_data;

    conversion_complete = true;

    for (uint32_t i = 0; i < length; i++) {
        uint32_t voltage_mv = hal_adc_raw_to_voltage(
            buffer[i],
            HAL_ADC_RESOLUTION_12B,
            3300
        );
    }
}

void adc_example_init(void) {
    stm32_adc_init_context(&adc_ctx);

    hal_adc_config_t config = {
        .instance = HAL_ADC_INSTANCE_1,
        .resolution = HAL_ADC_RESOLUTION_12B,
        .data_align = HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = HAL_ADC_SCAN_MODE_ENABLE,
        .continuous = HAL_ADC_CONTINUOUS_ENABLE,
        .dma_mode = HAL_ADC_DMA_ENABLE,
        .nbr_of_conversion = 4,
        .timeout = 100
    };
    hal_adc_init(&adc_ctx, &config);

    hal_adc_channel_config_t channel_config = {
        .sample_time = HAL_ADC_SAMPLETIME_47CYCLES_5
    };

    for (uint8_t i = 0; i < 4; i++) {
        channel_config.channel = HAL_ADC_CHANNEL_1 + i;
        channel_config.rank = i;
        hal_adc_config_channel(&adc_ctx, &channel_config);
    }

    hal_adc_calibration_config_t calib_config = {
        .single_ended = true,
        .differential = false,
        .calibration_count = 1
    };
    hal_adc_calibrate(&adc_ctx, HAL_ADC_INSTANCE_1, &calib_config);

    hal_adc_register_dma_callback(
        &adc_ctx,
        HAL_ADC_INSTANCE_1,
        adc_dma_callback,
        NULL
    );

    hal_adc_dma_config_t dma_config = {
        .buffer = adc_dma_buffer,
        .buffer_length = 4,
        .circular_mode = true
    };
    hal_adc_start_dma(&adc_ctx, HAL_ADC_INSTANCE_1, &dma_config);
}

void adc_example_process(void) {
    if (conversion_complete) {
        conversion_complete = false;

    }
}
```

## 8. 阻塞方式单次转换示例

```c
void adc_single_conversion_example(void) {
    hal_adc_context_t adc_ctx;
    uint32_t adc_value;

    stm32_adc_init_context(&adc_ctx);

    hal_adc_config_t config = {
        .instance = HAL_ADC_INSTANCE_1,
        .resolution = HAL_ADC_RESOLUTION_12B,
        .data_align = HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = HAL_ADC_SCAN_MODE_DISABLE,
        .continuous = HAL_ADC_CONTINUOUS_DISABLE,
        .dma_mode = HAL_ADC_DMA_DISABLE,
        .nbr_of_conversion = 1,
        .timeout = 100
    };
    hal_adc_init(&adc_ctx, &config);

    hal_adc_channel_config_t channel_config = {
        .channel = HAL_ADC_CHANNEL_1,
        .sample_time = HAL_ADC_SAMPLETIME_47CYCLES_5,
        .rank = 0
    };
    hal_adc_config_channel(&adc_ctx, &channel_config);

    hal_adc_calibration_config_t calib_config = {
        .single_ended = true,
        .differential = false,
        .calibration_count = 1
    };
    hal_adc_calibrate(&adc_ctx, HAL_ADC_INSTANCE_1, &calib_config);

    hal_adc_error_t ret = hal_adc_start_conversion(
        &adc_ctx,
        HAL_ADC_INSTANCE_1,
        &adc_value
    );

    if (ret == HAL_ADC_OK) {
        uint32_t voltage_mv = hal_adc_raw_to_voltage(
            adc_value,
            HAL_ADC_RESOLUTION_12B,
            3300
        );
    }
}
```

## 9. API 参考

| 函数 | 说明 |
|------|------|
| `stm32_adc_init_context()` | 初始化 STM32 平台 ADC 上下文 |
| `hal_adc_init()` | 初始化 ADC |
| `hal_adc_deinit()` | 反初始化 ADC |
| `hal_adc_config_channel()` | 配置 ADC 通道 |
| `hal_adc_start_conversion()` | 启动阻塞方式转换 |
| `hal_adc_start_conversion_async()` | 启动异步转换 |
| `hal_adc_stop_conversion()` | 停止转换 |
| `hal_adc_start_dma()` | 启动 DMA 转换 |
| `hal_adc_stop_dma()` | 停止 DMA 转换 |
| `hal_adc_get_value()` | 获取转换结果 |
| `hal_adc_calibrate()` | 校准 ADC |
| `hal_adc_register_callback()` | 注册转换完成回调 |
| `hal_adc_register_dma_callback()` | 注册 DMA 完成回调 |
| `hal_adc_raw_to_voltage()` | 原始值转电压值 |
| `hal_adc_get_max_value()` | 获取 ADC 最大值 |
