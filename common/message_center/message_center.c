/**
 * @file message_center.c
 * @author NeoZeng neozeng1@hnu.edu.cn, fubingyan qq:3245784484
 * @brief 发布-订阅（Pub/Sub）消息中心 (共享环形队列重构版) 核心实现
 * @version 3.0.0
 * @date 2025-03-25
 *
 * @copyright Copyright (c) 2025 fubingyan, All Rights Reserved.
 */

#include "message_center/message_center.h"
#include "debug/debug.h"
#include "memory_pool/memory_pool.h"

/* 调试打印宏，默认关闭 */
#define MSG_PRINTF(...) // DEBUG_DEBUG(__VA_ARGS__)

/** @brief 话题管理链表头 */
static Publisher_t *topic_list = NULL;
/** @brief 已注册的话题总数 */
static uint16_t topic_count = 0;
/** @brief 系统初始化标志 */
static uint8_t message_system_initialized = 0;

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 检查名称长度是否符合要求
 * @param name 待检查的名称字符串
 * @return uint8_t 1: 超过限制, 0: 符合要求
 */
static inline uint8_t CheckName(const char *name)
{
    return (__strnlen(name, MSG_MAX_TOPIC_NAME_LEN + 1) >= MSG_MAX_TOPIC_NAME_LEN) ? 1 : 0;
}

/**
 * @brief 内部查找话题发布者
 * @param name 话题名称
 * @return Publisher_t* 话题指针，未找到返回 NULL
 */
static Publisher_t *FindPublisher(const char *name)
{
    if (name == NULL)
        return NULL;
    Publisher_t *node = topic_list;
    while (node != NULL)
    {
        if (__strcmp(node->topic_name, name) == 0)
            return node;
        node = node->next_topic_node;
    }
    return NULL;
}

/* ==================== 系统管理API ==================== */

/**
 * @brief 初始化消息中心系统
 */
msg_error_e MessageCenterInit(void)
{
    if (message_system_initialized)
        return MSG_OK_EXISTED;
    topic_list = NULL;
    topic_count = 0;
    message_system_initialized = 1;
    return MSG_OK;
}

/**
 * @brief 反初始化消息中心系统，释放所有动态分配的资源
 */
void MessageCenterDeinit(void)
{
    if (!message_system_initialized)
        return;

    Publisher_t *current = topic_list;
    while (current != NULL)
    {
        Publisher_t *next = current->next_topic_node;

        /* 1. 释放该话题下的所有订阅者节点 */
        Subscriber_t *sub_current = current->first_subs;
        while (sub_current != NULL)
        {
            Subscriber_t *sub_next = sub_current->next_subs_queue;
            if (!sub_current->is_static)
            {
                /* 订阅者不再持有私有队列，直接释放结构体 */
                __free(sub_current);
            }
            sub_current = sub_next;
        }

        /* 2. 释放发布者持有的共享队列内存块 */
        if (!current->is_static)
        {
            for (uint8_t i = 0; i < MSG_DEFAULT_QUEUE_SIZE; ++i)
            {
                if (current->queue[i] != NULL)
                {
                    __free(current->queue[i]);
                    current->queue[i] = NULL;
                }
            }
            /* 3. 释放发布者结构体本身 */
            __free(current);
        }

        current = next;
    }

    topic_list = NULL;
    topic_count = 0;
    message_system_initialized = 0;
}

/* ==================== 发布者API ==================== */

/**
 * @brief 动态注册发布者实例
 */
