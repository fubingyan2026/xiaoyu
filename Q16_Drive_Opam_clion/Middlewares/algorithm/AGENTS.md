# modules/algorithm/

在模块间共享的数学工具、滤波器和位置计算。为嵌入式优化的定点数。

## 查找位置

| 任务 | 文件 | 说明 |
|------|------|-------|
| 数学函数 | `maths.c` | 三角函数、平方根、归一化 |
| 滤波器 | `filter.c` | 低通滤波、移动平均 |
| 工具函数 | `utils.c` | 通用辅助函数 |
| 关节位置 | `joint_pos.c` | 运动学 |

## 关键模块

- **filter.c**: 传感器数据的指数移动平均、PT1滤波器
- **maths.c**: 定点数sin/cos、atan2、normalize_angle
- **joint_pos.c**: 轮腿机器人的正向/反向运动学
- **user_lib.c**: 项目特定数学封装
