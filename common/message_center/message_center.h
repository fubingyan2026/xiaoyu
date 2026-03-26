/**
 * @file message_center.h
 * @author NeoZeng neozeng1@hnu.edu.cn, fubingyan qq:3245784484
 * @brief 发布-订阅（Pub/Sub）消息中心 (共享环形队列重构版)
 * @details 本模块提供了一个轻量级的消息中间件，支持多个订阅者共享发布者的环形队列，
 *          极大地降低了内存占用和 CPU 拷贝开销。支持静态和动态两种注册方式。
 * @version 3.0.0
 * @date 2025-03-25
 *
 * @copyright Copyright (c) 2025 fubingyan, All Rights Reserved.
 */

#ifndef MESSAGE_CENTER_H
#define MESSAGE_CENTER_H

#include "stdint.h"

/**
 * @brief 消息中心错误码枚举
 */
typedef enum
{
    MSG_OK = 0,                 /**< 成功 */
    MSG_OK_EXISTED = 1,         /**< 成功，但对象已存在 */
    MSG_ERR_INVALID_PARAM = -1, /**< 无效参数 */
    MSG_ERR_NO_MEMORY = -2,     /**< 内存分配失败 */
    MSG_ERR_NOT_FOUND = -3,     /**< 未找到对象 */
    MSG_ERR_ALREADY_EXIST = -4, /**< 对象已存在 */
    MSG_ERR_INTERNAL = -5,      /**< 内部错误 */
} msg_error_e;

/** @brief 检查操作是否成功 */
#define MSG_IS_OK(err) ((err) >= 0)
/** @brief 检查操作是否失败 */
#define MSG_IS_ERR(err) ((err) < 0)

/* ==================== 可配置参数 ==================== */

/** @brief 系统支持的最大话题数量 */
#ifndef MSG_MAX_TOPICS
#define MSG_MAX_TOPICS 32
#endif

/** @brief 话题名称的最大长度（包含结束符） */
#ifndef MSG_MAX_TOPIC_NAME_LEN
#define MSG_MAX_TOPIC_NAME_LEN 32
#endif

/**
 * @brief 默认共享队列深度
 * @note 建议使用2的幂次方（如 4, 8, 16），以便编译器优化取模运算。
 */
#ifndef MSG_DEFAULT_QUEUE_SIZE
#define MSG_DEFAULT_QUEUE_SIZE 4
#endif

/* ==================== 前向声明 ==================== */
struct ent;

/**
 * @brief 订阅者结构体
 * @details 每个订阅者维护自己的读取进度，不独立持有数据队列。
 */
typedef struct mqt
{
    struct ent *topic;           /**< 所属的话题（发布者）引用 */
    uint32_t read_count;         /**< 该订阅者已读取的消息总数（用于计算进度） */
    uint16_t data_len;           /**< 消息数据长度（校验用） */
    uint8_t is_static : 1;       /**< 标记是否为静态分配（1:静态, 0:动态） */
    struct mqt *next_subs_queue; /**< 指向下一个订阅者（同一话题的链表节点） */
} Subscriber_t;

/**
 * @brief 发布者（话题）结构体
 * @details 发布者负责维护物理存储队列，所有订阅者共享该队列。
 */
typedef struct ent
{
    const char *topic_name;              /**< 话题名称 */
    uint16_t data_len;                   /**< 消息数据长度 */
    uint8_t is_static : 1;               /**< 标记是否为静态分配（1:静态, 0:动态） */
    void *queue[MSG_DEFAULT_QUEUE_SIZE]; /**< 共享的环形FIFO队列 */
    uint32_t write_count;                /**< 总发布消息计数器 */
    Subscriber_t *first_subs;            /**< 指向第一个订阅者链表的首节点 */
    struct ent *next_topic_node;         /**< 指向下一个发布者（话题管理链表节点） */
} Publisher_t;

/* ==================== API声明 ==================== */

/**
 * @brief 初始化消息中心系统
 * @return msg_error_e 错误码
 */
msg_error_e MessageCenterInit(void);

/**
 * @brief 反初始化消息中心系统，释放所有资源
 */
void MessageCenterDeinit(void);

/**
 * @brief 动态注册发布者
 * @param name 话题名称
 * @param data_len 消息数据长度
 * @return Publisher_t* 发布者实例指针，失败返回 NULL
 */
Publisher_t *PubRegister(const char *name, uint16_t data_len);

/**
 * @brief 静态注册发布者
 * @param name 话题名称
 * @param data_len 消息数据长度
 * @param instance 用户提供的发布者结构体实例内存
 * @return msg_error_e 错误码
 */
msg_error_e PubRegisterStatic(const char *name, uint16_t data_len, Publisher_t *instance);

/**
 * @brief 动态注册订阅者
 * @param name 要订阅的话题名称
 * @param data_len 期望的消息数据长度
 * @return Subscriber_t* 订阅者实例指针，失败返回 NULL
 */
Subscriber_t *SubRegister(const char *name, uint16_t data_len);

/**
 * @brief 静态注册订阅者
 * @param name 要订阅的话题名称
 * @param data_len 期望的消息数据长度
 * @param instance 用户提供的订阅者结构体实例内存
 * @return msg_error_e 错误码
 */
msg_error_e SubRegisterStatic(const char *name, uint16_t data_len, Subscriber_t *instance);

/**
 * @brief 订阅者获取消息（FIFO出队）
 * @param sub 订阅者实例指针
 * @param data_ptr 接收消息数据的缓冲区指针
 * @return uint8_t 1: 成功获取新消息, 0: 无新消息或参数错误
 */
uint8_t SubGetMessage(Subscriber_t *sub, void *data_ptr);

/**
 * @brief 发布消息（写入共享环形队列）
 * @param pub 发布者实例指针
 * @param data_ptr 要发布的消息数据指针
 * @return uint8_t 当前该话题拥有的订阅者数量
 */
uint8_t PubPushMessage(Publisher_t *pub, const void *data_ptr);

/**
 * @brief 注销话题及其所有订阅者
 * @param name 话题名称
 * @return msg_error_e 错误码
 */
msg_error_e MessageCenterUnregister(const char *name);

/**
 * @brief 获取系统中已注册的话题总数
 * @return uint16_t 话题数量
 */
uint16_t MessageCenterGetCount(void);

/**
 * @brief 按话题名称查找发布者实例
 * @param name 话题名称
 * @return Publisher_t* 发布者实例指针，未找到返回 NULL
 */
Publisher_t *MessageCenterGetPublisher(const char *name);

/**
 * @brief 按话题名称查找该话题的第一个订阅者
 * @param name 话题名称
 * @return Subscriber_t* 订阅者实例指针，未找到返回 NULL
 */
Subscriber_t *MessageCenterGetSubscriber(const char *name);

#endif /* MESSAGE_CENTER_H */