Publisher_t *PubRegister(const char *name, const uint16_t data_len)
{
    /* 参数合法性检查 */
    if (name == NULL || data_len == 0 || CheckName(name))
        return NULL;

    /* 自动初始化检测 */
    if (!message_system_initialized)
        MessageCenterInit();

    /* 容量限制检查 */
    if (topic_count >= MSG_MAX_TOPICS)
        return NULL;

    /* 检查是否已存在同名话题，若存在则直接返回旧实例 */
    Publisher_t *node = FindPublisher(name);
    if (node != NULL)
        return node;

    /* 分配发布者结构体内存 */
    Publisher_t *new_topic = (Publisher_t *)__malloc(sizeof(Publisher_t));
    if (new_topic == NULL)
        return NULL;

    __memset(new_topic, 0, sizeof(Publisher_t));
    new_topic->data_len = data_len;
    new_topic->topic_name = name;

    /* 为该话题分配物理存储空间（共享环形队列） */
    uint8_t alloc_success = 1;
    for (uint8_t i = 0; i < MSG_DEFAULT_QUEUE_SIZE; ++i)
    {
        new_topic->queue[i] = __malloc(data_len);
        if (new_topic->queue[i] == NULL)
            alloc_success = 0;
    }

    /* 内存分配回滚处理 */
    if (!alloc_success)
    {
        for (uint8_t j = 0; j < MSG_DEFAULT_QUEUE_SIZE; ++j)
        {
            if (new_topic->queue[j] != NULL)
                __free(new_topic->queue[j]);
        }
        __free(new_topic);
        return NULL;
    }

    /* 采用头插法将话题加入全局链表 */
    new_topic->next_topic_node = topic_list;
    topic_list = new_topic;
    topic_count++;
    return new_topic;
}

/**
 * @brief 静态注册发布者实例
 */
msg_error_e PubRegisterStatic(const char *name, const uint16_t data_len, Publisher_t *instance)
{
    if (name == NULL || data_len == 0 || instance == NULL || CheckName(name))
        return MSG_ERR_INVALID_PARAM;

    if (!message_system_initialized)
        MessageCenterInit();

    if (topic_count >= MSG_MAX_TOPICS)
        return MSG_ERR_NO_MEMORY;

    if (FindPublisher(name) != NULL)
        return MSG_ERR_ALREADY_EXIST;

    /* 初始化静态内存块，注意：静态发布者需用户预先为其 queue 成员分配内存或留空 */
    __memset(instance, 0, sizeof(Publisher_t));
    instance->data_len = data_len;
    instance->topic_name = name;
    instance->is_static = 1;

    /* 加入链表 */
    instance->next_topic_node = topic_list;
    topic_list = instance;
    topic_count++;
    return MSG_OK;
}

/* ==================== 订阅者API ==================== */

/**
 * @brief 动态注册订阅者实例
 */
Subscriber_t *SubRegister(const char *name, const uint16_t data_len)
{
    if (name == NULL || data_len == 0 || CheckName(name))
        return NULL;

    if (!message_system_initialized)
        MessageCenterInit();

    /* 自动创建或查找对应的话题发布者 */
    Publisher_t *pub = PubRegister(name, data_len);
    if (pub == NULL)
        return NULL;

    /* 分配订阅者节点内存 */
    Subscriber_t *ret = (Subscriber_t *)__malloc(sizeof(Subscriber_t));
    if (ret == NULL)
        return NULL;

    __memset(ret, 0, sizeof(Subscriber_t));
    ret->data_len = data_len;
    ret->topic = pub;
    /* 核心策略：订阅时同步当前发布进度，即“不接收历史旧消息”，只收订阅后的新消息 */
    ret->read_count = pub->write_count;
    ret->is_static = 0;

    /* 加入订阅者链表 */
    ret->next_subs_queue = pub->first_subs;
    pub->first_subs = ret;
    return ret;
}

/**
 * @brief 静态注册订阅者实例
 */
msg_error_e SubRegisterStatic(const char *name, const uint16_t data_len, Subscriber_t *instance)
{
    if (name == NULL || data_len == 0 || instance == NULL || CheckName(name))
        return MSG_ERR_INVALID_PARAM;

    if (!message_system_initialized)
        MessageCenterInit();

    Publisher_t *pub = PubRegister(name, data_len);
    if (pub == NULL)
        return MSG_ERR_INTERNAL;

    __memset(instance, 0, sizeof(Subscriber_t));
    instance->data_len = data_len;
    instance->topic = pub;
    instance->read_count = pub->write_count;
    instance->is_static = 1;

    instance->next_subs_queue = pub->first_subs;
    pub->first_subs = instance;
    return MSG_OK;
}

/* ==================== 消息收发API ==================== */

/**
 * @brief 发布消息（写入共享环形队列）
 */
