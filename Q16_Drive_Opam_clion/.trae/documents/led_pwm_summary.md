# LED GPIO/PWM 切换问题 - 技术总结文档

<p align='right'>版本: 1.0.0 | 作者: fubingyan | 日期: 2025-04-12</p>

本文档记录 LED 模块在 STM32G431 平台上实现 GPIO 与 PWM 模式无缝切换的技术方案及问题排查过程。

---

## 一、问题描述

LED 模块在从 GPIO 模式（常亮/闪烁）切换回呼吸灯模式时，PWM 输出异常，LED 无法正常呼吸。

---

## 二、根因分析

### 2.1 关键问题

在 `led_fsm_on_entry` 进入呼吸灯模式时，**未设置 CCR 寄存器的初始占空比**，导致引脚切换到 PWM 模式时，定时器使用的是无效的占空比值。

### 2.2 问题演变过程

| 方案 | 描述 | 结果 | 失败原因 |
| :--- | :--- | :--- | :--- |
| 方案一 | 删除占空比设置，简化代码 | **失败** | 切换到 PWM 模式时，CCR 寄存器没有有效值 |
| 方案二 | 使用寄存器级切换 (`MODIFY_REG`) | **失败** | 枚举值强转为指针导致 HardFault |
| 方案三 | 恢复占空比设置 + 封装接口切换 | **成功** | 在切换引脚模式之前先设置占空比 |

---

## 三、STM32G431 平台特性分析

### 3.1 高级定时器 (TIM1/TIM8/TIM15/TIM16/TIM17) 的三大陷阱

1. **HAL 库通道状态锁死**：G4 的 HAL 库引入了严格的 `htim->ChannelState` 管理。如果定时器未正确停止而反复调用 `Start`，HAL 库会返回 `HAL_ERROR`。

2. **MOE 位被清零**：高级定时器带有刹车（Break）功能，`HAL_TIM_PWM_Stop()` 会清除 `BDTR` 寄存器中的 `MOE` 位，导致 PWM 无法输出。

3. **复用功能映射号丢失**：切换引脚模式时，如果未正确配置 `GPIO_InitStruct.Alternate`，引脚会默认切到 AF0，导致 PWM 信号无法输出。

### 3.2 引脚模式切换的瞬时性

STM32 的引脚模式切换是**瞬时**的，没有"缓冲"机制：
- GPIO 模式下，定时器 TIM17 仍在后台运行，CCR 寄存器保持着之前的值（或被初始化为 0）
- 当调用 `hal_tim_pwm_gpio_alternate` 将引脚从 GPIO 模式切换到 PWM 复用模式时，引脚的控制权**立即**从 GPIO 转移到定时器
- 如果在切换瞬间 CCR 寄存器值无效，定时器会输出错误的 PWM 波形

---

## 四、最终解决方案

### 4.1 设计原则：定时器劫持法 (Timer Hijack)

> 定时器初始化后**永远不停止**，只在注册时启动一次。当需要普通常亮/闪烁时，只把引脚的 `MODER` 寄存器强制切成"普通输出"；当需要呼吸灯时，再把引脚切回"复用功能"。

### 4.2 代码实现

#### 4.2.1 LED 注册 (`led_register_static`)

```c
if (instance->config.pwm_cfg.timer_instance) {
  hal_tim_pwm_error_t err_init = hal_tim_pwm_init(&tim_pwm_ctx, &instance->config.pwm_cfg);
  if (err_init != HAL_TIM_PWM_OK) {
    LED_PRINTF("PWM 初始化失败: %d\n", err_init);
    return LED_ERROR_INTERNAL;
  }

  // 只启动一次，之后再也不调用 Stop
  hal_tim_pwm_error_t err_start = hal_tim_pwm_start(
      &tim_pwm_ctx, instance->config.pwm_cfg.timer_instance,
      instance->config.pwm_cfg.channel);
  if (err_start != HAL_TIM_PWM_OK) {
    LED_PRINTF("PWM 启动失败: %d\n", err_start);
    return LED_ERROR_INTERNAL;
  }

  instance->pwm_init_flag = true;
}
```

#### 4.2.2 进入呼吸灯 (`led_fsm_on_entry`)

```c
case LED_STATE_BREATHING:
  handle->breath_value = handle->config.led_refresh_min_duty;
  handle->breath_cycle = 0;
  handle->last_breath_time = now;

  if (handle->pwm_init_flag) {
    // 【关键】顺序很重要：先设置 CCR 寄存器值
    hal_tim_pwm_set_duty_cycle(
        &tim_pwm_ctx, handle->config.pwm_cfg.timer_instance,
        handle->config.pwm_cfg.channel, handle->breath_value);

    // 再切换引脚为 PWM 复用模式
    hal_tim_pwm_gpio_alternate(&tim_pwm_ctx, &handle->config.pwm_cfg.gpio);
  }
  break;
```

#### 4.2.3 退出呼吸灯 (`led_fsm_on_exit`)

```c
case LED_STATE_BREATHING:
  if (handle->pwm_init_flag) {
    // 设置退出后的默认电平（避免切换瞬间产生毛刺）
    hal_gpio_pin_state_t target_state = (hal_gpio_pin_state_t)!handle->config.active_level;
    hal_gpio_write(&gpio_ctx, handle->config.port, handle->config.pin, target_state);

    // 使用封装接口将引脚从复用模式切换回 GPIO 模式
    hal_gpio_config_t gpio_cfg = {
        .port = handle->config.port,
        .pin = handle->config.pin,
        .mode = HAL_GPIO_MODE_OUTPUT_PP,
        .pull = HAL_GPIO_PULL_NONE,
        .speed = HAL_GPIO_SPEED_FREQ_LOW,
        .default_state = target_state
    };
    hal_gpio_init(&gpio_ctx, &gpio_cfg);
  }
  break;
```

---

## 五、关键技术要点

| 序号 | 要点 | 说明 |
| :--- | :--- | :--- |
| 1 | **顺序很重要** | 必须先设置 CCR 寄存器值，再切换引脚模式 |
| 2 | **直接写寄存器** | `hal_tim_pwm_set_duty_cycle` 底层直接写 CCR 寄存器，不受 HAL 状态影响 |
| 3 | **定时器只启动一次** | 避免 HAL 库通道状态锁死问题 |
| 4 | **使用封装接口** | 使用 `hal_gpio_init` 切换 GPIO 模式，避免枚举强转导致 HardFault |
| 5 | **设置默认电平** | 退出呼吸灯前设置目标电平，避免切换瞬间产生毛刺 |

---

## 六、验证结果

- [x] 呼吸灯模式正常切换
- [x] GPIO 模式（常亮/闪烁）正常工作
- [x] 两种模式间切换无毛刺
- [x] 多次切换后仍能正常呼吸

---

## 七、参考文档
- [led.md](../led/led.md) - LED 模块设计文档