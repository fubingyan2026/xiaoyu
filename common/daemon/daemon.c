/**
 * @brief: 守护进程(Daemon)模块
 * @FilePath: daemon.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-23 10:11:53
 * @LastEditTime: 2025-11-07 20:08:29
 * @version: V2.0.0
 * @note: 重构大厂规范版，支持静态/动态内存分配，修复时间溢出与反初始化缺陷
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "daemon.h"
#include "debug/debug.h"
#include "memory_pool/memory_pool.h"

/* 调试打印宏，默认关闭 */
#define DAEMON_PRINTF(...) // DEBUG_INFO(__VA_ARGS__)

/* 用于保存所有的daemon instance */
static daemon_t *daemon_master = NULL;

/* 守护进程系统初始化标志 */
static uint8_t daemon_system_initialized = 0;

/* 当前守护进程数量 */
static uint16_t daemon_count = 0;

/* 获取系统时间戳的回调函数 */
static daemon_get_time_func daemon_get_time = NULL;

/**
 * @brief 获取自系统启动以来的毫秒数
 * @return uint32_t 自系统启动以来经过的毫秒数
 */
static inline uint32_t get_times(void)
{
    if (daemon_get_time != NULL)
    {
        return daemon_get_time();
    }
    return 0; // 如果没有注册时间函数，则返回0（这会导致超时判断失效，因此Init时必须提供有效函数）
}

/**
 * @brief 计算时间差，安全处理32位时间戳溢出回绕
 */
static inline uint32_t daemon_time_diff(uint32_t new_time, uint32_t old_time)
{
    return (new_time >= old_time) ? (new_time - old_time) : (0xFFFFFFFF - old_time + new_time + 1);
}

/**
 * @brief 初始化守护进程系统
 * 必须在调用任何其他守护进程函数前调用此函数
 * @param get_time_cb 提供系统毫秒级时间戳的回调函数
 * @return daemon_error_e 成功返回DAEMON_OK，参数错误返回DAEMON_ERR_INVALID_PARAM
 */
daemon_error_e DaemonInit(daemon_get_time_func get_time_cb)
{
    if (get_time_cb == NULL)
    {
        DAEMON_PRINTF("DaemonInit failed: get_time_cb is NULL\n");
        return DAEMON_ERR_INVALID_PARAM;
    }

    if (daemon_system_initialized)
    {
        return DAEMON_OK_EXISTED; // 避免重复初始化
    }

    daemon_get_time = get_time_cb;
    daemon_master = NULL;
    daemon_count = 0;
    daemon_system_initialized = 1;

    DAEMON_PRINTF("Daemon system initialized\n");
    return DAEMON_OK;
}

/**
 * @brief 反初始化守护进程系统，释放动态分配的资源
 */
void DaemonDeinit(void)
{
    if (!daemon_system_initialized)
    {
        return;
    }

    daemon_t *current = daemon_master;
    daemon_t *next;

    /* 遍历链表释放所有动态分配的守护进程实例 */
    while (current != NULL)
    {
        next = current->next_daemon_lists;
        if (!current->data.is_static)
        {
            __free(current);
        }
        current = next;
    }

    daemon_master = NULL;
    daemon_count = 0;
    daemon_system_initialized = 0;

    DAEMON_PRINTF("Daemon system deinitialized\n");
}

/**
 * @brief 获取指定名称的daemon实例 (新API推荐使用)
 * @param name daemon实例的名称
 * @return daemon_t* 如果找到匹配的daemon实例则返回其指针，否则返回NULL
 */
daemon_t *DaemonGetInstance(const char *name)
{
    if (name == NULL || !daemon_system_initialized)
    {
        return NULL;
    }

    daemon_t *this = daemon_master;
    while (this != NULL)
    {
        if (__strcmp(this->config.owner_name, name) == 0)
        {
            return this;
        }
        this = this->next_daemon_lists;
    }
    return NULL;
}

/**
 * @brief 注册一个新的守护进程 (静态分配版)
 * @param config 指向初始化配置的指针
 * @param instance 预先分配的实例内存
 * @return daemon_error_e 返回错误码
 */