uint8_t PubPushMessage(Publisher_t *pub, const void *data_ptr)
{
    if (pub == NULL || data_ptr == NULL)
        return 0;

    /* 计算写入索引：对深度取模实现环形写入 */
    uint32_t idx = pub->write_count % MSG_DEFAULT_QUEUE_SIZE;
    if (pub->queue[idx] != NULL)
    {
        /* 仅执行 1 次数据拷贝动作，极大节省开销 */
        __memcpy(pub->queue[idx], data_ptr, pub->data_len);
    }

    /* 全局发布进度递增（无符号溢出自然回绕，配合模运算安全） */
    pub->write_count++;

    /* 统计并返回订阅者数量，兼容旧代码逻辑 */
    uint8_t sub_count = 0;
    Subscriber_t *iter = pub->first_subs;
    while (iter != NULL)
    {
        sub_count++;
        iter = iter->next_subs_queue;
    }
    return sub_count;
}

/**
 * @brief 订阅者获取消息（基于读取进度的偏移计算）
 */
uint8_t SubGetMessage(Subscriber_t *sub, void *data_ptr)
{
    if (sub == NULL || data_ptr == NULL || sub->topic == NULL)
        return 0;

    Publisher_t *pub = sub->topic;

    /* 检查是否有新消息：若个人读进度追平了发布进进度，说明无新消息 */
    if (sub->read_count == pub->write_count)
    {
        return 0;
    }

    /*  脏读/覆盖检查：
     *    由于共享队列深度有限，如果订阅者处理太慢，其 read_count 落后于 write_count 超过深度时，
     *    最老的消息已被覆盖。此时必须将进度强制推移到当前队列中最老的那一帧（丢弃已丢失数据）。
     *    注：利用无符号减法，天然兼容 32 位溢出回绕。
     */
    if (pub->write_count - sub->read_count > MSG_DEFAULT_QUEUE_SIZE)
    {
        sub->read_count = pub->write_count - MSG_DEFAULT_QUEUE_SIZE;
    }

    /*  读取数据 */
    uint32_t idx = sub->read_count % MSG_DEFAULT_QUEUE_SIZE;
    if (pub->queue[idx] == NULL)
    {
        return 0; /* 健壮性保护：防止静态发布者未正确挂载内存 */
    }

    __memcpy(data_ptr, pub->queue[idx], sub->data_len);

    /* 推进订阅者个人读取进度 */
    sub->read_count++;
    return 1;
}

/* ==================== 实例管理API ==================== */

/**
 * @brief 注销指定话题及其关联的所有订阅者
 */
msg_error_e MessageCenterUnregister(const char *name)
{
    if (name == NULL || !message_system_initialized)
        return MSG_ERR_NOT_FOUND;

    Publisher_t **prev = &topic_list;
    Publisher_t *current = topic_list;

    while (current != NULL)
    {
        if (__strcmp(current->topic_name, name) == 0)
        {
            *prev = current->next_topic_node;

            /* 释放订阅者链表 */
            Subscriber_t *sub_current = current->first_subs;
            while (sub_current != NULL)
            {
                Subscriber_t *sub_next = sub_current->next_subs_queue;
                if (!sub_current->is_static)
                {
                    __free(sub_current);
                }
                sub_current = sub_next;
            }

            /* 释放发布者共享队列及结构体 */
            if (!current->is_static)
            {
                for (uint8_t i = 0; i < MSG_DEFAULT_QUEUE_SIZE; ++i)
                {
                    if (current->queue[i] != NULL)
                    {
                        __free(current->queue[i]);
                    }
                }
                __free(current);
            }
            topic_count--;
            return MSG_OK;
        }
        prev = &current->next_topic_node;
        current = current->next_topic_node;
    }
    return MSG_ERR_NOT_FOUND;
}

/** @brief 获取当前话题总数 */
uint16_t MessageCenterGetCount(void) { return topic_count; }

/** @brief 获取指定话题的发布者指针 */
Publisher_t *MessageCenterGetPublisher(const char *name) { return FindPublisher(name); }

/** @brief 获取指定话题的第一个订阅者指针 */
Subscriber_t *MessageCenterGetSubscriber(const char *name)
{
    Publisher_t *pub = FindPublisher(name);
    return (pub != NULL) ? pub->first_subs : NULL;
}
