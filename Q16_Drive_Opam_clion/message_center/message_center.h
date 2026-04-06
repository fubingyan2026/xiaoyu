//
// Created by fubingyan on 26-4-6.
//

#ifndef __MESSAGE_CENTER_H
#define __MESSAGE_CENTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 消息中心错误码枚举
 */
typedef enum {
  MESSAGE_CENTER_OK = 0,              /**< 操作成功 */
  MESSAGE_CENTER_OK_EXISTED,          /**< 成功，但对象已存在 */
  MESSAGE_CENTER_ERROR_NULL_PTR,      /**< 空指针错误 */
  MESSAGE_CENTER_ERROR_INVALID_PARAM, /**< 无效参数 */
  MESSAGE_CENTER_ERROR_NO_MEMORY,     /**< 内存分配失败 */
  MESSAGE_CENTER_ERROR_NOT_FOUND,     /**< 未找到对象 */
  MESSAGE_CENTER_ERROR_ALREADY_EXIST, /**< 对象已存在 */
  MESSAGE_CENTER_ERROR_INTERNAL,      /**< 内部错误 */
  MESSAGE_CENTER_ERROR_UNINITIALIZED, /**< 未初始化 */
} message_center_error_t;

/**
 * @brief 订阅者结构体前向声明
 */
typedef struct message_center_subscriber message_center_subscriber_t;

/**
 * @brief 发布者结构体前向声明
 */
typedef struct message_center_publisher message_center_publisher_t;

/**
 * @brief 订阅者结构体
 * @details 每个订阅者维护自己的读取进度，不独立持有数据队列
 */
struct message_center_subscriber {
  message_center_publisher_t* topic;            /**< 所属的话题（发布者）引用 */
  uint32_t read_count;                          /**< 该订阅者已读取的消息总数 */
  uint16_t data_len;                            /**< 消息数据长度（校验用） */
  message_center_subscriber_t* next_subs_queue; /**< 指向下一个订阅者 */
};

/**
 * @brief 可配置参数：系统支持的最大话题数量
 */
#ifndef MESSAGE_CENTER_MAX_TOPICS
#define MESSAGE_CENTER_MAX_TOPICS 32
#endif

/**
 * @brief 可配置参数：话题名称的最大长度（包含结束符）
 */
#ifndef MESSAGE_CENTER_MAX_TOPIC_NAME_LEN
#define MESSAGE_CENTER_MAX_TOPIC_NAME_LEN 32
#endif

/**
 * @brief 可配置参数：默认共享队列深度
 * @note 建议使用 2 的幂次方（如 4, 8, 16），以便编译器优化取模运算
 */
#ifndef MESSAGE_CENTER_DEFAULT_QUEUE_SIZE
#define MESSAGE_CENTER_DEFAULT_QUEUE_SIZE 16
#endif

/**
 * @brief 消息中心配置结构体
 */
typedef struct {
  const char* name;            /**< 话题名称 */
  uint16_t max_topics;         /**< 最大话题数量 */
  uint16_t queue_size;         /**< 默认队列深度 */
  uint16_t max_topic_name_len; /**< 话题名称最大长度 */
  uint16_t data_len;           /**< 消息数据长度 */
} message_center_config_t;

/**
 * @brief 发布者结构体
 * @details 发布者负责维护物理存储队列，所有订阅者共享该队列
 */
struct message_center_publisher {
  void* queue[MESSAGE_CENTER_DEFAULT_QUEUE_SIZE]; /**< 共享的环形 FIFO 队列 */
  uint32_t write_count;                           /**< 总发布消息计数器 */
  message_center_subscriber_t* first_subs; /**< 指向第一个订阅者链表的首节点 */
  message_center_publisher_t* next_topic_node; /**< 指向下一个发布者 */
  message_center_config_t config;              /**< 配置参数 */
};

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/** @brief 检查操作是否成功 */
#define MESSAGE_CENTER_IS_OK(err) ((err) >= 0)

/** @brief 检查操作是否失败 */
#define MESSAGE_CENTER_IS_ERR(err) ((err) < 0)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 注册发布者实例
 * @param publisher 输出参数，返回创建的发布者实例指针
 * @param config 配置结构体
 * @return 操作结果错误码
 */
message_center_error_t message_center_publisher_register(
    message_center_publisher_t** publisher, message_center_config_t config);

/**
 * @brief 注册订阅者
 * @param publisher 发布者实例指针
 * @param subscriber 输出的订阅者实例指针
 * @return 操作结果错误码
 */
message_center_error_t message_center_subscriber_register(
    message_center_publisher_t* publisher,
    message_center_subscriber_t** subscriber);

/**
 * @brief 订阅者获取消息（FIFO 出队）
 * @param subscriber 订阅者实例指针
 * @param data_ptr 接收消息数据的缓冲区指针
 * @return 1: 成功获取新消息，0: 无新消息或参数错误
 */
uint8_t message_center_subscriber_get_message(
    message_center_subscriber_t* subscriber, void* data_ptr);

/**
 * @brief 发布消息（写入共享环形队列）
 * @param publisher 发布者实例指针
 * @param data_ptr 要发布的消息数据指针
 * @return 当前该话题拥有的订阅者数量
 */
uint8_t message_center_publisher_push_message(
    message_center_publisher_t* publisher, const void* data_ptr);

/**
 * @brief 注销发布者及其所有订阅者
 * @param publisher 发布者实例指针
 * @return 操作结果错误码
 */
message_center_error_t message_center_unregister(
    message_center_publisher_t* publisher);

/**
 * @brief 按话题名称查找发布者实例
 * @param name 话题名称
 * @return 发布者实例指针，未找到返回 NULL
 */
message_center_publisher_t* message_center_find_publisher(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_CENTER_H */