daemon_error_e DaemonRegisterStatic(const daemon_init_config_t *config, daemon_t *instance)
{
    if (config == NULL || config->owner_name == NULL || instance == NULL)
    {
        DAEMON_PRINTF("DaemonRegisterStatic failed: invalid parameter\n");
        return DAEMON_ERR_INVALID_PARAM;
    }

    if (!daemon_system_initialized)
    {
        DAEMON_PRINTF("DaemonRegisterStatic failed: system not initialized\n");
        return DAEMON_ERR_INTERNAL;
    }

    daemon_t *existing = DaemonGetInstance(config->owner_name);
    if (existing != NULL)
    {
        DAEMON_PRINTF("DaemonRegisterStatic failed: daemon %s already exists\n", config->owner_name);
        return DAEMON_ERR_ALREADY_EXIST;
    }

    __memset(instance, 0, sizeof(daemon_t));
    __memcpy(&instance->config, (void *)config, sizeof(daemon_init_config_t));
    
    instance->data.temp_time = get_times();
    instance->data.last_temp_time = instance->data.temp_time;
    instance->data.online_last = 1;
    instance->data.online = 1;
    instance->data.is_static = 1; // 标记为静态分配，防止被free

    /* 使用头插法添加到链表 */
    instance->next_daemon_lists = daemon_master;
    daemon_master = instance;
    daemon_count++;

    DAEMON_PRINTF("Daemon static registered: %s, total: %d\n", instance->config.owner_name, daemon_count);
    return DAEMON_OK;
}

/**
 * @brief 注册一个新的守护进程或返回已存在的守护进程 (动态分配版)
 * @param config 指向初始化配置的指针，包含守护进程的所有必要信息
 * @return daemon_t* 新注册或已存在的守护进程的指针，NULL表示注册失败
 */
daemon_t *DaemonRegister(const daemon_init_config_t *config)
{
    if (config == NULL || config->owner_name == NULL)
    {
        return NULL;
    }

    if (!daemon_system_initialized)
    {
        DAEMON_PRINTF("DaemonRegister failed: system not initialized\n");
        return NULL;
    }

    /* 检查是否已存在同名daemon */
    daemon_t *existing = DaemonGetInstance(config->owner_name);
    if (existing != NULL)
    {
        return existing; // 返回已存在的实例
    }

    /* 分配新daemon实例 */
    daemon_t *new_daemon = (daemon_t *)__malloc(sizeof(daemon_t));
    if (new_daemon == NULL)
    {
        return NULL; // 内存分配失败
    }

    /* 初始化新daemon实例 */
    __memset(new_daemon, 0, sizeof(daemon_t));
    __memcpy(&new_daemon->config, (void *)config, sizeof(daemon_init_config_t));
    
    new_daemon->data.temp_time = get_times();
    new_daemon->data.last_temp_time = new_daemon->data.temp_time;
    new_daemon->data.online_last = 1;
    new_daemon->data.online = 1;
    new_daemon->data.is_static = 0; // 标记为动态分配

    /* 使用头插法添加到链表 */
    new_daemon->next_daemon_lists = daemon_master;
    daemon_master = new_daemon;
    daemon_count++; // 增加计数
    
    DAEMON_PRINTF("Daemon registered: %s, total: %d\n", new_daemon->config.owner_name, daemon_count);
    return new_daemon;
}

/**
 * @brief 删除指定的守护进程
 * @param name 要删除的守护进程名称
 * @return daemon_error_e 成功或错误码
 */
daemon_error_e DaemonUnregister(const char *name)
{
    if (name == NULL)
    {
        return DAEMON_ERR_INVALID_PARAM;
    }
    if (!daemon_system_initialized)
    {
        return DAEMON_ERR_NOT_FOUND;
    }

    daemon_t **ptr = &daemon_master;
    while (*ptr != NULL)
    {
        if (__strcmp((*ptr)->config.owner_name, name) == 0)
        {
            daemon_t *to_remove = *ptr;
            *ptr = (*ptr)->next_daemon_lists; // 从链表中移除
            
            /* 仅当不是静态分配时才释放内存 */
            if (!to_remove->data.is_static)
            {
                __free(to_remove);
            }
            
            daemon_count--; // 减少计数
            DAEMON_PRINTF("Daemon unregistered: %s, remaining: %d\n", name, daemon_count);
            return DAEMON_OK;
        }
        ptr = &(*ptr)->next_daemon_lists;
    }
    
    DAEMON_PRINTF("Daemon not found for unregister: %s\n", name);
    return DAEMON_ERR_NOT_FOUND;
}

