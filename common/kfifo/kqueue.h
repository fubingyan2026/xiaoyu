/**
 * @brief: Generic Object Queue based on kfifo
 * @FilePath: kqueue.h
 * @author: Your Name / Based on fubingyan
 * @Date: 2026-02-04
 * @version: V1.0.0
 * @note: Thread-safe if underlying kfifo lock is provided.
 *        Supports fixed-size elements (structs, ints, etc).
 */

#ifndef KQUEUE_H
#define KQUEUE_H

#include "daemon/daemon.h"
#include "kfifo/kfifo.h"

#include <stdbool.h>

/* 错误码定义 */
typedef enum
{
    KQUEUE_OK = 0,
    KQUEUE_ERR_PARAM = -1,
    KQUEUE_ERR_MEM = -2,
    KQUEUE_ERR_FULL = -3,
    KQUEUE_ERR_EMPTY = -4,
    KQUEUE_ERR_TIMEOUT = -5,
} kqueue_err_t;

typedef struct
{
    const char *name;
    kfifo_t *fifo;          /* 底层 kfifo 句柄 */
    daemon_t *daemon;       /* 底层进程守护 句柄 */
    unsigned int item_size; /* 单个元素大小 */
    bool is_external_fifo;  /* 标记 fifo 是否为外部传入(目前默认为内部创建) */
} kqueue_t;

/**
 * @brief 创建队列
 * @param item_size 单个元素的大小(bytes)
 * @param max_items 期望容纳的最大元素个数 (会自动向上取整适应kfifo的2幂次特性)
 * @param lock 自旋锁指针 (可为NULL，若为NULL则需要用户保证线程安全或由kfifo内部机制决定)
 * @return 队列句柄，失败返回NULL
 */
kqueue_t *kqueue_register(const char *name, unsigned int item_size, unsigned int max_items, unsigned int time_out);

/**
 * @brief 销毁队列
 * @param q 队列句柄
 */
void kqueue_destroy(kqueue_t *q);

/**
 * @brief 入队一个元素
 * @param q 队列句柄
 * @param item 指向要入队元素的指针
 * @return KQUEUE_OK 成功, KQUEUE_ERR_FULL 满, 其他为错误
 */
int kqueue_push(kqueue_t *q, const void *item);

/**
 * @brief 出队一个元素
 * @param q 队列句柄
 * @param item 用于接收数据的缓冲区指针
 * @return KQUEUE_OK 成功, KQUEUE_ERR_EMPTY 空, 其他为错误
 */
int kqueue_pop(kqueue_t *q, void *item);

/**
 * @brief 查看队头元素但不移除 (Peek)
 * @param q 队列句柄
 * @param item 用于接收数据的缓冲区指针
 * @return KQUEUE_OK 成功, KQUEUE_ERR_EMPTY 空
 */
int kqueue_peek(const kqueue_t *q, void *item);

/**
 * @brief 获取当前队列中的元素个数
 * @param q 队列句柄
 * @return 元素个数
 */
unsigned int kqueue_count(const kqueue_t *q);

/**
 * @brief 获取队列是否为空
 */
bool kqueue_is_empty(const kqueue_t *q);

/**
 * @brief 获取队列是否已满
 */
bool kqueue_is_full(const kqueue_t *q);

/**
 * @brief 重置队列
 */
void kqueue_reset(kqueue_t *q);

/* ==============================================
 * 辅助宏，提供类似C++模板的便捷性和类型检查提示
 * ============================================== */

/**
 * @brief 类型安全的入队宏
 * @param _q 队列句柄
 * @param _type 元素类型
 * @param _val 元素值 (非指针)
 */
#define KQUEUE_PUSH_VAL(_q, _type, _val)                                                                               \
    ({                                                                                                                 \
        _type __v = (_val);                                                                                            \
        kqueue_push(_q, &__v);                                                                                         \
    })

#endif /* KQUEUE_H */