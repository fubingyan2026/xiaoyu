/**
 * @brief: copyright liunx: Kernel fifo (Platform Independent & Flat Structure)
 * @FilePath: kfifo.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-08-17 23:58:01
 * @LastEditTime: 2025-09-28 21:15:39
 * @version: V1.0.0
 *           V1.0.1 2025-08-17 23:58:01
 * 更新了头和尾的限幅值，防止内存随即初始化带来的的计算耗时 V1.0.2 2025-09-05
 * 19:38:06 修改了Create_FIFO_Buffer函数，传入外部申请的内存
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "kfifo.h"

/**
 * @brief 判断一个数是否为2的幂次方
 * @param n 要判断的数
 * @return 1表示是2的幂，0表示不是
 * @note 用于检查缓冲区大小是否符合环形队列要求
 */
static inline int is_power_of_2(unsigned int n)
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

/**
 * @brief 将一个数向上取整到最近的2的幂次方
 * @param n 输入数
 * @return 最近的2的幂次方数
 * @note 用于自动调整缓冲区大小以优化性能
 */
static inline unsigned int roundup_pow_of_two(unsigned int n)
{
    if (n == 0)
        return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

/**
 * @brief 取两个数中的最小值
 * @param a 第一个数
 * @param b 第二个数
 * @return 较小的数
 */
static inline unsigned int min(unsigned int a, unsigned int b)
{
    return a < b ? a : b;
}

/**
 * @brief 初始化kfifo结构体（静态分配内存）
 * @param fifo kfifo结构体指针
 * @param buffer 缓冲区指针
 * @param size 缓冲区大小，必须为2的幂
 * @param lock 锁指针，用于线程安全
 * @note 如果参数无效，会将fifo置为空状态
 */
void kfifo_init(kfifo_t* fifo, unsigned char* buffer, unsigned int size, uint32_t* lock)
{
    if (fifo == NULL || buffer == NULL)
        return;

    if (!is_power_of_2(size)) {
        size = roundup_pow_of_two(size);
    }

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;
    fifo->lock = lock;
}

/**
 * @brief 动态分配并初始化kfifo
 * @param size 缓冲区大小，会自动调整为2的幂
 * @param lock 锁指针，用于线程安全
 * @return 成功返回kfifo指针，失败返回NULL
 * @note 自动分配缓冲区和结构体内存
 */
kfifo_t* kfifo_alloc(unsigned int size, uint32_t* lock)
{
    if (!is_power_of_2(size)) {
        if (size > 0x80000000)
            return NULL;
        size = roundup_pow_of_two(size);
    }

    /* 使用平台隔离层的宏 */
    unsigned char* buffer = (unsigned char*)KFIFO_MALLOC(size);
    if (!buffer)
        return NULL;

    kfifo_t* fifo = (kfifo_t*)KFIFO_MALLOC(sizeof(kfifo_t));
    if (!fifo) {
        KFIFO_FREE(buffer);
        return NULL;
    }

    kfifo_init(fifo, buffer, size, lock);
    return fifo;
}

/**
 * @brief 释放kfifo及其缓冲区
 * @param fifo kfifo指针
 * @note 只适用于动态分配的kfifo，静态分配的需要手动管理
 */
void kfifo_free(kfifo_t* fifo)
{
    if (fifo) {
        KFIFO_FREE(fifo->buffer);
        KFIFO_FREE(fifo);
    }
}

/**
 * @brief 向kfifo放入数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际放入的数据长度
 * @note 线程安全，支持环形缓冲区回绕
 */
unsigned int kfifo_put(kfifo_t* fifo, const unsigned char* buffer, unsigned int len)
{
    if (fifo == NULL || buffer == NULL || len == 0)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    len = min(len, fifo->size - (fifo->in - fifo->out));
    if (len > 0) {
        const unsigned int l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
        memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);

        /* 避免无效拷贝 */
        if (len > l) {
            memcpy(fifo->buffer, buffer + l, len - l);
        }

        /* 平台隔离层：数据落盘屏障，保证 DMA/多核 看到正确的数据 */
        KFIFO_MEM_BARRIER();
        fifo->in += len;
    }

    KFIFO_UNLOCK(flags);
    return len;
}

/**
 * @brief 从kfifo取出数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际取出的数据长度
 * @note 线程安全，支持环形缓冲区回绕
 */
unsigned int kfifo_get(kfifo_t* fifo, unsigned char* buffer, unsigned int len)
{
    if (fifo == NULL || buffer == NULL || len == 0)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    len = min(len, fifo->in - fifo->out);
    if (len > 0) {
        const unsigned int l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
        memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);

        /* 避免无效拷贝 */
        if (len > l) {
            memcpy(buffer + l, fifo->buffer, len - l);
        }

        /* 平台隔离层：确保读取完毕后再更新 out 指针，防止后面的写操作覆盖数据 */
        KFIFO_MEM_BARRIER();

        /* 保持原汁原味的自然溢出机制，只推进 out，绝对不再重置 out = in */
        fifo->out += len;
    }

    KFIFO_UNLOCK(flags);
    return len;
}

