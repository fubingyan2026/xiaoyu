//
// Created by fubingyan on 26-4-6.
//

/**
 * @file    message_center.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2026-04-06
 * @brief   发布 - 订阅（Pub/Sub）消息中心核心实现 - 去中心化架构
 * @attention
 *
 * Copyright (c) 2026 fubingyan.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "message_center.h"

#include <stdlib.h>
#include <string.h>

#include "public.h"

/* Private constants ---------------------------------------------------------*/

/* 调试打印宏，默认关闭 */
#define MSG_PRINTF(...)

/* Private variables ---------------------------------------------------------*/

// 全局发布者链表
static CLIST_HEAD(g_topic_list);

/* Private function prototypes -----------------------------------------------*/

static message_center_error_t check_name_length(const char* name,
    uint16_t max_len);
static bool is_valid_queue_size(uint16_t size);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 注册发布者实例
 */
message_center_error_t message_center_publisher_register(
    message_center_publisher_t** publisher, message_center_config_t config)
{
    // 检查参数有效性
    if (!publisher) {
        return MESSAGE_CENTER_ERROR_NULL_PTR;
    }

    if (config.name == NULL || config.data_len == 0) {
        return MESSAGE_CENTER_ERROR_INVALID_PARAM;
    }

    // 参数默认值
    if (config.queue_size == 0) {
        config.queue_size = MESSAGE_CENTER_DEFAULT_QUEUE_SIZE;
    }
    if (config.max_topic_name_len == 0) {
        config.max_topic_name_len = MESSAGE_CENTER_MAX_TOPIC_NAME_LEN;
    }

    // 验证队列大小
    if (!is_valid_queue_size(config.queue_size)) {
        return MESSAGE_CENTER_ERROR_INVALID_PARAM;
    }

    // 检查名称长度
    if (check_name_length(config.name, config.max_topic_name_len) != MESSAGE_CENTER_OK) {
        return MESSAGE_CENTER_ERROR_INVALID_PARAM;
    }

    // 检查是否已存在同名话题
    MESSAGE_CENTER_ENTER_CRITICAL();
    if (message_center_find_publisher(config.name) != NULL) {
        MESSAGE_CENTER_EXIT_CRITICAL();
        return MESSAGE_CENTER_ERROR_ALREADY_EXIST;
    }
    MESSAGE_CENTER_EXIT_CRITICAL();

    // 分配发布者结构体内存
    message_center_publisher_t* new_topic = (message_center_publisher_t*)__malloc(sizeof(message_center_publisher_t));
    if (new_topic == NULL) {
        return MESSAGE_CENTER_ERROR_NO_MEMORY;
    }

    // 初始化发布者（必须在分配队列之前）
    memset(new_topic, 0, sizeof(message_center_publisher_t));
    clist_init(&new_topic->subs_list);
    new_topic->config = config;

    // 分配队列内存
    bool alloc_success = true;
    for (uint8_t i = 0; i < config.queue_size; ++i) {
        new_topic->queue[i] = __malloc(config.data_len);
        if (new_topic->queue[i] == NULL) {
            alloc_success = false;
        }
    }

    // 内存分配回滚处理
    if (!alloc_success) {
        for (uint8_t j = 0; j < config.queue_size; ++j) {
            if (new_topic->queue[j] != NULL) {
                __free(new_topic->queue[j]);
            }
        }
        __free(new_topic);
        return MESSAGE_CENTER_ERROR_NO_MEMORY;
    }

    // 加入全局链表
    MESSAGE_CENTER_ENTER_CRITICAL();
    clist_add(&g_topic_list, &new_topic->topic_node);
    MESSAGE_CENTER_EXIT_CRITICAL();

    *publisher = new_topic;
    return MESSAGE_CENTER_OK;
}

/**
 * @brief 注册订阅者
 */
message_center_error_t message_center_subscriber_register(
    message_center_publisher_t* publisher,
    message_center_subscriber_t** subscriber)
{
    // 检查参数有效性
    if (!publisher || !subscriber) {
        return MESSAGE_CENTER_ERROR_NULL_PTR;
    }

    // 分配订阅者节点内存
    message_center_subscriber_t* ret = (message_center_subscriber_t*)__malloc(
        sizeof(message_center_subscriber_t));
    if (ret == NULL) {
        return MESSAGE_CENTER_ERROR_NO_MEMORY;
    }

    memset(ret, 0, sizeof(message_center_subscriber_t));
    ret->data_len = publisher->config.data_len;
    ret->topic = publisher;
    ret->read_count = publisher->write_count;

    // 加入订阅者链表
    MESSAGE_CENTER_ENTER_CRITICAL();
    clist_add(&publisher->subs_list, &ret->subs_node);
    MESSAGE_CENTER_EXIT_CRITICAL();

    *subscriber = ret;
    return MESSAGE_CENTER_OK;
}

