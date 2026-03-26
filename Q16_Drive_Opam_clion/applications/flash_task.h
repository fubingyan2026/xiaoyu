/**
 * @file    flash_task.h
 * @brief   Flash延迟任务队列 - 基于kqueue通用队列
 * @author  FOC Development Team
 * @date    2026-02-04
 */

#ifndef FLASH_TASK_H
#define FLASH_TASK_H

#include "kfifo/kqueue.h"
#include <stddef.h>
#include <stdint.h>
#include "flash_config.h"

/*============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief Flash任务类型
 */
typedef enum __attribute__((packed))
{
    FLASH_TASK_WRITE_CALI = 0, ///< 写入校准数据
    FLASH_TASK_WRITE_HALL,     ///< 写入霍尔参数
    FLASH_TASK_WRITE_CAN,      ///< 写入CAN-ID
    FLASH_TASK_ERASE_ALL,      ///< 擦除所有
    FLASH_TASK_COUNT
} flash_task_type_t;

/**
 * @brief Flash任务请求结构
 */
typedef struct __attribute__((packed))
{
    flash_task_type_t type; ///< 任务类型
    void *data;             ///< 指向要写入的数据
    size_t size;            ///< 数据大小
} flash_task_request_t;

/**
 * @brief Flash任务队列管理器（基于kqueue）
 */
typedef struct __attribute__((packed))
{
    kqueue_t *queue;        ///< kqueue队列句柄
    uint32_t pending_count; ///< 等待执行的任务数
    uint8_t idle_threshold; ///< CPU空闲阈值(%)
} flash_task_queue_t;

/*============================================================================
 * 公共API声明
 *============================================================================*/

/**
 * @brief 初始化Flash任务队列
 * @return 成功返回0，失败返回-1
 */
int flash_task_init(void);

/**
 * @brief 请求Flash任务（异步）
 * @param type 任务类型
 * @param data 指向要写入的数据
 * @param size 数据大小
 * @note 任务会被放入队列，等待flash_task_process()执行
 */
void flash_task_request(flash_task_type_t type, void *data, size_t size);

/**
 * @brief 处理Flash任务（低优先级任务中调用）
 * @note 仅在CPU空闲率足够时执行Flash写入
 */
void flash_task_process(void);

/**
 * @brief 获取等待执行的任务数
 * @return 等待执行的任务数量
 */
uint32_t flash_task_pending_count(void);

/**
 * @brief 获取任务队列实例
 * @return 任务队列指针
 */
flash_task_queue_t *flash_task_get_instance(void);

/**
 * @brief 销毁Flash任务队列（释放资源）
 */
void flash_task_destroy(void);

#endif /* FLASH_TASK_H */