/**
 * @brief 跳过指定长度的数据而不读取
 * @param fifo kfifo指针
 * @param len 要跳过的数据长度
 * @return 实际跳过的数据长度
 * @note 用于丢弃无效数据，常用于协议解析
 */
uint32_t kfifo_skip(kfifo_t* fifo, uint32_t len)
{
    if (fifo == NULL || len == 0)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    len = min(len, fifo->in - fifo->out);
    if (len > 0) {
        /* 仅仅推进 out 指针跳过数据，无任何指针重置逻辑 */
        fifo->out += len;
    }

    KFIFO_UNLOCK(flags);
    return len;
}

/**
 * @brief 查看环形缓冲区中的数据而不移除
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 期望查看的数据长度
 * @param offset 查看偏移量（从当前读取位置开始）
 * @return 实际查看的数据长度
 * @note 用于预览数据而不改变读取位置
 */
uint32_t kfifo_peek(kfifo_t* fifo, uint8_t* buffer, uint32_t len, uint32_t offset)
{
    if (fifo == NULL || buffer == NULL || len == 0)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags); /* 必须加锁防止偏移计算时指针被中断改写 */

    const uint32_t available = fifo->in - fifo->out;
    if (offset >= available) {
        KFIFO_UNLOCK(flags);
        return 0;
    }

    len = min(len, available - offset);
    const uint32_t out = fifo->out + offset;

    const uint32_t l = min(len, fifo->size - (out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (out & (fifo->size - 1)), l);

    if (len > l) {
        memcpy(buffer + l, fifo->buffer, len - l);
    }

    KFIFO_UNLOCK(flags);
    return len;
}

/**
 * @brief 获取kfifo中数据长度
 * @param fifo kfifo指针
 * @return 数据长度
 */
unsigned int kfifo_len(kfifo_t* fifo)
{
    if (fifo == NULL)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    const unsigned int len = fifo->in - fifo->out;
    KFIFO_UNLOCK(flags);

    return len;
}

/**
 * @brief 获取kfifo中可用空间
 * @param fifo kfifo指针
 * @return 可写入的数据长度
 */
unsigned int kfifo_avail(kfifo_t* fifo)
{
    if (fifo == NULL)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    const unsigned int avail = fifo->size - (fifo->in - fifo->out);
    KFIFO_UNLOCK(flags);

    return avail;
}

/**
 * @brief 重置kfifo
 * @param fifo kfifo指针
 * @note 清空所有数据，重置读写指针
 */
void kfifo_reset(kfifo_t* fifo)
{
    if (fifo == NULL)
        return;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    fifo->in = fifo->out = 0;
    KFIFO_UNLOCK(flags);
}

/**
 * @brief 清除环形缓冲区中的指定长度的数据
 * @param fifo kfifo指针
 * @param len 要清除的数据长度
 * @return 实际清除的数据长度
 * @note 相当于跳过数据
 */
unsigned int kfifo_clear_len(kfifo_t* fifo, unsigned int len)
{
    if (fifo == NULL || len == 0)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    len = min(len, fifo->in - fifo->out);
    fifo->out += len;

    KFIFO_UNLOCK(flags);
    return len;
}

/**
 * @brief  同步 DMA 硬件的写入位置到 kfifo（专用于 DMA NORMAL模式）
 * @param  fifo         kfifo 指针
 * @param  dma_hw_index 当前 DMA 硬件正在指向的物理数组下标 (0 到 size-1)
 * @note   在 STM32 中，通常通过 (fifo->size - __HAL_DMA_GET_COUNTER(hdma)) 计算得出
 */
void kfifo_skip_in(kfifo_t* fifo, const unsigned int len)
{
    if (fifo == NULL || len == 0)
        return;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    unsigned int available = fifo->size - (fifo->in - fifo->out);
    unsigned int actual_len = min(len, available);

    /* 平台隔离层：确保 DMA 数据真正到达内存，再更新软件 in 指针 */
    KFIFO_MEM_BARRIER();
    fifo->in += actual_len;

    KFIFO_UNLOCK(flags);
}

/**
 * @brief  同步 DMA 硬件的写入位置到 kfifo（专用于 DMA CIRCLE模式）
 * @param  fifo         kfifo 指针
 * @param  dma_hw_index 当前 DMA 硬件正在指向的物理数组下标 (0 到 size-1)
 * @note   在 STM32 中，通常通过 (fifo->size - __HAL_DMA_GET_COUNTER(hdma)) 计算得出
 */