/**
 * @brief 订阅者获取消息（基于读取进度的偏移计算）
 */
uint8_t message_center_subscriber_get_message(
    message_center_subscriber_t* subscriber, void* data_ptr)
{
    if (!subscriber || !data_ptr || !subscriber->topic) {
        return 0;
    }

    message_center_publisher_t* pub = subscriber->topic;

    MESSAGE_CENTER_ENTER_CRITICAL();

    // 检查是否有新消息
    if (subscriber->read_count == pub->write_count) {
        MESSAGE_CENTER_EXIT_CRITICAL();
        return 0;
    }

    // 脏读/覆盖检查
    if (pub->write_count - subscriber->read_count > pub->config.queue_size) {
        subscriber->read_count = pub->write_count - pub->config.queue_size;
    }

    // 读取数据
    uint32_t idx = subscriber->read_count % pub->config.queue_size;
    if (pub->queue[idx] == NULL) {
        MESSAGE_CENTER_EXIT_CRITICAL();
        return 0;
    }

    memcpy(data_ptr, pub->queue[idx], subscriber->data_len);

    // 推进订阅者个人读取进度
    subscriber->read_count++;

    MESSAGE_CENTER_EXIT_CRITICAL();
    return 1;
}

/**
 * @brief 发布消息（写入共享环形队列）
 */
uint8_t message_center_publisher_push_message(
    message_center_publisher_t* publisher, const void* data_ptr)
{
    if (!publisher || !data_ptr) {
        return 0;
    }

    MESSAGE_CENTER_ENTER_CRITICAL();

    // 计算写入索引
    uint32_t idx = publisher->write_count % publisher->config.queue_size;
    if (publisher->queue[idx] != NULL) {
        memcpy(publisher->queue[idx], data_ptr, publisher->config.data_len);
    }

    // 全局发布进度递增
    publisher->write_count++;

    // 统计并返回订阅者数量
    uint8_t sub_count = 0;
    message_center_subscriber_t* iter;
    clist_for_each_entry(iter, &publisher->subs_list, subs_node)
    {
        sub_count++;
    }

    MESSAGE_CENTER_EXIT_CRITICAL();
    return sub_count;
}

/**
 * @brief 注销发布者及其所有订阅者
 */
message_center_error_t message_center_unregister(
    message_center_publisher_t* publisher)
{
    if (!publisher) {
        return MESSAGE_CENTER_ERROR_NULL_PTR;
    }

    MESSAGE_CENTER_ENTER_CRITICAL();

    // 从全局链表中移除
    clist_del(&publisher->topic_node);

    // 释放订阅者链表
    message_center_subscriber_t *sub, *tmp;
    clist_for_each_entry_safe(sub, tmp, &publisher->subs_list, subs_node)
    {
        __free(sub);
    }

    MESSAGE_CENTER_EXIT_CRITICAL();

    // 释放发布者共享队列及结构体
    for (uint8_t i = 0; i < publisher->config.queue_size; ++i) {
        if (publisher->queue[i] != NULL) {
            __free(publisher->queue[i]);
        }
    }
    __free(publisher);

    return MESSAGE_CENTER_OK;
}

/**
 * @brief 按话题名称查找发布者实例
 */
message_center_publisher_t* message_center_find_publisher(const char* name)
{
    if (!name) {
        return NULL;
    }

    MESSAGE_CENTER_ENTER_CRITICAL();

    message_center_publisher_t* pub;
    clist_for_each_entry(pub, &g_topic_list, topic_node)
    {
        if (strcmp(pub->config.name, name) == 0) {
            MESSAGE_CENTER_EXIT_CRITICAL();
            return pub;
        }
    }

    MESSAGE_CENTER_EXIT_CRITICAL();
    return NULL;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 验证队列大小是否有效
 */
static bool is_valid_queue_size(uint16_t size)
{
    if (size == 0 || size > MESSAGE_CENTER_DEFAULT_QUEUE_SIZE) {
        return false;
    }
    return (size & (size - 1)) == 0;
}

/**
 * @brief 检查话题名称长度是否有效
 */
static message_center_error_t check_name_length(const char* name,
    uint16_t max_len)
{
    if (!name) {
        return MESSAGE_CENTER_ERROR_NULL_PTR;
    }
    if (strlen(name) >= max_len) {
        return MESSAGE_CENTER_ERROR_INVALID_PARAM;
    }
    return MESSAGE_CENTER_OK;
}
