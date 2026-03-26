# Q16_FOC/

**Generated:** 2026-02-13

FOC核心模块，包含电机控制算法、SVPWM生成、滑模观测器和状态机。

## 查找位置

| 任务 | 文件 | 说明 |
|------|------|-------|
| FOC控制循环 | `foc_ctrl_q16.c` | Id/Iq的PI控制器 |
| SVPWM | `foc_svpwm_q16.c` | 空间矢量PWM生成 |
| 观测器 | `foc_observer_q16.c` | 滑模观测器 |
| 状态机 | `foc_sm.c` | IDLE→ALIGN→CALIBRATE→RUN→HALL |
| 定点数数学 | `q16_16_math.c` | ISR的Q16格式 |

## 关键模块

- **foc_ctrl_q16.c**: Clark/Park变换、PI控制器、转矩/磁通控制
- **foc_svpwm_q16.c**: 6扇区3相PWM、死区时间补偿
- **foc_observer_q16.c**: 无传感器控制反电动势估算
- **q16_16_math.c**: 避免ISR中浮点的定点数数学