void kfifo_move_in(kfifo_t* fifo, unsigned int dma_hw_index)
{
    if (fifo == NULL)
        return;

    dma_hw_index = dma_hw_index & (fifo->size - 1);

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    const unsigned int current_in = fifo->in & (fifo->size - 1);
    const unsigned int move_in_len = (dma_hw_index >= current_in) ? (dma_hw_index - current_in) : (dma_hw_index + fifo->size - current_in);

    if (move_in_len > 0) {
        /* 内存隔离层：确保 DMA 数据真正到达 RAM，再更新软件 in 指针 */
        KFIFO_MEM_BARRIER();
        fifo->in += move_in_len;

        /*
         * 致命错误保护：DMA 数据覆盖（Overrun）保护
         * 如果 DMA 跑得太快，追上了甚至超越了 out 指针，物理内存里的老数据已经被覆盖了。
         * 此时为了防止 kfifo_len 超过 size 导致 memcpy 越界，我们必须强制推进 out 指针，
         * 也就是忍痛丢弃被破坏的老数据，保留最新的数据。
         */
        if ((fifo->in - fifo->out) > fifo->size) {
            fifo->out = fifo->in - fifo->size;
            /* 可选：如果你有全局状态记录，这里可以增加一个 Overrun 溢出错误计数 */
        }
    }

    KFIFO_UNLOCK(flags);
}

/**
 * @brief 获取kfifo状态（带有精确高低水位的高级状态）
 * @param fifo kfifo指针
 * @return kfifo状态枚举值
 * @note 提供详细的状态信息，包括水位预警
 */
kfifo_state kfifo_status(kfifo_t* fifo)
{
    if (fifo == NULL)
        return KFIFO_ERR_NULL;
    if (fifo->buffer == NULL || fifo->size == 0)
        return KFIFO_ERR_UNINIT;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    const unsigned int len = fifo->in - fifo->out;
    KFIFO_UNLOCK(flags);

    if (len == 0) {
        return KFIFO_EMPTY;
    } else if (len == fifo->size) {
        return KFIFO_FULL;
    }
    /* 队列数据量 >= 75% (即将溢出警戒) */
    else if (len >= (fifo->size - (fifo->size >> 2))) {
        return KFIFO_ALMOST_FULL;
    }
    /* 队列数据量在 37.5% ~ 62.5% 之间 (半满状态) */
    else if (len >= ((fifo->size >> 1) - (fifo->size >> 3)) && len <= ((fifo->size >> 1) + (fifo->size >> 3))) {
        return KFIFO_HALFFULL;
    }
    /* 队列数据量 <= 25% (即将枯竭警戒) */
    else if (len <= (fifo->size >> 2)) {
        return KFIFO_ALMOST_EMPTY;
    }

    return KFIFO_PARTIAL;
}

/**
 * @brief 一次性获取队列全状态视图（常用于UI面板/Log诊断）
 * @param fifo kfifo指针
 * @param info 信息结构体指针
 * @note 提供完整的队列状态信息
 */
void kfifo_get_info(kfifo_t* fifo, kfifo_info_t* info)
{
    if (fifo == NULL || info == NULL)
        return;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);

    unsigned int len = fifo->in - fifo->out;
    info->total_size = fifo->size;
    info->used_len = len;
    info->free_len = fifo->size - len;

    if (fifo->size > 0) {
        info->usage_percent = (uint8_t)((len * 100) / fifo->size);
    } else {
        info->usage_percent = 0;
    }

    if (len == 0) {
        info->state = KFIFO_EMPTY;
    } else if (len == fifo->size) {
        info->state = KFIFO_FULL;
    } else if (len >= (fifo->size - (fifo->size >> 2))) {
        info->state = KFIFO_ALMOST_FULL;
    } else if (len >= ((fifo->size >> 1) - (fifo->size >> 3)) && len <= ((fifo->size >> 1) + (fifo->size >> 3))) {
        info->state = KFIFO_HALFFULL;
    } else if (len <= (fifo->size >> 2)) {
        info->state = KFIFO_ALMOST_EMPTY;
    } else {
        info->state = KFIFO_PARTIAL;
    }

    KFIFO_UNLOCK(flags);
}

/**
 * @brief 获取kfifo的总大小
 * @param fifo kfifo指针
 * @return 总大小
 * @note 参考Linux内核kfifo_size
 */
unsigned int kfifo_size(kfifo_t* fifo)
{
    return fifo ? fifo->size : 0;
}

/**
 * @brief 检查kfifo是否为空
 * @param fifo kfifo指针
 * @return 1为空，0为非空
 * @note 参考Linux内核kfifo_is_empty
 */
int kfifo_is_empty(kfifo_t* fifo)
{
    if (fifo == NULL)
        return 1;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    int empty = (fifo->in == fifo->out);
    KFIFO_UNLOCK(flags);

    return empty;
}

/**
 * @brief 检查kfifo是否为满
 * @param fifo kfifo指针
 * @return 1为满，0为不满
 * @note 参考Linux内核kfifo_is_full
 */
int kfifo_is_full(kfifo_t* fifo)
{
    if (fifo == NULL)
        return 0;

    kfifo_lock_state_t flags;
    KFIFO_LOCK(flags);
    int full = (fifo->size - (fifo->in - fifo->out)) == 0;
    KFIFO_UNLOCK(flags);

    return full;
}
