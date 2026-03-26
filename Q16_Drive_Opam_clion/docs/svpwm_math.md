# SVPWM 数学原理详解

## 1. 概述

SVPWM（Space Vector Pulse Width Modulation，空间矢量脉宽调制）是一种用于三相逆变器的调制技术。本文档详细介绍两种SVPWM实现方法的数学原理：

- **扇区法**（Sector-based SVPWM）
- **三次谐波注入法**（3rd Harmonic Injection）

---

## 2. Clark变换与Park变换

### 2.1 Clark变换（3→2）

将三相静止坐标系(a, b, c)转换到两相静止坐标系(α, β)：

$
\begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix} = \frac{2}{3} \begin{bmatrix} 1 & -\frac{1}{2} & -\frac{1}{2} \\ 0 & \frac{\sqrt{3}}{2} & -\frac{\sqrt{3}}{2} \end{bmatrix} \begin{bmatrix} V_a \\ V_b \\ V_c \end{bmatrix}
$

逆Clark变换（2→3，标准形式）：

$
\begin{bmatrix} V_a \\ V_b \\ V_c \end{bmatrix} = \begin{bmatrix} 1 & 0 \\ -\frac{1}{2} & \frac{\sqrt{3}}{2} \\ -\frac{1}{2} & -\frac{\sqrt{3}}{2} \end{bmatrix} \begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix}
$

### 2.2 **扇区法中的A-B-C变换**

扇区法用于扇区判断的变换**不是**逆Clark变换，而是**abc到A-B-C坐标系的变换**：

#### 旋转矩阵推导

旋转θ角的2D旋转矩阵：

$
R(\theta) = \begin{bmatrix} \cos\theta & -\sin\theta \\ \sin\theta & \cos\theta \end{bmatrix}
$

将α-β坐标系旋转+30°：

$
R(+30^\circ) = \begin{bmatrix} \frac{\sqrt{3}}{2} & -\frac{1}{2} \\ \frac{1}{2} & \frac{\sqrt{3}}{2} \end{bmatrix}
$

变换后的坐标 $(V_{rot\alpha}, V_{rot\beta})$：

$
\begin{bmatrix} V_{rot\alpha} \\ V_{rot\beta} \end{bmatrix} = \begin{bmatrix} \frac{\sqrt{3}}{2} & -\frac{1}{2} \\ \frac{1}{2} & \frac{\sqrt{3}}{2} \end{bmatrix} \begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix}
$

#### A-B-C轴的定义

**A轴**（旋转后β轴，垂直向上）：

$
A = V_{rot\beta} = \frac{1}{2}V_\alpha + \frac{\sqrt{3}}{2}V_\beta
$

**B轴**（旋转后α轴逆时针旋转90°，即θ+120°）：

$
B = |V_{ref}| \cdot \sin(\theta + 120^\circ)
= -\frac{1}{2}V_\beta + \frac{\sqrt{3}}{2}V_\alpha
$

**C轴**（旋转后α轴顺时针旋转90°，即θ+240°）：

$
C = |V_{ref}| \cdot \sin(\theta + 240^\circ)
= -\frac{1}{2}V_\beta - \frac{\sqrt{3}}{2}V_\alpha
$

#### 最终变换公式

$
\boxed{
\begin{aligned}
A &= V_\beta \\
B &= \frac{\sqrt{3}}{2}V_\alpha - \frac{1}{2}V_\beta \\
C &= -\frac{\sqrt{3}}{2}V_\alpha - \frac{1}{2}V_\beta
\end{aligned}}
$

#### 几何解释

```
C轴 (210°)
    ↑
   /|
  / | 
 /  |  
B轴 ← α轴(0°) → A轴(90°)
(150°)    |
         |
         ↓
       β轴
```

- **A轴**：90°方向，投影 = Vβ
- **B轴**：150°方向，投影 = √3/2·Vα - 1/2·Vβ
- **C轴**：210°方向，投影 = -√3/2·Vα - 1/2·Vβ

### 2.3 扇区判断原理

通过A、B、C的符号组合（共8种）唯一确定参考电压矢量所在的扇区：

| A > 0 | B > 0 | C > 0 | 扇区 |
|-------|-------|-------|------|
| 0 | 0 | 0 | 无效 |
| 0 | 0 | 1 | 1 |
| 0 | 1 | 0 | 2 |
| 0 | 1 | 1 | 3 |
| 1 | 0 | 0 | 4 |
| 1 | 0 | 1 | 5 |
| 1 | 1 | 0 | 6 |
| 1 | 1 | 1 | 无效 |

