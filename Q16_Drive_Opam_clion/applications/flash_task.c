/**
 * @file    flash_task.c
 * @brief   Flash延迟任务队列实现 - 基于message_center
 * @author  FOC Development Team
 * @date    2026-03-27
 */

#include "flash_task.h"
#include "debug/debug.h"
#include "easyflash.h"
#include "flash_config.h"
#include <string.h>

#define FLASH_TASK_TOPIC_NAME "FlashTask" ///< 任务话题名称
#define FLASH_TASK_IDLE_THRESHOLD 50      ///< CPU空闲率阈值(50%)

static flash_task_mgr_t g_flash_task_mgr = {0};

/**
 * @brief   默认环境变量表
 * @note    这些变量会从Flash加载到RAM中
 */
const ef_env default_env_set[FLASH_ENV_SET_SIZE] = {
    {FLASH_ENCODER_MAGIC, &g_motor_flash_cfg, sizeof(g_motor_flash_cfg)}, ///< 电机校准数据
    {FLASH_LINE_HALL_MAGIC, &hall_save_param, sizeof(hall_save_param)},   ///< 霍尔传感器参数
    {FLASH_CAN_ID_MAGIC, &can_save_id, sizeof(can_save_id)},              ///< CAN通信ID配置
};

/**
 * @brief 写入校准数据任务处理
 */
static void task_write_cali(void *data, size_t size)
{
    ef_set_env_blob(FLASH_ENCODER_MAGIC, data, size);
    DEBUG_INFO("[FlashTask] 编码器校准写入");
}

/**
 * @brief 写入霍尔参数任务处理
 */
static void task_write_hall(void *data, size_t size)
{
    /* TODO: 实现霍尔参数写入 */
    ef_set_env_blob(FLASH_LINE_HALL_MAGIC, data, size);

    DEBUG_INFO("[FlashTask] 霍尔参数写入");
}

/**
 * @brief 写入CAN-ID任务处理
 */
static void task_write_can(void *data, size_t size)
{
    ef_set_env_blob(FLASH_CAN_ID_MAGIC, data, size);
    DEBUG_INFO("[FlashTask] CAN ID 写入为: %d", *(int *)data);
}

/**
 * @brief 擦除所有数据任务处理
 */
static void task_erase_all(void *data, size_t size)
{
    g_motor_flash_cfg = (motor_flash_config_t){0}; // 或具体的默认值
    can_save_id = 0;
    hall_save_param = (hall_save_param_t){0};

    ef_env_set_default();
    /* TODO: 实现擦除所有 */
    DEBUG_INFO("[FlashTask] 擦除所有数据");
}

/* 任务处理函数表 */
static void (*task_handler[])(void *, size_t) = {
    [FLASH_TASK_WRITE_CALI] = task_write_cali,
    [FLASH_TASK_WRITE_HALL] = task_write_hall,
    [FLASH_TASK_WRITE_CAN] = task_write_can,
    [FLASH_TASK_ERASE_ALL] = task_erase_all,
};

/**
 * @brief 初始化Flash任务管理器
 */
int flash_task_init(void)
{
    /* 注册发布者 */
    g_flash_task_mgr.publisher = PubRegister(FLASH_TASK_TOPIC_NAME, sizeof(flash_task_request_t));
    if (g_flash_task_mgr.publisher == NULL)
    {
        DEBUG_ERROR("[FlashTask] 发布者注册失败");
        return -1;
    }

    /* 注册订阅者 */
    g_flash_task_mgr.subscriber = SubRegister(FLASH_TASK_TOPIC_NAME, sizeof(flash_task_request_t));
    if (g_flash_task_mgr.subscriber == NULL)
    {
        DEBUG_ERROR("[FlashTask] 订阅者注册失败");
        MessageCenterUnregister(FLASH_TASK_TOPIC_NAME);
        return -1;
    }

    g_flash_task_mgr.idle_threshold = FLASH_TASK_IDLE_THRESHOLD;
    g_flash_task_mgr.pending_count = 0;

    DEBUG_INFO("[FlashTask] 初始化成功");
    return 0;
}

/**
 * @brief 获取任务管理器实例
 */
flash_task_mgr_t *flash_task_get_instance(void)
{
    return &g_flash_task_mgr;
}

/**
 * @brief 请求Flash任务（异步）
 */
void flash_task_request(flash_task_type_t type, void *data, size_t size)
{
    flash_task_request_t req;
    req.type = type;
    req.data = data;
    req.size = size;

    /* 发布消息 */
    uint8_t subs_count = PubPushMessage(g_flash_task_mgr.publisher, &req);
    
    if (subs_count == 0)
    {
        DEBUG_WARN("[FlashTask] 发布消息失败, 类型: %d", type);
        return;
    }

    g_flash_task_mgr.pending_count = flash_task_get_pending_count();
    DEBUG_DEBUG("[FlashTask] 任务已加入队列, 类型: %d, 待处理: %lu", 
               type, g_flash_task_mgr.pending_count);
}

/**
 * @brief 处理Flash任务（低优先级任务中调用）
 */
void flash_task_process(void)
{
    flash_task_request_t req;

    /* 检查是否有新消息 */
    if (!SubGetMessage(g_flash_task_mgr.subscriber, &req))
    {
        return;
    }

    /* TODO: 实现CPU空闲率检测
     * 当前版本先检查是否有紧急任务需要处理
     * 实际使用时可以根据 perf_counter 或其他机制获取CPU空闲率
     */
    // uint8_t cpu_idle = get_cpu_idle_percent();
    // if (cpu_idle < g_flash_task_mgr.idle_threshold) {
    //     return;  // CPU繁忙，延迟处理
    // }

    if (req.type < FLASH_TASK_COUNT && task_handler[req.type] != NULL)
    {
        DEBUG_DEBUG("[FlashTask] 执行任务类型: %d", req.type);

        /* 关中断执行Flash写入 */
        __disable_irq();
        task_handler[req.type](req.data, req.size);
        __enable_irq();

        DEBUG_DEBUG("[FlashTask] 任务 %d 完成", req.type);
    }
    else
    {
        DEBUG_WARN("[FlashTask] 未知任务类型: %d", req.type);
    }

    g_flash_task_mgr.pending_count = flash_task_get_pending_count();
}

/**
 * @brief 获取等待执行的任务数
 */
uint32_t flash_task_get_pending_count(void)
{
    if (g_flash_task_mgr.publisher == NULL || g_flash_task_mgr.subscriber == NULL)
    {
        return 0;
    }
    
    /* 计算待处理的消息数 */
    uint32_t write_count = g_flash_task_mgr.publisher->write_count;
    uint32_t read_count = g_flash_task_mgr.subscriber->read_count;
    
    /* 处理溢出情况 */
    if (write_count >= read_count)
    {
        return write_count - read_count;
    }
    else
    {
        /* 32位溢出，计算实际差值 */
        return (UINT32_MAX - read_count) + write_count + 1;
    }
}

/**
 * @brief 销毁Flash任务管理器（释放资源）
 */
void flash_task_destroy(void)
{
    if (g_flash_task_mgr.publisher != NULL || g_flash_task_mgr.subscriber != NULL)
    {
        MessageCenterUnregister(FLASH_TASK_TOPIC_NAME);
        g_flash_task_mgr.publisher = NULL;
        g_flash_task_mgr.subscriber = NULL;
    }
    DEBUG_INFO("[FlashTask] 已销毁");
}
