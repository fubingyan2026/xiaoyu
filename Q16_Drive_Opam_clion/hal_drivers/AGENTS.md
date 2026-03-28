# hal_drivers/

**Generated:** 2026-02-13

STM32外设硬件抽象层。抽象GPIO、UART、FDCAN和PWM。

## 查找位置

| 任务  | 文件            | 说明                |
| ----- | --------------- | ------------------- |
| GPIO  | `hal_gpio.c`    | 引脚配置、读写      |
| UART  | `hal_uart.c`    | 串口通信            |
| FDCAN | `hal_fdcan.c`   | CAN FD总线通信      |
| PWM   | `hal_tim_pwm.c` | 基于定时器的PWM输出 |

## 关键模块

- **hal_gpio.c**: 中断配置、回调注册
- **hal_fdcan.c**: CAN滤波器设置、消息发送/接收
- **hal_tim_pwm.c**: PWM通道设置、死区时间配置
- **stm32_hal_*.c**: STM32 HAL封装实现
