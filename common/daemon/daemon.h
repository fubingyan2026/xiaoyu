#ifndef DAEMON_H
#define DAEMON_H

#include "stdint.h"

/* 守护进程错误码 */
typedef enum
{
    DAEMON_OK = 0,                 // 成功
    DAEMON_OK_EXISTED = 1,         // 已存在
    DAEMON_ERR_INVALID_PARAM = -1, // 无效参数
    DAEMON_ERR_NO_MEMORY = -2,     // 内存不足
    DAEMON_ERR_NOT_FOUND = -3,     // 未找到
    DAEMON_ERR_ALREADY_EXIST = -4, // 已存在
    DAEMON_ERR_INTERNAL = -5,      // 内部错误
} daemon_error_e;

/* 错误码检查宏 */
#define DAEMON_IS_OK(err) ((err) >= 0)
#define DAEMON_IS_ERR(err) ((err) < 0)

/* 模块离线处理函数指针 */
typedef void (*offline_callback)(void *);

/* 获取系统时间戳的回调函数指针 (返回毫秒数) */
typedef uint32_t (*daemon_get_time_func)(void);

/* daemon初始化配置 */
typedef struct
{
    uint16_t reload_time_out;  // 离线超时时间(ms), 0xFFFF或0表示永不超时
    uint16_t init_wait_time;   // 上线等待时间(ms)
    offline_callback callback; // 异常处理回调函数
    const char *owner_name;    // 拥有者的名称
    void *owner_pointer;       // 指向拥有者的指针，作为回调的参数
} daemon_init_config_t;

/* daemon结构体定义 */
typedef struct daemon
{
    daemon_init_config_t config; // 初始化配置
    struct
    {
        uint32_t last_temp_time;      // 上次喂狗时间
        uint32_t temp_time;           // 当前喂狗时间
        float daemon_frequent;        // 喂狗频率,单位Hz
        uint16_t init_wait_time : 15; // 上线等待时间计数
        uint16_t online : 1;          // 在线状态,1在线,0离线
        uint16_t online_last : 1;     // 上次在线状态,用于状态变化检测
        uint8_t rx_counts;            // 接收计数,用于稳定性判断
        uint8_t is_static : 1;        // 标记是否为静态分配，静态实例不可被free
    } data;
    struct daemon *next_daemon_lists; // 指向下一个daemon实例的指针
} daemon_t;

/* 常量定义 */
#define DAEMON_STABLE_TIMES_MS 10 // 稳定计数阈值(毫秒)

/* 函数声明 */
daemon_error_e DaemonInit(daemon_get_time_func get_time_cb);
void DaemonDeinit(void);

/* 注册与注销 API */
daemon_t *DaemonRegister(const daemon_init_config_t *config);                                // 动态分配 (兼容老API)
daemon_error_e DaemonRegisterStatic(const daemon_init_config_t *config, daemon_t *instance); // 静态分配 (新增)
daemon_error_e DaemonUnregister(const char *name);                                           // 注销

/* 状态更新与查询 API */
void DaemonReload(daemon_t *_daemon);
uint8_t DaemonIsOnline(const daemon_t *_daemon);
const char * DaemonGetName(const daemon_t *_daemon);

void DaemonTask(void);

/* 实例获取 API */
daemon_t *DaemonGetInstance(const char *name); // 推荐使用的新API

/* 统计 API */
uint16_t DaemonGetCount(void);
daemon_t *DaemonGetMasterPointer(void);

#endif // DAEMON_H
