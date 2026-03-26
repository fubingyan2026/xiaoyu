
--- START OF FILE text/markdown ---

# message_center 消息中心模块

<p align='right'>版本: 3.0.0 (共享队列重构版) | 作者: NeoZeng, fubingyan</p>

基于**发布-订阅（Pub/Sub）**模式的消息中心，用于解耦应用间的消息通信。

本次重构引入了**一写多读（Single-Writer, Multiple-Reader）**的共享环形队列架构，极大地降低了多订阅者场景下的内存消耗与 CPU 拷贝负担。

---

## 🚀 核心特性

- **O(1) 零增量拷贝** — 发布消息时，无论该话题有 1 个还是 100 个订阅者，底层只发生 **1 次** 数据拷贝。极大节省了高频或大载荷（如图像、长数组）通信时的总线带宽。
- **共享环形队列** — 队列内存统一由 **发布者（Topic）** 持有。订阅者被剥离了笨重的私有队列，仅维护轻量级的“读取进度（游标）”，内存占用锐降。
- **静态/动态无缝支持** — 支持静态分配实例与缓存，完美适配无动态内存（`malloc/free`）的严苛嵌入式场景。
- **防脏读与自动丢帧机制** — 当消费者处理过慢、读取进度落后于队列深度时，框架可自动察觉并使进度安全跳跃（丢弃已被覆盖的老数据），防止读取错乱。

---

## 📖 核心概念

| 角色 | 说明 |
|-----|------|
| **发布者（Publisher / Topic）** | 消息的发送方与**存储方**。负责维护共享环形队列与全局的 `write_count`（写进度）。 |
| **订阅者（Subscriber）** | 消息的接收方。极为轻量，仅维护自身的 `read_count`（读进度）和话题的引用。 |
| **话题（Topic）** | 消息的“频道”，以字符串名称区分。订阅者通过话题名称与发布者建立连接。 |

---

## 🛠️ 快速上手

### 1. 初始化系统

在使用任何 API 前，必须先初始化消息中心：

```c
#include "message_center.h"

// 初始化消息系统
MessageCenterInit();
```

### 2. 注册发布者（Topic）

**动态注册（便捷）：**
```c
// 自动分配 Publisher_t 结构体与 MSG_DEFAULT_QUEUE_SIZE 个深度的队列缓存
Publisher_t *pub = PubRegister("sensor_data", sizeof(float));
```

**静态注册（推荐无 RTOS 或极度要求稳定性的场景）：**
> ⚠️ **注意：** 静态注册时，**必须由用户手动绑定共享队列的缓存！**

```c
static Publisher_t sensor_pub;
static float shared_queue_buf[MSG_DEFAULT_QUEUE_SIZE]; // 预分配一片共享数据区

// 1. 注册实例
PubRegisterStatic("sensor_data", sizeof(float), &sensor_pub);

// 2. 绑定共享队列内存（关键步骤！）
for (int i = 0; i < MSG_DEFAULT_QUEUE_SIZE; i++) {
    sensor_pub.queue[i] = &shared_queue_buf[i];
}
```

### 3. 注册订阅者

得益于共享队列重构，订阅者现在变得**非常轻量**，不再需要分配庞大的队列缓存。

**动态注册：**
```c
Subscriber_t *sub = SubRegister("sensor_data", sizeof(float));
```

**静态注册（极简）：**
```c
static Subscriber_t sensor_sub1;
static Subscriber_t sensor_sub2;

// 静态订阅者现在不需要绑定任何缓存，直接注册即可
SubRegisterStatic("sensor_data", sizeof(float), &sensor_sub1);
SubRegisterStatic("sensor_data", sizeof(float), &sensor_sub2);
```

### 4. 消息收发

**发布消息（Push）：**
```c
float sensor_value = 3.14f;
// 无论有几个 Sub，这里内部只发生 1 次 memcpy
PubPushMessage(&sensor_pub, &sensor_value);
```

**获取消息（Get）：**
```c
float received_value;
// 各个 Sub 独立推进自己的读取游标，互不干扰
if (SubGetMessage(&sensor_sub1, &received_value)) {
    // sub1 成功获取到新消息
}
```

**返回值：**
- `1` — 成功获取到新消息。
- `0` — 进度已追平（无新消息），或发生了参数错误。

---

## ⚙️ 实例与生命周期管理

### 查找实例

```c
// 按话题名称查找发布者
Publisher_t *pub = MessageCenterGetPublisher("sensor_data");

// 查找话题的第一个订阅者
Subscriber_t *sub = MessageCenterGetSubscriber("sensor_data");
```

### 注销话题与内存回收

