# AUTOSAR 嵌入式软件架构设计原理文档

> **版本**: v3.0.0 | **状态**: 正式发布 | **适用标准**: AUTOSAR Classic Platform R21-11  
> **适用范围**: 车载 ECU 软件开发 · 底层驱动 · 通信栈 · 功能安全  
> **安全等级**: ISO 26262 ASIL-B ~ ASIL-D | **维护团队**: 基础软件平台组

---

## 目录

1. [系统总览](#1-系统总览)
2. [AUTOSAR 分层架构](#2-autosar-分层架构)
3. [OS 任务调度机制](#3-os-任务调度机制)
4. [RTE 通信原理](#4-rte-通信原理)
5. [通信栈协议流程](#5-通信栈协议流程)
6. [诊断服务架构](#6-诊断服务架构)
7. [非易失性存储管理](#7-非易失性存储管理)
8. [看门狗与监控机制](#8-看门狗与监控机制)
9. [ECU 状态机模型](#9-ecu-状态机模型)
10. [实时性分析与调度模型](#10-实时性分析与调度模型)
11. [功能安全机制](#11-功能安全机制)
12. [内存保护与分区](#12-内存保护与分区)
13. [附录：术语表](#13-附录术语表)

---

## 1. 系统总览

AUTOSAR（**AUT**omotive **O**pen **S**ystem **AR**chitecture）Classic Platform 是面向资源受限 MCU 的车载嵌入式软件标准架构。其核心设计目标是：

- **可移植性**：软件组件（SWC）与硬件解耦，通过 RTE 抽象通信
- **可复用性**：BSW 模块标准化，跨 ECU 复用
- **功能安全**：支持 ISO 26262 ASIL-A 至 ASIL-D 混合关键性
- **实时确定性**：基于优先级抢占的静态调度，WCET 可分析

### 1.1 AUTOSAR 软件分层总览

```
┌─────────────────────────────────────────────────────────────────┐
│               应用层 Application Layer                           │
│      SWC_A  │  SWC_B  │  SWC_C  │  SWC_D (软件组件)             │
├─────────────────────────────────────────────────────────────────┤
│               运行时环境 RTE (Runtime Environment)               │
│    Port · Runnable · S/R Interface · C/S Interface              │
├────────────────┬──────────────────┬─────────────────────────────┤
│  服务层        │  ECU 抽象层       │  复杂驱动层                  │
│  Services      │  ECU Abstraction  │  Complex Drivers (CDD)      │
│  OS·WDG·NvM   │  IoHwAb·PduR      │  直接访问硬件                │
├────────────────┴──────────────────┴─────────────────────────────┤
│               微控制器抽象层 MCAL                                 │
│   GPT · ADC · PWM · SPI · CAN · LIN · FlexRay · ETH            │
├─────────────────────────────────────────────────────────────────┤
│               微控制器硬件 MCU / Hardware                         │
│   Cortex-M · TriCore · RH850 · S32K  +  外设寄存器              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. AUTOSAR 分层架构

### 2.1 完整模块依赖图

```mermaid
graph TB
    subgraph APP["应用层 (Application)"]
        SWC1[SWC: 车速控制]
        SWC2[SWC: 故障诊断]
        SWC3[SWC: 热管理]
    end

    subgraph RTE["运行时环境 (RTE)"]
        RTE_CORE[RTE Core\nRunnable 调度·端口映射]
    end

    subgraph SERVICES["服务层 (Services)"]
        OS[AUTOSAR OS\nTask·ISR·Alarm·Schedule Table]
        NVM[NvM Manager\n非易失存储管理]
        WDG[WdgM\n看门狗管理]
        DCM[DCM\n诊断通信管理]
        DEM[DEM\n诊断事件管理]
        COM[COM\n通信管理]
        PDUR[PduR\nPDU 路由器]
    end

    subgraph ECUA["ECU 抽象层 (ECU Abstraction)"]
        IOHWAB[IoHwAb\n信号抽象]
        CANIF[CanIf\nCAN 接口]
        CANTP[CanTp\n传输协议]
        CANNM[CanNm\n网络管理]
    end

    subgraph MCAL["微控制器抽象层 (MCAL)"]
        CAN_DRV[Can Driver]
        SPI_DRV[Spi Driver]
        ADC_DRV[Adc Driver]
        FLS_DRV[Fls Driver]
        WDG_DRV[Wdg Driver]
    end

    subgraph HW["硬件 (Hardware)"]
        MCU[MCU Core\nCPU·Cache·MMU]
        PERIPH[外设\nCAN·SPI·ADC·Flash]
    end

    SWC1 & SWC2 & SWC3 <--> RTE_CORE
    RTE_CORE --> OS & NVM & COM & DCM & DEM & WDG
    DCM --> CANTP
    COM --> PDUR --> CANIF --> CAN_DRV
    CANTP --> CANIF
    NVM --> FLS_DRV
    WDG --> WDG_DRV
    IOHWAB --> ADC_DRV & SPI_DRV
    CAN_DRV & ADC_DRV & FLS_DRV & WDG_DRV --> PERIPH
    MCU --> PERIPH
```

---

## 3. OS 任务调度机制

### 3.1 AUTOSAR OS 任务类型与优先级

AUTOSAR OS 基于 **OSEK/VDX** 标准，采用**静态优先级抢占调度**（Fixed Priority Preemptive Scheduling）。

| 任务类型 | 调度方式 | 典型周期 | ASIL 等级 | 示例 |
|----------|----------|----------|-----------|------|
| ISR Category 1 | 立即响应，不可被 OS 管理 | 事件触发 | ASIL-D | CAN 接收中断 |
| ISR Category 2 | OS 管理，可激活 Task | 事件触发 | ASIL-C/D | 定时器中断 |
| Basic Task | 运行到完成，不允许等待 | 1ms / 5ms / 10ms | ASIL-B/C | 控制算法 |
| Extended Task | 可等待事件，支持 WaitEvent | 20ms / 100ms | QM/ASIL-A | 诊断处理 |

### 3.2 任务调度时序图

```mermaid
sequenceDiagram
    autonumber
    participant HW as 硬件定时器
    participant ISR as ISR Cat2<br/>(优先级最高)
    participant T1 as Task_1ms<br/>(优先级 30)
    participant T5 as Task_5ms<br/>(优先级 20)
    participant T10 as Task_10ms<br/>(优先级 10)
    participant IDLE as Idle Task<br/>(优先级 0)

    Note over HW,IDLE: t=0ms 系统启动

    HW->>ISR: 1ms 定时器中断触发
    ISR->>T1: ActivateTask(Task_1ms)
    ISR-->>HW: 中断返回

    T1->>T1: 执行传感器采样 & 控制计算
    Note over T1: WCET ≤ 0.4ms (预算 40%)

    T1->>IDLE: Task 完成，切换回 Idle

    HW->>ISR: 5ms 到期 (第5次 1ms 中断)
    ISR->>T5: ActivateTask(Task_5ms)
    ISR->>T1: ActivateTask(Task_1ms)

    Note over T5,T1: T1 优先级高，抢占 T5

    T1->>T1: 执行 Task_1ms
    T1->>T5: Task_1ms 完成，T5 恢复执行

    T5->>T5: CAN 信号发送 & 状态更新
    Note over T5: WCET ≤ 1.5ms

    T5->>T10: 通过 SetEvent 激活 Task_10ms
    T10->>T10: 诊断轮询 & NvM 操作
    T10->>IDLE: Task_10ms 完成
```

### 3.3 Rate Monotonic 可调度性分析

对于 $n$ 个周期任务，**Rate Monotonic（RM）** 调度的充分条件：

$$U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq n\left(2^{1/n} - 1\right)$$

其中 $C_i$ 为任务 $i$ 的最坏执行时间（WCET），$T_i$ 为周期。

当 $n \to \infty$ 时，利用率上界趋近于：

$$U_{bound} = \ln 2 \approx 0.693$$

**示例验证**（3个任务）：

| 任务 | 周期 $T_i$ | WCET $C_i$ | 利用率 $C_i/T_i$ |
|------|-----------|-----------|----------------|
| Task_1ms | 1 ms | 0.35 ms | 0.350 |
| Task_5ms | 5 ms | 1.20 ms | 0.240 |
| Task_10ms | 10 ms | 1.50 ms | 0.150 |
| **合计** | — | — | **0.740** |

$$U_{bound}(3) = 3 \times (2^{1/3} - 1) \approx 3 \times 0.26 = 0.779$$

$$0.740 < 0.779 \Rightarrow \text{可调度性验证通过 ✅}$$

---

## 4. RTE 通信原理

### 4.1 SWC 间通信模式

AUTOSAR RTE 提供两种端口接口：

**① Sender/Receiver (S/R) 接口** — 数据流通信，适合周期信号

**② Client/Server (C/S) 接口** — 服务调用通信，适合触发式操作

```mermaid
flowchart LR
    subgraph SWC_A["SWC_A (车速传感器处理)"]
        RUNA["Runnable:\nReadSpeed()"]
        PORT_S["Send Port\n[VehicleSpeed : float32]"]
    end

    subgraph RTE["RTE 内部缓冲"]
        BUF["Data Buffer\nVehicleSpeed\nTimestamp\nStatus"]
    end

    subgraph SWC_B["SWC_B (制动控制)"]
        PORT_R["Receive Port\n[VehicleSpeed : float32]"]
        RUNB["Runnable:\nBrakeControl()"]
        CLI["Client Port\n[NvM_WriteBlock]"]
    end

    subgraph SWC_NVM["NvM Server"]
        SRV["Server Port\n[NvM_WriteBlock]"]
        NVM_SVC["WriteBlock()"]
    end

    RUNA -->|Rte_Write| PORT_S --> BUF
    BUF -->|Rte_Read| PORT_R --> RUNB
    RUNB -->|Rte_Call| CLI --> SRV --> NVM_SVC
    NVM_SVC -->|Rte_Result| RUNB
```

### 4.2 RTE 信号更新时序

```mermaid
sequenceDiagram
    autonumber
    participant OS_1ms as OS Alarm\n(1ms 周期)
    participant SWC_A as SWC_A Runnable\nReadSpeed
    participant RTE as RTE Buffer
    participant SWC_B as SWC_B Runnable\nBrakeControl
    participant COM as COM Layer

    OS_1ms->>SWC_A: 激活 Runnable ReadSpeed
    SWC_A->>SWC_A: 读取 ADC 采样值，计算车速
    SWC_A->>RTE: Rte_Write(PP_Speed, &speedValue)
    Note over RTE: 更新缓冲区，置 DataUpdated 标志

    OS_1ms->>SWC_B: 激活 Runnable BrakeControl
    SWC_B->>RTE: Rte_Read(RP_Speed, &localSpeed)
    RTE-->>SWC_B: 返回最新车速值 + E_OK
    SWC_B->>SWC_B: 制动力矩计算

    SWC_B->>RTE: Rte_Write(PP_TorqueCmd, &torque)
    RTE->>COM: 触发信号发送 (Signal Group: TorqueSignals)
    COM-->>RTE: 发送确认
```

---

## 5. 通信栈协议流程

### 5.1 CAN 发送完整链路

```mermaid
flowchart TD
    SWC["SWC\nRte_Write(Signal)"] 
    --> COM["COM 层\nSignal → PDU 打包\n字节序·位域转换"]
    --> PDUR["PduR\n路由决策\n(CanTp / CanIf 分发)"]
    --> CANIF["CanIf\n选择 CAN 控制器\nPDU → CAN Frame"]
    --> CDRV["Can Driver (MCAL)\n写入 TX Mailbox"]
    --> HW["CAN 控制器硬件\nBit Stuffing · CRC · ACK"]
    --> BUS[CAN 总线]

    BUS -->|接收方| HW2["对端 CAN 控制器"]
    HW2 --> CDRV2["Can Driver RX ISR\n从 RX FIFO 读取"]
    CDRV2 --> CANIF2["CanIf\nCanIf_RxIndication()"]
    CANIF2 --> PDUR2["PduR\n路由到 COM 或 CanTp"]
    PDUR2 --> COM2["COM\nPDU → Signal 解包"]
    COM2 --> RTE2["RTE Buffer 更新"]
    RTE2 --> SWC2["SWC\nRte_Read(Signal)"]
```

### 5.2 CanTp 分段传输（诊断报文 > 8 字节）

```mermaid
sequenceDiagram
    autonumber
    participant TESTER as 诊断仪 (Tester)
    participant CANIF_R as CanIf (接收)
    participant CANTP as CanTp
    participant DCM as DCM

    Note over TESTER,DCM: 发送 UDS 请求（例：ReadDataByIdentifier，12字节）

    TESTER->>CANIF_R: CAN Frame [0x02, 0x21, DID_H, DID_L, ...]
    Note over TESTER: First Frame (FF)\nDataLength = 12

    CANIF_R->>CANTP: CanIf_RxIndication(FF)
    CANTP->>CANTP: 分配接收缓冲区，记录剩余长度

    CANTP->>TESTER: Flow Control Frame (FC)\n[0x30, BS=0, STmin=25ms]
    Note over CANTP: 允许发送，块大小=0(无限), 最小间隔25ms

    TESTER->>CANIF_R: Consecutive Frame 1 (CF)\n[0x21, data[6..12]]
    CANIF_R->>CANTP: CanIf_RxIndication(CF)
    CANTP->>CANTP: 拼装完整 PDU

    CANTP->>DCM: PduR_CanTpRxIndication(full SDU, 12 bytes)
    DCM->>DCM: 解析 UDS Service 0x22
    DCM->>CANTP: 响应报文（构造 Response PDU）
    CANTP->>TESTER: 分段发送响应
```

---

## 6. 诊断服务架构

### 6.1 UDS 诊断请求处理流程

```mermaid
flowchart TD
    CAN_RX[CAN RX 中断\n接收诊断帧] 
    --> CANIF_IND[CanIf_RxIndication]
    --> CANTP_RX[CanTp 组帧/传输协议]
    --> PDUR_IND[PduR_CanTpRxIndication]
    --> DCM_RX[Dcm_TpRxIndication\n接收完整 SDU]
    --> PARSE{服务ID解析\nSID?}

    PARSE -->|0x10 DiagnosticSessionControl| SES[会话管理\n切换 Default/Extended/Programming]
    PARSE -->|0x11 ECU Reset| RST[复位处理\nHard/Soft/KeyOffOn]
    PARSE -->|0x22 ReadDataByIdentifier| RDBI[读取 DID\n查 DID 路由表]
    PARSE -->|0x2E WriteDataByIdentifier| WDBI[写入 DID\n权限校验 → NvM 写]
    PARSE -->|0x27 SecurityAccess| SEC[安全访问\nSeed-Key 挑战响应]
    PARSE -->|0x14 ClearDiagnosticInfo| CLR["清除 DTC<br/>Dem_ClearDTC"]
    PARSE -->|0x19 ReadDTCInfo| DTC["读取 DTC<br/>Dem_GetDTCInfo"]
    PARSE -->|未知 SID| NRC["否定响应<br/>NRC 0x11 ServiceNotSupported"]

    RDBI --> DID_ROUTE{DID 路由}
    DID_ROUTE -->|映射到 SWC| RTE_DATA[Rte_Read 获取数据]
    DID_ROUTE -->|映射到 BSW| BSW_DATA[直接读取 BSW 模块]
    RTE_DATA & BSW_DATA --> RESP[构造肯定响应 0x62]
    SES & RST & SEC & CLR & DTC & RESP --> TX[DCM 构造响应 PDU]
    NRC --> TX
    TX --> CANTP_TX[CanTp 分段发送]
    CANTP_TX --> CAN_TX[CAN 帧发送]
```

### 6.2 Seed-Key 安全访问时序

```mermaid
sequenceDiagram
    autonumber
    participant TESTER as 诊断仪
    participant DCM as DCM
    participant SEC_MOD as SecurityAccess\nModule

    TESTER->>DCM: 0x27 0x01 (RequestSeed, Level=0x01)
    DCM->>SEC_MOD: Dcm_GetSeed(level=1)
    SEC_MOD->>SEC_MOD: 生成随机 Seed (32bit)
    Note over SEC_MOD: 启动超时计时器 (默认10s)
    SEC_MOD-->>DCM: Seed = 0xA3F2B1C8
    DCM-->>TESTER: 0x67 0x01 0xA3 0xF2 0xB1 0xC8

    TESTER->>TESTER: Key = SecurityAlgo(Seed, SecretConst)
    TESTER->>DCM: 0x27 0x02 (SendKey, Key=0x5C1D3E7A)

    DCM->>SEC_MOD: Dcm_CompareKey(key)
    SEC_MOD->>SEC_MOD: 本地计算期望 Key 并比对

    alt Key 匹配
        SEC_MOD-->>DCM: E_OK，解锁成功
        DCM-->>TESTER: 0x67 0x02 (肯定响应)
        Note over DCM: SecurityLevel = 1 已激活
    else Key 不匹配
        SEC_MOD-->>DCM: E_NOT_OK
        SEC_MOD->>SEC_MOD: 失败计数+1\n≥3次 → 锁定(延迟10min)
        DCM-->>TESTER: 0x7F 0x27 0x35 (InvalidKey)
    end
```

---

## 7. 非易失性存储管理

### 7.1 NvM 模块分层架构

```mermaid
graph TB
    subgraph APP_LAYER["应用/服务层"]
        SWC_NVM[SWC\nNvM_WriteBlock / NvM_ReadBlock]
        FEE_USER[Fee_Write / Fee_Read]
    end

    subgraph NVM_LAYER["NvM Manager"]
        NVM_CORE[NvM Core\n队列管理·Block 路由]
        NVM_QUEUE["Job Queue\n优先级: Immediate > Standard > Restore"]
    end

    subgraph MEM_LAYER["MemIf 抽象层"]
        MEMIF[MemIf\nDevice 选择路由]
    end

    subgraph DEV_LAYER["存储设备驱动"]
        FEE[Fee\nFlash Emulation EEPROM\n磨损均衡·坏块管理]
        EA[Ea\nEEPROM Abstraction\n物理 EEPROM 访问]
    end

    subgraph MCAL_LAYER["MCAL"]
        FLS[Fls Driver\n内部 Flash\n页擦除·字编程]
        EEP[Eep Driver\n外部 EEPROM\nSPI/I2C 接口]
    end

    SWC_NVM --> NVM_CORE
    NVM_CORE --> NVM_QUEUE
    NVM_CORE --> MEMIF
    MEMIF --> FEE & EA
    FEE --> FLS
    EA --> EEP
```

### 7.2 NvM Block 写入时序

```mermaid
sequenceDiagram
    autonumber
    participant SWC as SWC
    participant NVM as NvM Manager
    participant MEMIF as MemIf
    participant FEE as Fee
    participant FLS as Fls Driver

    SWC->>NVM: NvM_WriteBlock(BlockId=5, DataPtr)
    Note over NVM: 加入 Standard Job Queue

    NVM->>NVM: NvM_MainFunction() 处理队列
    NVM->>NVM: 计算 CRC-16 校验码
    Note over NVM: RAM Block + CRC 打包

    NVM->>MEMIF: MemIf_Write(DeviceIdx, BlockIdx, DataPtr)
    MEMIF->>FEE: Fee_Write(BlockNumber, DataBufferPtr)
    FEE->>FEE: 查找可用页，磨损均衡算法
    FEE->>FLS: Fls_Erase(TargetAddress, Length)

    loop Flash 擦除轮询 (MainFunction驱动)
        FLS->>FLS: 硬件擦除进行中
        FLS-->>FEE: FLS_JOB_PENDING
    end

    FLS-->>FEE: FLS_JOB_OK (擦除完成)
    FEE->>FLS: Fls_Write(TargetAddress, DataPtr, Length)

    loop Flash 编程轮询
        FLS->>FLS: 硬件编程进行中
    end

    FLS-->>FEE: FLS_JOB_OK
    FEE-->>MEMIF: MEMIF_JOB_OK
    MEMIF-->>NVM: MemIf_JobResultType = MEMIF_JOB_OK
    NVM->>NVM: 更新 Block Status = NVM_REQ_OK
    NVM->>SWC: NvM_JobEndNotification() 回调
```

---

## 8. 看门狗与监控机制

### 8.1 WdgM 监控架构

```mermaid
graph TB
    subgraph SWCS["被监控软件组件"]
        SE1["Supervised Entity 1\nTask_1ms Checkpoint"]
        SE2["Supervised Entity 2\nTask_10ms Checkpoint"]
        SE3["Supervised Entity 3\nCAN Rx Alive"]
    end

    subgraph WDGM["WdgM (看门狗管理器)"]
        CP["Checkpoint 管理\n活性监控·时序监控·逻辑监控"]
        GS["Global Status\nOK / FAILED / EXPIRED"]
        MODE["模式管理\nSLEEP / ACTIVE / SHUTDOWN"]
    end

    subgraph WDGIF["WdgIf (看门狗接口)"]
        WDGIF_CORE["WdgIf_SetMode\nWdgIf_SetTriggerCondition"]
    end

    subgraph WDG_DRV["Wdg Driver (MCAL)"]
        IDRV["内部看门狗\nWindow WDG / Timeout WDG"]
        EDRV["外部看门狗\nSPI 接口触发"]
    end

    SE1 & SE2 & SE3 -->|WdgM_CheckpointReached| CP
    CP --> GS
    GS -->|全局状态 FAILED| MODE
    MODE --> WDGIF_CORE
    WDGIF_CORE --> IDRV & EDRV
```

### 8.2 看门狗状态机（窗口看门狗）

```mermaid
stateDiagram-v2
    [*] --> INIT : ECU 上电初始化

    INIT --> ACTIVE : WdgM_Init() 完成\n设置窗口参数 (Open=30%, Close=70%)

    ACTIVE --> WINDOW_OPEN : 进入喂狗窗口\n(Trigger Counter 进入 [WinOpen, WinClose])
    WINDOW_OPEN --> TRIGGERED : WdgM_MainFunction 检测\n所有 SE 活性 OK → 触发喂狗
    TRIGGERED --> ACTIVE : 看门狗计数器复位\n等待下一个周期

    ACTIVE --> FAILED_EARLY : 过早喂狗\n(在 WinOpen 之前触发)
    ACTIVE --> FAILED_LATE : 超时未喂狗\n(超过 WinClose 未触发)
    ACTIVE --> FAILED_LOGIC : SE 逻辑监控失败\n(Checkpoint 顺序异常)

    FAILED_EARLY --> RESET : 硬件强制复位
    FAILED_LATE --> RESET : 硬件强制复位
    FAILED_LOGIC --> SHUTDOWN : WdgM 触发受控关闭\n执行安全响应

    SHUTDOWN --> RESET : 超时后强制复位
    RESET --> [*] : ECU 复位，记录复位原因

    note right of WINDOW_OPEN : Open=30% × Period，Close=70% × Period
    note right of FAILED_LOGIC : DEM记录故障事件，触发FiM禁止相关功能
```

---

## 9. ECU 状态机模型

### 9.1 EcuM 生命周期状态机

```mermaid
stateDiagram-v2
    [*] --> STARTUP : 上电 / 复位

    state STARTUP {
        [*] --> StartPreOS : 初始化 MCU\n配置 PLL·时钟·堆栈
        StartPreOS --> StartOS : StartOS() 调用\n创建任务·中断
        StartOS --> StartPostOS : BSW 初始化\n按顺序初始化各模块
        StartPostOS --> [*]
    }

    STARTUP --> RUN : EcuM_StartupTwo() 完成\n所有 BSW 初始化 OK

    state RUN {
        [*] --> APP_RUNNING : 应用功能运行中
        APP_RUNNING --> WAKEUP_VALIDATION : 唤醒源验证中
        WAKEUP_VALIDATION --> APP_RUNNING : 验证通过
    }

    RUN --> SLEEP : 无唤醒源\nEcuM_GoSleep() 请求

    state SLEEP {
        [*] --> GoSleep : 准备休眠\nBSW 模块休眠前处理
        GoSleep --> HALT : 进入低功耗模式\nCPU 停止执行
        HALT --> WakeupDetect : 检测到唤醒源\n(CAN 唤醒 / LIN 唤醒 / 定时器)
        WakeupDetect --> [*]
    }

    SLEEP --> RUN : 唤醒并验证成功

    RUN --> OFF_PRE : KL15 断电\n接收关机请求

    state OFF_PRE {
        [*] --> SaveNvM : NvM_WriteAll()\n持久化数据
        SaveNvM --> NotifyBSW : 通知各 BSW 模块关闭
        NotifyBSW --> GoOff : 等待关机条件满足
        GoOff --> [*]
    }

    OFF_PRE --> OFF : 关机完成

    OFF --> [*] : ECU 断电

    note right of RUN : KL15 ON 正常运行，所有周期任务运行
    note right of SLEEP : 电流小于1mA，仅唤醒检测电路工作
    note right of OFF_PRE : 最大关机时间 500ms，AUTOSAR规范要求
```

### 9.2 ComM 通信管理器状态机

```mermaid
stateDiagram-v2
    [*] --> NO_COM : 初始化完成\n总线静默

    NO_COM --> FULL_COM : 收到通信请求\nComM_RequestComMode(FULL)
    NO_COM --> NO_COM : 保持静默

    FULL_COM --> SILENT_COM : 无通信请求 (NM 超时)\nNm_NetworkRelease()

    state FULL_COM {
        [*] --> NetworkRequested : 请求网络\nCanNm_NetworkRequest()
        NetworkRequested --> ReadySleep : NM 报文停止\n进入 Ready Sleep 状态
        ReadySleep --> NetworkRequested : 收到新请求
    }

    SILENT_COM --> NO_COM : NM 总线静默超时\n所有通信停止
    SILENT_COM --> FULL_COM : 收到通信激活请求
```

---

## 10. 实时性分析与调度模型

### 10.1 最坏情况响应时间 (WCRT) 分析

对于优先级为 $p$ 的任务 $i$，其最坏情况响应时间 $R_i$ 由以下迭代方程求解：

$$R_i^{(0)} = C_i$$

$$R_i^{(n+1)} = C_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i^{(n)}}{T_j} \right\rceil C_j$$

迭代直至 $R_i^{(n+1)} = R_i^{(n)}$，若 $R_i \leq D_i$（截止期限），则任务可调度。

**示例计算**（Task_5ms，优先级中等，被 Task_1ms 抢占）：

$$R_{5ms}^{(0)} = 1.2\text{ms}$$

$$R_{5ms}^{(1)} = 1.2 + \left\lceil \frac{1.2}{1} \right\rceil \times 0.35 = 1.2 + 2 \times 0.35 = 1.90\text{ms}$$

$$R_{5ms}^{(2)} = 1.2 + \left\lceil \frac{1.9}{1} \right\rceil \times 0.35 = 1.2 + 2 \times 0.35 = 1.90\text{ms}$$

$$R_{5ms} = 1.90\text{ms} \leq D_{5ms} = 5\text{ms} \Rightarrow \text{满足截止期限 ✅}$$

### 10.2 中断延迟模型

任务从触发到开始执行的最大延迟（**调度延迟**）：

$$\Delta_{sched} = T_{ISR} + T_{context\_switch} + \sum_{j \in running} C_j^{remaining}$$

其中：
- $T_{ISR}$：中断响应时间（CPU 流水线刷新 + 向量跳转），典型值 $\approx 50\text{ns}$（TriCore @200MHz）
- $T_{context\_switch}$：OS 上下文切换开销，典型值 $\approx 1\mu s$
- $C_j^{remaining}$：当前运行任务的剩余不可抢占段长度

### 10.3 CPU 负载计算

$$\text{CPU Load} = \frac{\sum_{i=1}^{n} \frac{C_i}{T_i} + \sum_{k=1}^{m} f_k \cdot C_k^{ISR}}{1} \times 100\%$$

其中 $f_k$ 为 ISR $k$ 的触发频率，$C_k^{ISR}$ 为 ISR 执行时间。

**CPU 负载预算分配**（建议）：

| 类别 | 预算占比 | 说明 |
|------|---------|------|
| 周期任务（控制算法） | ≤ 40% | 核心控制功能 |
| 通信栈（CAN/LIN） | ≤ 20% | ISR + 主函数 |
| 诊断与 NvM | ≤ 10% | 低优先级后台任务 |
| OS 开销 | ≤ 5% | 任务切换·调度器 |
| **安全余量** | **≥ 25%** | 应对峰值负载 |

---

## 11. 功能安全机制

### 11.1 ISO 26262 ASIL 分解

```mermaid
flowchart TD
    ITEM["功能安全项\n电子转向助力 (EPS)"]
    --> HARA["危害分析与风险评估 (HARA)\nSeverity × Exposure × Controllability"]
    --> ASILD["整体 ASIL-D 要求\n(S3 × E4 × C3)"]

    ASILD --> DECOMP["ASIL 分解\n独立冗余设计"]
    DECOMP --> PATH_A["通道 A\nASIL-C\n主控制算法"]
    DECOMP --> PATH_B["通道 B\nASIL-B (残余)\n监控与安全停止"]

    PATH_A --> SW_A["软件 A\nCoding Guidelines\nMISRA-C:2012\nStack 覆盖率 ≥ MC/DC"]
    PATH_B --> SW_B["软件 B\n独立编译器·独立 MCU 核\nE2E 通信保护"]

    SW_A & SW_B --> SAFE_STATE["安全状态\n助力扭矩清零\n驾驶员可接管"]
```

### 11.2 E2E 通信保护（AUTOSAR E2E Profile 2）

端到端保护帧格式：

```
┌──────────┬────────────┬──────────────┬─────────────────────────┐
│ CRC [8b] │ Counter[4b]│ DataID[16b]  │  Payload Data           │
│ CRC-8    │ 0~14 循环  │ 配置固定值   │  实际信号数据            │
└──────────┴────────────┴──────────────┴─────────────────────────┘
```

**CRC-8 计算多项式**（E2E Profile 2）：

$$G(x) = x^8 + x^5 + x^4 + 1 \quad (0x2F)$$

**计数器翻转检测**（接收端）：

$$\Delta_{counter} = (Counter_{received} - Counter_{last} + 15) \bmod 15$$

若 $\Delta_{counter} = 0$：**重复帧**（丢弃）；$\Delta_{counter} > 1$：**帧丢失**（记录错误）；$\Delta_{counter} = 1$：**正常**。

### 11.3 故障注入与安全响应状态机

```mermaid
stateDiagram-v2
    [*] --> NORMAL : 系统正常运行

    NORMAL --> DEGRADED : 检测到 E2E 错误\n或 Dem 故障计数达阈值
    NORMAL --> EMERGENCY : 硬件故障\nADC 超量程 / 电源异常

    DEGRADED --> NORMAL : 故障消除 (Healing Counter 清零)
    DEGRADED --> SAFE_STATE : 故障持续 > 容错时间 (FTT)\n或 多故障叠加

    EMERGENCY --> SAFE_STATE : 立即切换\n无容错等待

    SAFE_STATE --> NORMAL : 仅允许在 KL15 重启后恢复
    SAFE_STATE --> OFF : 受控下电

    OFF --> [*]

    note right of DEGRADED : 功能降级运行，关闭非安全相关功能，DEM记录故障事件
    note right of SAFE_STATE : 输出清零，执行器去使能，点亮MIL，响应时间不超过FTTI
```

---

## 12. 内存保护与分区

### 12.1 MPU 分区布局（以 Aurix TriCore 为例）

```mermaid
graph TB
    subgraph FLASH["Flash 存储空间 (2MB)"]
        BOOT["Bootloader 区\n0x8000_0000 ~ 0x8001_FFFF (128KB)\n只读·不可执行修改"]
        BSW_CODE["BSW 代码区\n0x8002_0000 ~ 0x8009_FFFF (512KB)\nASIL-D"]
        APP_CODE["APP 代码区\n0x800A_0000 ~ 0x800F_FFFF (384KB)\nASIL-B/QM"]
        CAL["标定数据区\n0x8010_0000 ~ 0x8013_FFFF (256KB)\nXCP 可写"]
    end

    subgraph RAM["RAM 空间 (512KB)"]
        OS_STACK["OS Stacks\n各任务独立栈\nMPU 保护隔离"]
        BSW_DATA["BSW 数据区\n全局变量·BSS\n仅 BSW 可写"]
        APP_DATA["APP 数据区\nSWC RAM\nRTE 隔离访问"]
        SHARED["共享通信缓冲区\nRTE 管理\n读写权限按端口配置"]
    end

    subgraph MPU["MPU 配置 (16 Region)"]
        R0["Region 0: Flash 全局只读"]
        R1["Region 1: RAM_BSW 读写"]
        R2["Region 2: RAM_APP 读写"]
        R3["Region 3: RAM_Shared 条件读写"]
        R_PRIV["Region 15: 特权模式保留"]
    end

    BSW_CODE --> R0
    BSW_DATA --> R1
    APP_DATA --> R2
    SHARED --> R3
```

### 12.2 栈溢出检测原理

栈使用水位监控公式（**Stack Pattern 检测**）：

$$\text{Stack\_Usage} = \text{Stack\_Top} - \text{Lowest\_Unpainted\_Address}$$

$$\text{Stack\_Margin} = \text{Stack\_Size} - \text{Stack\_Usage}$$

**安全要求**：$\text{Stack\_Margin} \geq 20\%$ × $\text{Stack\_Size}$

栈尾填充 Magic Pattern `0xDEADBEEF`，若 OS\_Task\_GetStackUsage() 检测 Pattern 被覆写，立即触发 `ProtectionHook(E_OS_STACKFAULT)`。

---

## 13. 附录：术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| AUTOSAR | AUTomotive Open System ARchitecture | 汽车开放系统架构标准 |
| BSW | Basic Software | AUTOSAR 基础软件层，标准化平台组件 |
| RTE | Runtime Environment | 运行时环境，SWC 间及 SWC 与 BSW 间通信中间件 |
| SWC | Software Component | 应用软件组件，通过 Port/Interface 与 RTE 交互 |
| MCAL | Microcontroller Abstraction Layer | 微控制器抽象层，直接操作硬件寄存器 |
| OS | AUTOSAR OS (OSEK-based) | 基于 OSEK/VDX 的静态配置实时操作系统 |
| DCM | Diagnostic Communication Manager | 诊断通信管理器，处理 UDS / OBD 服务 |
| DEM | Diagnostic Event Manager | 诊断事件管理器，管理故障码 (DTC) 生命周期 |
| NvM | Non-volatile Memory Manager | 非易失存储管理器，抽象 Flash/EEPROM 访问 |
| WdgM | Watchdog Manager | 看门狗管理器，监控软件活性与逻辑时序 |
| PduR | PDU Router | PDU 路由器，在通信栈各层间路由数据单元 |
| CanIf | CAN Interface | CAN 接口层，抽象多个 CAN 控制器 |
| CanTp | CAN Transport Protocol | CAN 传输协议，实现 ISO 15765-2 分段/重组 |
| CanNm | CAN Network Management | CAN 网络管理，基于 AUTOSAR NM 规范 |
| Fee | Flash EEPROM Emulation | 用 Flash 模拟 EEPROM，提供磨损均衡 |
| ASIL | Automotive Safety Integrity Level | 汽车安全完整性等级，A~D，D 为最高 |
| HARA | Hazard Analysis and Risk Assessment | 危害分析与风险评估，ISO 26262 安全过程 |
| WCET | Worst-Case Execution Time | 最坏情况执行时间，实时性分析关键指标 |
| WCRT | Worst-Case Response Time | 最坏情况响应时间，包含抢占和阻塞延迟 |
| MPU | Memory Protection Unit | 内存保护单元，实现软件分区隔离 |
| E2E | End-to-End Protection | 端到端通信保护，防止传输错误（CRC+计数器）|
| FTT | Fault Tolerant Time | 故障容忍时间，检测故障到进入安全状态的最大时间 |
| UDS | Unified Diagnostic Services | 统一诊断服务，ISO 14229 标准 |
| MISRA-C | — | 汽车行业 C 语言编码规范，禁止危险语言特性 |

---

> **文档变更记录**
>
> | 版本 | 日期 | 变更内容 | 作者 |
> |------|------|---------|------|
> | v3.0.0 | 2025-11 | 全面重构：AUTOSAR 架构·OS 调度·E2E·MPU 章节 | BSW 平台组 |
> | v2.2.0 | 2025-07 | 新增 EcuM/ComM 状态机·WdgM 窗口看门狗 | 安全组 |
> | v2.0.0 | 2025-03 | 增加 UDS 诊断流程·CanTp 时序·NvM 分层 | 通信组 |
> | v1.0.0 | 2024-09 | 初始版本：基础 AUTOSAR 分层架构描述 | BSW 平台组 |