**原理**：将α-β坐标系旋转30°，使6个扇区的边界与A、B、C轴对齐，形成对称的符号判断系统。

### 2.4 Park变换（2→2）

将两相静止坐标系(α, β)转换到旋转坐标系(d, q)：

$
\begin{bmatrix} V_d \\ V_q \end{bmatrix} = \begin{bmatrix} \cos\theta & \sin\theta \\ -\sin\theta & \cos\theta \end{bmatrix} \begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix}
$

逆Park变换：

$
\begin{bmatrix} V_\alpha \\ V_\beta \end{bmatrix} = \begin{bmatrix} \cos\theta & -\sin\theta \\ \sin\theta & \cos\theta \end{bmatrix} \begin{bmatrix} V_d \\ V_q \end{bmatrix}
$

---

## 3. 扇区法SVPWM

### 3.1 基本电压矢量

三相逆变器有8种开关状态（2³ = 8），其中6个非零矢量（V₁~V₆）和2个零矢量（V₀, V₇）：

| 开关状态 | 矢量 | 角度 |
|---------|------|------|
| 100 | V₁ | 0° |
| 110 | V₂ | 60° |
| 010 | V₃ | 120° |
| 011 | V₄ | 180° |
| 001 | V₅ | 240° |
| 110 | V₆ | 300° |
| 000/111 | V₀/V₇ | - |

### 3.2 扇区判断

通过A、B、C的符号组合（共8种）唯一确定参考电压矢量所在的扇区：

| A > 0 | B > 0 | C > 0 | 扇区 |
|-------|-------|-------|------|
| 0 | 0 | 0 | 无效 |
| 0 | 0 | 1 | 1 |
| 0 | 1 | 0 | 2 |
| 0 | 1 | 1 | 3 |
| 1 | 0 | 0 | 4 |
| 1 | 0 | 1 | 5 |
| 1 | 1 | 0 | 6 |
| 1 | 1 | 1 | 无效 |

**原理**：将α-β坐标系旋转30°，使6个扇区的边界与A、B、C轴对齐，形成对称的符号判断系统。

### 3.3 作用时间计算

以扇区1为例（参考矢量在V₁和V₂之间）：

$$
\begin{aligned}
T_1 &= \frac{\sqrt{3} \cdot T_s}{V_{dc}} \cdot (V_\alpha - \frac{V_\beta}{\sqrt{3}}) \\
T_2 &= \frac{\sqrt{3} \cdot T_s}{V_{dc}} \cdot V_\beta \\
T_0 &= T_s - T_1 - T_2
\end{aligned}
$$

其中 $T_s$ 为PWM周期，$V_{dc}$ 为母线电压。

### 3.4 占空比计算

三相占空比：

$$
\begin{aligned}
D_a &= \frac{T_0}{2T_s} + \frac{T_1}{T_s} \\
D_b &= \frac{T_0}{2T_s} + \frac{T_2}{T_s} + \frac{T_1}{T_s} \\
D_c &= \frac{T_0}{2T_s} + \frac{T_2}{T_s}
\end{aligned}
$$

### 3.5 扇区法代码映射

各扇区的 `(ta, tb, tc)` 计算：

| 扇区 | tx | ty | ta | tb | tc |
|------|-----|-----|-----|-----|-----|
| 1 | tα - tβ | 2tβ | t₀/₂ | t₀/₂ + tx | tb + ty |
| 2 | tβ - tα | tα + tβ | t₀/₂ + tx | t₀/₂ | ta + ty |
| 3 | 2tβ | -(tα + tβ) | t₀/₂ + ty | t₀/₂ | t₀/₂ + tx + ty |
| 4 | -(2tβ) | tβ - tα | t₀/₂ + tx + ty | t₀/₂ + tx | t₀/₂ |
| 5 | -(tα + tβ) | tα - tβ | t₀/₂ + tx + ty | t₀/₂ + ty | t₀/₂ + tx |
| 6 | tα + tβ | -(2tβ) | t₀/₂ | t₀/₂ + tx + ty | t₀/₂ + tx |

---

## 4. 三次谐波注入法

### 4.1 原理

在基波电压上注入三次谐波，形成"马鞍波"：

