/**
 * @brief: Generic Object Queue Implementation
 * @FilePath: kqueue.c
 */

#include "kqueue.h"

kqueue_t *kqueue_register(const char *name, unsigned int item_size, unsigned int max_items, unsigned int time_out)
{
    /* 先检查参数有效性 */
    if (item_size == 0 || max_items == 0 || name == NULL)
    {
        return NULL;
    }

    kqueue_t *q = (kqueue_t *)__malloc(sizeof(kqueue_t));
    if (!q)
    {
        return NULL;
    }

    q->name = name;
    q->item_size = item_size;
    q->is_external_fifo = false;
    q->daemon = NULL;  /* 初始化为 NULL，便于错误处理 */

    unsigned int total_bytes = max_items * item_size;
    q->fifo = kfifo_alloc(total_bytes, NULL);

    if (!q->fifo)
    {
        __free(q);
        return NULL;
    }

    const daemon_init_config_t daemon_config_encoder = {
        .callback = NULL,
        .owner_pointer = NULL,
        .owner_name = name,
        .reload_time_out = time_out,
        .init_wait_time = 1000,
    };

    q->daemon = DaemonRegister(&daemon_config_encoder);
    /* daemon 允许为 NULL（可选功能），不作为注册失败的判断条件 */

    return q;
}

void kqueue_destroy(kqueue_t *q)
{
    if (q)
    {
        if (!q->is_external_fifo && q->fifo)
        {
            kfifo_free(q->fifo);
        }
        __free(q);
    }
}

int kqueue_push(kqueue_t *q, const void *item)
{
    if (!q || !item)
    {
        return KQUEUE_ERR_PARAM;
    }

    /*
     * 检查剩余空间是否足够容纳一个完整的 item。
     * kfifo_len 返回的是已用字节数。
     * q->fifo->size 是总容量。
     */
    unsigned int used = kfifo_len(q->fifo);
    unsigned int capacity = q->fifo->size;

    if (capacity - used < q->item_size)
    {
        return KQUEUE_ERR_FULL;
    }

    /* 写入 kfifo */
    unsigned int ret = kfifo_put(q->fifo, (const unsigned char *)item, q->item_size);

    if (ret != q->item_size)
    {
        /* 理论上上面检查过空间，这里不应该发生，除非多线程锁失效 */
        return KQUEUE_ERR_FULL;
    }

    /* 只有 daemon 存在时才 reload */
    if (q->daemon != NULL)
    {
        DaemonReload(q->daemon);
    }

    return KQUEUE_OK;
}

int kqueue_pop(kqueue_t *q, void *item)
{
    if (!q || !item)
    {
        return KQUEUE_ERR_PARAM;
    }

    /* 先检查队列是否有数据，优先返回数据 */
    unsigned int used = kfifo_len(q->fifo);
    if (used < q->item_size)
    {
        return KQUEUE_ERR_EMPTY;
    }

    /* daemon 可选检查：如果存在且不在线才返回 timeout */
    if (q->daemon != NULL && !DaemonIsOnline(q->daemon))
    {
        return KQUEUE_ERR_TIMEOUT;
    }

    unsigned int ret = kfifo_get(q->fifo, (unsigned char *)item, q->item_size);

    if (ret != q->item_size)
    {
        return KQUEUE_ERR_EMPTY;
    }

    return KQUEUE_OK;
}

int kqueue_peek(const kqueue_t *q, void *item)
{
    if (!q || !item)
    {
        return KQUEUE_ERR_PARAM;
    }

    /* 使用 kfifo_peek 预览数据，偏移量为 0 */
    unsigned int ret = kfifo_peek(q->fifo, (unsigned char *)item, q->item_size, 0);

    if (ret != q->item_size)
    {
        return KQUEUE_ERR_EMPTY;
    }

    return KQUEUE_OK;
}

unsigned int kqueue_count(const kqueue_t *q)
{
    if (!q)
        return 0;
    /* 元素个数 = 总字节数 / 单个元素大小 */
    return kfifo_len(q->fifo) / q->item_size;
}

bool kqueue_is_empty(const kqueue_t *q)
{
    if (!q)
        return true;
    return (kfifo_len(q->fifo) < q->item_size);
}

bool kqueue_is_full(const kqueue_t *q)
{
    if (!q)
        return false;  /* 空指针视为未满（保守处理） */
    unsigned int used = kfifo_len(q->fifo);
    return (q->fifo->size - used < q->item_size);
}

void kqueue_reset(kqueue_t *q)
{
    if (q && q->fifo)
    {
        kfifo_reset(q->fifo);
    }
}