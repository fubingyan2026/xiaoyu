//
// Created by fubingyan on 25-4-5.
//

#ifndef __DAEMON_H
#define __DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 守护进程错误码枚举
 */
typedef enum {
  DAEMON_OK = 0,                           /**< 操作成功 */
  DAEMON_OK_EXISTED,                       /**< 已存在 */
  DAEMON_ERROR_NULL_PTR,                   /**< 空指针错误 */
  DAEMON_ERROR_INVALID_PARAM,              /**< 无效参数 */
  DAEMON_ERROR_NO_MEMORY,                  /**< 内存不足 */
  DAEMON_ERROR_NOT_FOUND,                  /**< 未找到 */
  DAEMON_ERROR_ALREADY_EXIST,              /**< 已存在 */
  DAEMON_ERROR_UNINITIALIZED,              /**< 未初始化 */
  DAEMON_ERROR_GENERIC,                    /**< 通用错误 */
} daemon_error_t;

/**
 * @brief 离线回调函数类型
 * @param owner_ptr 拥有者指针
 */
typedef void (*daemon_offline_cb_t)(void* owner_ptr);

/**
 * @brief 获取系统时间戳回调函数类型
 * @return 系统时间戳(毫秒)
 */
typedef uint32_t (*daemon_get_time_cb_t)(void);

/**
 * @brief 守护进程配置结构体
 */
typedef struct {
  const char* name;                        /**< 守护进程名称 */
  void* owner_ptr;                         /**< 拥有者指针 */
  daemon_offline_cb_t offline_cb;          /**< 离线回调函数 */
  uint16_t reload_timeout_ms;              /**< 超时时间(ms)，0xFFFF或0表示永不超时 */
  uint16_t init_wait_time_ms;              /**< 初始化等待时间(ms) */
} daemon_config_t;

/**
 * @brief 守护进程上下文结构体前向声明
 */
typedef struct daemon_context daemon_context_t;

/**
 * @brief 守护进程上下文结构体
 */
struct daemon_context {
  daemon_config_t config;                  /**< 配置参数 */

  uint32_t last_feed_time;                 /**< 上次喂狗时间 */
  uint32_t current_feed_time;              /**< 当前喂狗时间 */
  float feed_frequency;                    /**< 喂狗频率(Hz) */
  uint32_t init_wait_counter;              /**< 初始化等待计数器 */
  bool online;                             /**< 在线状态 */
  bool online_last;                        /**< 上次在线状态 */
  uint8_t rx_counter;                      /**< 接收计数器 */
  bool is_static;                          /**< 静态分配标志 */
  daemon_context_t* next;                  /**< 链表下一个节点 */
};

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

#define DAEMON_STABLE_TIMES_MS 10          /**< 稳定计数阈值(毫秒) */

#define DAEMON_IS_OK(err) ((err) >= 0)     /**< 错误码检查宏 */
#define DAEMON_IS_ERR(err) ((err) < 0)     /**< 错误码检查宏 */

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化守护进程系统
 * @param get_time_cb 获取系统时间戳的回调函数
 * @return 操作结果错误码
 */
daemon_error_t daemon_init(daemon_get_time_cb_t get_time_cb);

/**
 * @brief 反初始化守护进程系统
 */
void daemon_deinit(void);

/**
 * @brief 注册守护进程(动态分配)
 * @param config 配置结构体指针
 * @return 守护进程上下文指针，失败返回NULL
 */
daemon_context_t* daemon_register(const daemon_config_t* config);

/**
 * @brief 注册守护进程(静态分配)
 * @param config 配置结构体指针
 * @param instance 预先分配的实例内存
 * @return 操作结果错误码
 */
daemon_error_t daemon_register_static(const daemon_config_t* config,
                                       daemon_context_t* instance);

/**
 * @brief 注销守护进程
 * @param name 守护进程名称
 * @return 操作结果错误码
 */
daemon_error_t daemon_unregister(const char* name);

/**
 * @brief 重新加载守护进程(喂狗)
 * @param ctx 守护进程上下文指针
 */
void daemon_reload(daemon_context_t* ctx);

/**
 * @brief 检查守护进程是否在线
 * @param ctx 守护进程上下文指针
 * @return true表示在线，false表示离线
 */
bool daemon_is_online(const daemon_context_t* ctx);

/**
 * @brief 获取守护进程名称
 * @param ctx 守护进程上下文指针
 * @return 守护进程名称，失败返回"not_found!"
 */
const char* daemon_get_name(const daemon_context_t* ctx);

/**
 * @brief 守护进程任务函数
 * @note 需在主循环或定时器中定期调用
 */
void daemon_task(void);

/**
 * @brief 获取守护进程实例
 * @param name 守护进程名称
 * @return 守护进程上下文指针，未找到返回NULL
 */
daemon_context_t* daemon_get_instance(const char* name);

/**
 * @brief 获取守护进程数量
 * @return 守护进程数量
 */
uint16_t daemon_get_count(void);

/**
 * @brief 获取守护进程主指针
 * @return 守护进程链表头指针
 */
daemon_context_t* daemon_get_master_pointer(void);

#ifdef __cplusplus
}
#endif

#endif /* __DAEMON_H */