$$
\begin{aligned}
V_a &= V_1 \sin(\theta) + V_3 \sin(3\theta) \\
V_b &= V_1 \sin(\theta - 120^\circ) + V_3 \sin(3(\theta - 120^\circ)) \\
V_c &= V_1 \sin(\theta - 240^\circ) + V_3 \sin(3(\theta - 240^\circ))
\end{aligned}
$$

### 4.2 注入比优化

最优三次谐波注入比约为基波幅值的15.47%：

$$
V_3 = \frac{V_1}{4} \quad (\text{最优注入})
$$

### 4.3 马鞍波特性

```
相电压波形：
      /\      /\
     /  \    /  \
----/----\/----\/----  ← 马鞍形
   /      /\      \
  /      /  \      \
```

### 4.4 优势

1. **线性调制范围扩展**：从 $V_{dc}/\sqrt{3}$ 扩展到 $V_{dc} \cdot \sqrt{3}/2$（约15%提升）
2. **计算量小**：不需要扇区判断
3. **开关损耗均衡**

### 4.5 算法步骤

**步骤1**：归一化电压计算

标准逆Clark变换：
$
\begin{aligned}
V_a &= V_\alpha \\
V_b &= -\frac{1}{2}V_\alpha + \frac{\sqrt{3}}{2}V_\beta \\
V_c &= -\frac{1}{2}V_\alpha - \frac{\sqrt{3}}{2}V_\beta
\end{aligned}
$

**代码中三次谐波法的定义**：
$
\begin{aligned}
v_a &= V_\alpha \\
v_b &= -\frac{1}{2}V_\alpha + \frac{\sqrt{3}}{2}V_\beta \\
v_c &= -\frac{1}{2}V_\alpha - \frac{\sqrt{3}}{2}V_\beta
\end{aligned}
$

### 4.6 三次谐波法与标准abc坐标系的差异

#### 标准abc坐标系的扇区判断

标准abc坐标系中，根据Va、Vb、Vc的符号和最大值判断扇区：

| 扇区 | Va | Vb | Vc | 最大值 |
|------|-----|-----|-----|-------|
| 1 | >0 | <0 | <0 | Va |
| 2 | <0 | >0 | <0 | Vb |
| 3 | <0 | >0 | <0 | Vb |
| 4 | <0 | <0 | >0 | Vc |
| 5 | <0 | <0 | >0 | Vc |
| 6 | >0 | <0 | <0 | Va |

#### 三次谐波法的扇区判断

三次谐波法中，va、vb、vc的定义相同，但实际值不同：

| 扇区 | va | vb | vc | 最大值 |
|------|-----|-----|-----|-------|
| 1 | >0 | <0 | <0 | va |
| 2 | <0 | >0 | <0 | vb |
| 3 | <0 | >0 | <0 | vb |
| 4 | <0 | <0 | >0 | vc |
| 5 | <0 | <0 | >0 | vc |
| 6 | >0 | <0 | <0 | va |

#### 关键差异：va与Va的物理意义

**标准abc坐标系**（基于sin函数）：
- Va = |V|·sin(θ)，Va在θ=90°时最大
- 扇区1范围：θ ∈ [30°, 90°]

**三次谐波法**（基于cos函数）：
- va = |V|·cos(θ-30°)，va在θ=30°时最大
- 扇区1范围：θ ∈ [30°, 90°]

**虽然扇区判断结果相同，但占空比输出不同**：

在扇区1中：
- 标准abc：Va > Vb > Vc（占空比Da > Db > Dc）
- 三次谐波：va > vb > vc（占空比da > db > dc）

但扇区法中，扇区1的占空比是tc > tb > ta！

#### 占空比输出对比

**扇区法**（扇区1）：
- 输出顺序：tc > tb > ta
- 对应：Vc > Vb > Va

**三次谐波法**（扇区1）：
- 输出顺序：ta > tb > tc
- 对应：va > vb > vc

**因此需要交换输出**：
```c
this->tc = va_norm;  // tc = va
this->ta = vb_norm;  // ta = vb
this->tb = vc_norm;  // tb = vc
```

#### 总结

| 方法 | 坐标定义 | 占空比输出顺序 | 是否需要交换 |
|------|---------|---------------|-------------|
| 扇区法 | 隐含在扇区计算中 | tc > tb > ta（C-A-B） | 否 |
| 三次谐波 | va, vb, vc | ta > tb > tc（A-B-C） | 是 |