/**
 * @brief 获取当前守护进程数量
 * @return uint16_t 守护进程数量
 */
uint16_t DaemonGetCount(void)
{
    return daemon_count;
}

/**
 * @brief 获取主守护进程的指针
 * @return daemon_t* 主守护进程的指针；如果系统未初始化，则返回NULL
 */
daemon_t *DaemonGetMasterPointer(void)
{
    if (!daemon_system_initialized)
    {
        return NULL;
    }
    return daemon_master;
}

/**
 * @brief 重新加载守护程序数据 (喂狗)
 * @param _daemon 指向要重新加载的守护程序实例的指针
 */
void DaemonReload(daemon_t *_daemon)
{
    if (_daemon == NULL)
    {
        return;
    }
    _daemon->data.last_temp_time = _daemon->data.temp_time;
    _daemon->data.temp_time = get_times();
}

/**
 * @brief 检查给定的守护进程是否在线
 * @param _daemon 指向要检查的守护进程实例的指针
 * @return uint8_t 返回1表示守护进程在线，返回0表示守护进程离线或传入的指针为NULL
 */
uint8_t DaemonIsOnline(const daemon_t *_daemon)
{
    if (_daemon == NULL)
    {
        return 0;
    }
    return _daemon->data.online;
}

/**
 * @brief 获取守护进程的名称
 * @param _daemon 指向要获取名称的守护进程实例的指针
 * @return const char* 守护进程的名称；如果传入的指针为NULL，则返回"not_found!"
 */
const char * DaemonGetName(const daemon_t *_daemon)
{
    if (_daemon == NULL)
    {
        return "not_found!";
    }
    return _daemon->config.owner_name;
}



/**
 * @brief 守护任务函数，用于监控和管理所有注册的守护进程。
 * @note 需在主循环或定时器中定期调用
 */
void DaemonTask(void)
{
    /* 系统未初始化或没有注册的守护进程 */
    if (!daemon_system_initialized || daemon_master == NULL)
    {
        return;
    }

    static uint32_t times_last = 0;
    uint32_t current_time = get_times();
    uint32_t diff_times = daemon_time_diff(current_time, times_last);
    times_last = current_time;

    /* 避免除零错误 */
    if (diff_times == 0)
    {
        diff_times = 1;
    }

    daemon_t *this = daemon_master; // 从链表头开始遍历
    while (this != NULL)
    {
        /* 初始化等待阶段 */
        if (this->data.init_wait_time <= this->config.init_wait_time)
        {
            this->data.init_wait_time += diff_times;
            if (this->data.init_wait_time > this->config.init_wait_time)
            {
                this->data.online = 1;
            }
        }
        /* 超时检查 */
        else if (daemon_time_diff(current_time, this->data.temp_time) > this->config.reload_time_out)
        {
            if (this->config.reload_time_out == 0xFFFF || this->config.reload_time_out == 0)
            {
                this->data.online = 1;
            }
            else
            {
                this->data.online = 0;
            }
            this->data.rx_counts = 0;
        }
        /* 稳定性检查 */
        else
        {
            uint32_t threshold = (diff_times > 0) ? (DAEMON_STABLE_TIMES_MS / diff_times) : DAEMON_STABLE_TIMES_MS;
            if (threshold == 0) threshold = 1;
            
            if (++this->data.rx_counts >= threshold)
            {
                this->data.rx_counts = 0;
                this->data.online = 1;
            }
        }

        /* 计算喂狗频率 */
        if (this->data.temp_time > this->data.last_temp_time)
        {
            uint32_t time_diff = daemon_time_diff(this->data.temp_time, this->data.last_temp_time);
            if (time_diff > 0)
            {
                this->data.daemon_frequent = 1000.0f / (float)time_diff;
            }
        }

        /* 状态变化处理 */
        if (this->data.online_last != this->data.online)
        {
            if (this->config.callback != NULL)
            {
                // module内可以将owner_pointer强制类型转换成自身类型从而调用特定module的offline callback
                this->config.callback(this->config.owner_pointer);
            }
            this->data.online_last = this->data.online;
        }

        this = this->next_daemon_lists; // 移动到下一个节点
    }
}