```c
// 注销话题，这会安全地移除该话题及其下的所有订阅者
// 动态分配的内存会被自动 free()，静态分配的只解除注册
MessageCenterUnregister("sensor_data");

// 在系统关闭时，销毁整个消息中心
MessageCenterDeinit();
```

---

## 🔧 配置参数

可在工程的编译选项或包含该头文件前，通过宏覆盖默认值：

```c
#define MSG_MAX_TOPICS 32           // 系统允许的最大话题数量
#define MSG_MAX_TOPIC_NAME_LEN 32   // 话题名称字符串的最大长度
#define MSG_DEFAULT_QUEUE_SIZE 4    // 默认环形队列深度 (建议设为 2 的幂次方，如 4, 8, 16)
```

---

## 🐞 错误码参考

```c
typedef enum {
    MSG_OK = 0,                 // 成功
    MSG_OK_EXISTED = 1,         // 初始化时系统已存在
    MSG_ERR_INVALID_PARAM = -1, // 无效参数（如空指针、名称超长）
    MSG_ERR_NO_MEMORY = -2,     // 话题数达上限或 malloc 失败
    MSG_ERR_NOT_FOUND = -3,     // 未找到指定话题
    MSG_ERR_ALREADY_EXIST = -4, // 静态注册时话题已存在
    MSG_ERR_INTERNAL = -5,      // 内部逻辑错误
} msg_error_e;
```

辅助宏：
```c
MSG_IS_OK(err)  // >= 0 时为真
MSG_IS_ERR(err) // < 0 时为真
```

---

## 🔍 实现细节与原理解析

### 1. 进度追踪（`write_count` 与 `read_count`）
- `Publisher` 拥有 `write_count`，每次发布自增 1。
- `Subscriber` 拥有 `read_count`，每次成功读取自增 1。
- 实际映射到环形数组的索引为：`idx = count % MSG_DEFAULT_QUEUE_SIZE`。

### 2. 滞后处理与防覆盖
当某一订阅者的处理任务被阻塞，导致 `pub->write_count - sub->read_count > MSG_DEFAULT_QUEUE_SIZE` 时，说明该订阅者原本该读的最老数据**已经被新数据覆盖了**。
此时，`SubGetMessage` 会触发**自救机制**：自动将 `read_count` 快进到 `write_count - MSG_DEFAULT_QUEUE_SIZE`（即当前存活的最老消息处），从而跳过损坏的数据。

### 3. 内存隔离
- **动态实例** 的内存在反初始化 (`Deinit`) 或注销 (`Unregister`) 时会自动 `__free()`。
- **静态实例** 的 `is_static` 标志会被置为 `1`，系统进行销毁遍历时，会安全跳过它们，不会引发 HardFault。

---

## 📝 完整业务示例

```c
#include "message_center.h"

// ================= 全局静态内存区 =================
static Publisher_t  pub_imu;
static float        pub_imu_queue[MSG_DEFAULT_QUEUE_SIZE]; // IMU 数据比较大，复用非常有意义

static Subscriber_t sub_imu_filter; // 负责滤波的订阅者
static Subscriber_t sub_imu_logger; // 负责日志的订阅者

// ================= 1. 初始化配置 =================
void System_Message_Init(void)
{
    MessageCenterInit();

    // 注册发布者
    PubRegisterStatic("imu_data", sizeof(float), &pub_imu);
    for (int i = 0; i < MSG_DEFAULT_QUEUE_SIZE; i++) {
        pub_imu.queue[i] = &pub_imu_queue[i]; // 绑定共享队列
    }

    // 注册订阅者 (自动挂载到 imu_data 话题)
    SubRegisterStatic("imu_data", sizeof(float), &sub_imu_filter);
    SubRegisterStatic("imu_data", sizeof(float), &sub_imu_logger);
}

// ================= 2. 生产者任务 =================
void Task_IMU_Read(void)
{
    while(1) {
        float new_data = hardware_read_imu();
        
        // 推送给所有订阅者 (内部只做1次memcpy)
        PubPushMessage(&pub_imu, &new_data); 
        
        delay_ms(10);
    }
}

// ================= 3. 消费者任务 =================
void Task_Filter(void)
{
    float data;
    while(1) {
        // 持续拉取新数据
        while (SubGetMessage(&sub_imu_filter, &data)) {
            apply_kalman_filter(data);
        }
        delay_ms(5);
    }
}

void Task_Logger(void)
{
    float data;
    while(1) {
        // SD卡写入较慢，可能发生跨帧跳跃，但读取到的数据一定是完整的、未污染的
        if (SubGetMessage(&sub_imu_logger, &data)) {
            sd_card_write(data);
        }
        delay_ms(50);
    }
}
```

--- END OF FILE text/markdown ---