**步骤2**：找到最大最小值

$$
\begin{aligned}
V_{min} &= \min(V_a, V_b, V_c) \\
V_{max} &= \max(V_a, V_b, V_c)
\end{aligned}
$$

**步骤3**：计算偏移量（等效三次谐波注入）

$$
V_{offset} = \frac{V_{min} + V_{max}}{2}
$$

**步骤4**：中心化并限幅

$$
\begin{aligned}
D_a &= 0.5 + V_a - V_{offset} \\
D_b &= 0.5 + V_b - V_{offset} \\
D_c &= 0.5 + V_c - V_{offset}
\end{aligned}
$$

### 4.6 相序问题

**注意**：由于逆Clark变换的相位定义与扇区法不同，需要调整输出映射：

```c
// 代码中的实际映射
tc = va_norm;  // C相 = Va
ta = vb_norm;  // A相 = Vb  
tb = vc_norm;  // B相 = Vc
```

这是因为扇区法和三次谐波法中相位的物理对应关系不同。

### 4.7 为什么需要交换输出顺序？

#### 核心原因：电压空间矢量的旋转方向

**FOC控制链**：
```
d-q坐标 → Park逆变换 → α-β坐标 → 逆Clark → 三相电压(Va, Vb, Vc)
```

扇区法和三次谐波法在**最后一步逆Clark变换**的定义上存在差异。

#### 三次谐波法的相序问题

代码中的三次谐波法计算：
$
\begin{aligned}
v_a &= V_\alpha \\
v_b &= -\frac{1}{2}V_\alpha + \frac{\sqrt{3}}{2}V_\beta \\
v_c &= -\frac{1}{2}V_\alpha - \frac{\sqrt{3}}{2}V_\beta
\end{aligned}
$

如果直接输出 `(ta=va, tb=vb, tc=vc)`，相当于：
- A相 = Vα
- B相 = -1/2·Vα + √3/2·Vβ
- C相 = -1/2·Vα - √3/2·Vβ

这对应的是 **A-B-C** 相序，而扇区法对应的是 **C-A-B** 相序。

#### 相序差异的影响

FOC算法假设的磁场旋转方向与实际输出的相序不匹配时：
- **扇区法**：输出相序与FOC假设一致，直接工作
- **三次谐波法**：输出相序旋转了120°，需要交换映射

#### 为什么是120°旋转？

三次谐波法中 `(va, vb, vc)` 的定义是标准abc坐标系的**倒序**：
- 标准abc：Va, Vb, Vc 间隔120°
- 三次谐波：va, vb, vc 间隔-120°（或240°）

因此需要交换输出：
```c
tc = va_norm;  // C = Va
ta = vb_norm;  // A = Vb
tb = vc_norm;  // B = Vc
```

#### 总结

| 方法 | 逆Clark变换定义 | 输出相序 | 是否需要交换 |
|------|---------------|---------|-------------|
| 扇区法 | 隐含在扇区计算中 | C-A-B | 否 |
| 三次谐波法 | va, vb, vc | A-B-C | 是 |

---

## 5. 两种方法对比

| 特性 | 扇区法 | 三次谐波法 |
|------|--------|-----------|
| 调制比 | 0.577 | 0.666 |
| 计算复杂度 | 高（扇区判断） | 低（无需扇区） |
| 线性范围 | 内切圆 | 外切圆 |
| 谐波含量 | 较低 | 略高 |
| 开关损耗 | 不均衡 | 均衡 |
| 相位精度 | 高 | 取决于注入比 |

---

## 6. 代码实现关键点

### 6.1 Q16.16定点数

使用Q16.16格式实现定点运算，避免浮点运算的实时性问题：

```c
#define Q16_16_ONE       (1 << 16)           // 1.0
#define Q16_16_HALF      (1 << 15)           // 0.5
#define Q16_16_SQRT3_2   FLOAT_TO_Q16_16(0.8660254f)
#define Q16_16_INV_SQRT3 FLOAT_TO_Q16_16(0.5773503f)
```

### 6.2 过调制处理

当电压矢量超出线性调制范围时，进行幅度缩放：

```c
if (|V_ref| > V_max) {
    scale = V_max / |V_ref|
    V_ref = V_ref * scale
}
```

## 7. PWM输出与ADC采样同步

### 7.1 硬件配置

本工程使用STM32G431的定时器1（TIM1）实现PWM输出和ADC同步采样：

