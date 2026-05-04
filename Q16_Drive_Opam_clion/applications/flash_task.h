/**
 * @file    flash_task.h
 * @brief   Flash延迟任务队列 - 基于message_center
 * @author  FOC Development Team
 * @date    2026-03-27
 */

#ifndef FLASH_TASK_H
#define FLASH_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "public.h"

#include "easyflash.h"
#include "encoder_alignment.h"
#include "hall_adjustment.h"

/*============================================================================
 * 宏定义
 *============================================================================*/

/**
 * @brief Flash存储的Magic名称定义
 * @note 使用宏定义代替数组，避免运行时开销和类型转换问题
 */
#define FLASH_MAGIC_ENCODER "FlashEncoder"
#define FLASH_MAGIC_HALL "FlashLineHall"
#define FLASH_MAGIC_CAN "FlashCanid"

/**
 * @brief 默认环境变量表大小
 */
#define FLASH_ENV_SET_SIZE 3

/*============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief Flash任务类型
 */
typedef enum __attribute__((packed)) {
  FLASH_TASK_WRITE_ENCODER = 0,  ///< 写入校准数据
  FLASH_TASK_WRITE_HALL,         ///< 写入霍尔参数
  FLASH_TASK_WRITE_CAN,          ///< 写入CAN-ID
  FLASH_TASK_ERASE_ALL,          ///< 擦除所有
  FLASH_TASK_COUNT
} flash_task_type_t;

extern motor_flash_config_t g_motor_flash_cfg;
extern hall_save_param_t hall_save_param;
extern uint32_t can_save_id;

/**
 * @brief   默认环境变量表
 * @note    这些变量会从Flash加载到RAM中
 */
extern const ef_env default_env_set[FLASH_ENV_SET_SIZE];

/**
 * @brief Flash任务请求结构
 */
typedef struct {
  flash_task_type_t type;  ///< 任务类型
  void* data;              ///< 指向要写入的数据
  size_t size;             ///< 数据大小
} flash_task_request_t;

/**
 * @brief Flash任务管理器（基于message_center）
 */
typedef struct {
  message_center_publisher_t* publisher;    ///< 消息发布者
  message_center_subscriber_t* subscriber;  ///< 消息订阅者
  uint32_t pending_count;                   ///< 等待执行的任务数
  uint8_t idle_threshold;                   ///< CPU空闲阈值(%)
} flash_task_mgr_t;

/*============================================================================
 * 公共API声明
 *============================================================================*/

/**
 * @brief 初始化Flash任务管理器
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
void flash_task_request(flash_task_type_t type, void* data, size_t size);

/**
 * @brief 处理Flash任务（低优先级任务中调用）
 * @note 仅在CPU空闲率足够时执行Flash写入
 */
void flash_task_process(void);

/**
 * @brief 获取等待执行的任务数
 * @return 等待执行的任务数量
 */
uint32_t flash_task_get_pending_count(void);

/**
 * @brief 获取任务管理器实例
 * @return 任务管理器指针
 */
flash_task_mgr_t* flash_task_get_instance(void);

/**
 * @brief 销毁Flash任务管理器（释放资源）
 */
void flash_task_destroy(void);

/**
 * @brief 获取默认环境变量表大小
 * @return 默认环境变量表大小
 * @note 供ef_port.c使用
 */
size_t flash_task_get_env_set_size(void);

#endif /* FLASH_TASK_H */