| 外设 | 用途 | 配置说明 |
|------|------|---------|
| TIM1_CH1 | A相PWM上管 | 互补输出 |
| TIM1_CH1N | A相PWM下管 | 互补输出 |
| TIM1_CH2 | B相PWM上管 | 互补输出 |
| TIM1_CH2N | B相PWM下管 | 互补输出 |
| TIM1_CH3 | C相PWM上管 | 互补输出 |
| TIM1_CH3N | C相PWM下管 | 互补输出 |
| TIM1_CH4 | ADC同步触发 | 设置比较值触发ADC |
| ADC1 | 电流采样 | 注入模式 + DMA |

### 7.2 PWM与ADC同步机制

```
PWM周期 (Ts)
┌─────────────────────────────────────────────────┐
│                                                 │
│   PWM更新 ──────────────────────────────► PWM输出 │
│                                                 │
│         ▲                               ▲       │
│         │                               │       │
│         │      ADC采样点                │       │
│         │      (Td时刻)                │       │
│         │                               │       │
└─────────────────────────────────────────────────┘

     Td = max(ta, tb, tc)  →  三相占空比的最小值时刻
```

**同步原理**：
1. 在PWM周期的**中心位置**（Td时刻）进行ADC采样
2. 此时三相PWM都处于**导通状态**，可以采样三相电流
3. Td是三相占空比的**最小值**，确保所有下管都导通

### 7.3 代码实现

**SVPWM中计算Td**：
```c
// 计算最大占空比（用于ADC同步采样）
this->td = q16_16_max(this->ta, q16_16_max(this->tb, this->tc));
```

**PWM输出回调**：
```c
static void stm32_pwm_output(uint32_t ta, uint32_t tb, uint32_t tc, uint32_t td)
{
    // 设置三相PWM占空比
    __HAL_TIM_SetCompare(&PWM_INSTANCE, TIM_CHANNEL_1, ta);  // A相
    __HAL_TIM_SetCompare(&PWM_INSTANCE, TIM_CHANNEL_2, tb);  // B相
    __HAL_TIM_SetCompare(&PWM_INSTANCE, TIM_CHANNEL_3, tc);  // C相
    
    // 设置同步触发点（使用PWM_PERIOD-1确保在中心位置之后触发）
    __HAL_TIM_SetCompare(&PWM_INSTANCE, TIM_CHANNEL_4, PWM_PERIOD - 1);
}
```

**ADC采样回调**：
```c
static void stm32_adc_read(float *current_a, float *current_b, float *current_c)
{
    uint32_t adc_a = current_dma_value[0];  // ADC通道1 → A相电流
    uint32_t adc_b = current_dma_value[1];  // ADC通道2 → B相电流
    
    // 去除偏置并应用缩放因子
    *current_a = (float)(adc_a - current_adc_init_buff[0]) * CURRENT_SAMPLE_FACTOR;
    *current_b = (float)(adc_b - current_adc_init_buff[1]) * CURRENT_SAMPLE_FACTOR;
    
    // 根据基尔霍夫电流定律：C相电流 = -(A相 + B相)
    *current_c = -(*current_a + *current_b);
}
```

### 7.4 PWM通道与电机相序对应

| PWM通道 | 硬件连接 | FOC相序 | 说明 |
|--------|---------|---------|------|
| TIM1_CH1 | U相上管 | A相 | 互补输出CH1N |
| TIM1_CH2 | V相上管 | B相 | 互补输出CH2N |
| TIM1_CH3 | W相上管 | C相 | 互补输出CH3N |

**占空比映射**：
```
SVPWM输出          →          PWM定时器
─────────────────────────────────────
ta (A相占空比)     →    TIM1_CH1 比较值
tb (B相占空比)     →    TIM1_CH2 比较值
tc (C相占空比)     →    TIM1_CH3 比较值
```

### 7.5 时序图

```
PWM周期
  │        ┌─────────────────────────────┐
CH1─┐      │         ┌─────────┐         │      ┌──
    │      │         │         │         │      │
    └───┐  │         │         │         │  ┌───┘
        │  │         │         │         │  │
        └──┴─────────┴─────────┴─────────┴──┘
           ▲         ▲         ▲         ▲
           │         │         │         │
           │         │         │         └─ PWM周期结束
           │         │         │
           │         │         └───────────── ADC采样点（Td）
           │         │
           │         └─────────────────────── PWM更新点
           │
           └──────────────────────────────── PWM周期开始

时间顺序：
1. PWM周期开始，CNT从0计数
2. 比较匹配时翻转PWM输出
3. 到达Td时触发ADC采样
4. DMA传输完成中断
5. FOC计算读取ADC数据
6. 计算新的占空比
7. PWM更新点更新占空比寄存器
```

### 7.6 采样时刻选择

**为何选择Td（三相占空比最大值）？**

| 时刻 | A相 | B相 | C相 | 是否可采样 |
|------|-----|-----|-----|-----------|
| 任意时刻 | 可能关闭 | 可能关闭 | 可能关闭 | 否 |
| Td时刻 | 导通 | 导通 | 导通 | **是** |

在Td时刻，三相PWM都处于导通状态（占空比最大的相也导通），此时：
- 采样电阻上有电流流过
- 可以采样三相电流
- 满足基尔霍夫电流定律：Ia + Ib + Ic = 0

### 7.7 上管采样 vs 下管采样

#### 采样电阻位置

```
下管采样电路：
         ┌──┬────────────────────── Vbus
         │  │
        ┌┴─┴┐
        │HS  │  上管（High Side）
        │MOS │
        └─┬─┘
          │
          │◄── 相电压输出 (U)
          │
        ┌─┴─┐
        │LS  │  下管（Low Side）
        │MOS │
        └──┬┘
          │
          └─ GND
          │
        ┌──┴──┐
        │ Rs  │  采样电阻
        └─────┘
          │
         GND

上管采样电路：
         ┌──┬────────────────────── Vbus
         │  │
        ┌┴─┴┐
        │HS  │  上管（High Side）
        │MOS │
        └──┬─┘
          │
          │◄── 相电压输出 (U)
          │
          │
        ┌──┴──┐
        │ Rs  │  采样电阻
        └─────┘
          │
        ┌─┴─┐
        │LS  │  下管（Low Side）
        │MOS │
        └──┬─┘
          │
         GND
```

#### 对比分析

| 特性 | 下管采样 | 上管采样 |
|------|---------|---------|
| **采样电阻电压** | 0 ~ GND | 0 ~ Vbus |
| **运放需求** | 单电源运放即可 | 需要高压运放或电平移位 |
| **共模电压** | 低（~0V） | 高（~Vbus/2） |
| **采样电阻功耗** | 低 | 高 |
| **驱动复杂度** | 低 | 高（需隔离/电平移位） |
**PWM噪声** | 低（关断时电压摆幅小） | 高（开关瞬间电压摆幅大） |
| **死区时间影响** | 小 | 大 |

#### 本工程采用：下管采样

本工程使用下管采样（如上图所示），原因：
1. **共模电压低** - 采样电阻靠近GND，共模电压约为0V
2. **运放简单** - 使用单电源运放（如OPAMP）即可
3. **噪声小** - 下管关断时电压摆幅小
4. **成本低** - 不需要高压运放

#### 采样时机与PWM状态的关系

```
PWM波形（下管采样）：

占空比 < 50%：
CHx (上管)    ─────┐      ┌──────────────────
                  │      │
CHxN (下管)   ─────┘      └──────────────────
                    │▲    │
                    ││    └ 采样区域（电流流过采样电阻）
                    │▼    │

占空比 > 50%：
CHx (上管)    ───────────────────┐      ┌────
                                  │      │
CHxN (下管)   ───────────────────┘      └────
                              │▲      │
                              ││      └ 采样区域
                              │▼      │

**关键点**：无论占空比大小，只要下管导通，电流就流过采样电阻
```

#### 电流方向与符号

```
电机相电流方向约定：

    Ia > 0：电流从驱动板流向电机
    Ia < 0：电流从电机流向驱动板

下管采样电阻电压：
    V_Rs = I_phase × Rs

如果电流从电机流向驱动板（Ia < 0）：
    下管导通时，电流从GND→Rs→下管→电机
    V_Rs 为负值

ADC采样值需要根据实际电路极性进行符号校正。

---

## 8. 参考文献

1. H. W. van der Broeck, H. C. Skudelny, "Analysis and Realization of a Pulsewidth Modulator Based on Voltage Space Vectors", IEEE Transactions on Industry Applications, 1988
2. J. Holtz, "Pulsewidth Modulation for Electronic Power Conversion", Proceedings of the IEEE, 1994
