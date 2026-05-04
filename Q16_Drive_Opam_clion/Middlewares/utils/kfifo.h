/**
 * @brief: Kernel fifo header (Platform Independent & Flat Structure)
 * @FilePath: kfifo.h
 * @author: fubingyan qq:3245784484
 * @Date: 2025-08-17 23:58:01
 * @LastEditTime: 2025-09-28 21:15:39
 * @version: V1.0.0
 * @note: Linux内核风格的环形缓冲区实现，支持平台移植
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#ifndef KFIFO_H
#define KFIFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================== */
/*                         【平台移植接口层 Porting Layer】 */
/* 说明：如果要移植到其他平台（如 ESP32, Linux,
 * 纯裸机），只需修改本区域的宏定义即可   */
/* ========================================================================== */

/* 1. 引入你的平台底层依赖头文件 */
#include "main.h" /* 例如: STM32 HAL库头文件，提供 __DMB 和 __get_PRIMASK */
#include "memory_pool.h" /* 内存池头文件 */

/* 2. 内存分配与释放接口 */
#ifndef KFIFO_MALLOC
#define KFIFO_MALLOC(size) __malloc(size)
#endif

#ifndef KFIFO_FREE
#define KFIFO_FREE(ptr) __free(ptr)
#endif

/* 3. 数据内存屏障接口 (解决带 Cache / 多核 / DMA 的一致性问题) */
#ifndef KFIFO_MEM_BARRIER
/* STM32/ARM 架构默认使用 __DMB()。如果是普通无流水线单片机，可留空。 */
#define KFIFO_MEM_BARRIER()
#endif

/* 4. 临界区保护接口 (解决多线程/嵌套中断并发安全) */
typedef uint32_t kfifo_lock_state_t; /* 保存锁状态的变量类型 */

#ifndef KFIFO_LOCK
/* STM32 裸机安全嵌套中断锁。如果用 FreeRTOS，可换成 taskENTER_CRITICAL_FROM_ISR
 */
#define KFIFO_LOCK(flags)          \
    do {                           \
        (flags) = __get_PRIMASK(); \
        __disable_irq();           \
    } while (0)
#endif

#ifndef KFIFO_UNLOCK
#define KFIFO_UNLOCK(flags)   \
    do {                      \
        __set_PRIMASK(flags); \
    } while (0)
#endif

/* ========================================================================== */

/**
 * @brief 高级运行状态枚举 (细化水位，方便硬件流控与报警)
 */
typedef enum {
    /* --- 错误状态区 (<0) --- */
    KFIFO_ERR_NULL = -1, /* 传入了空指针 */
    KFIFO_ERR_UNINIT = -2, /* 缓冲区未初始化 */

    /* --- 正常运行状态区 (>=0) --- */
    KFIFO_EMPTY = 0, /* 队列全空：无数据可读 */
    KFIFO_ALMOST_EMPTY = 1, /* 将空水位：数据量不足 1/4 */
    KFIFO_PARTIAL = 2, /* 常规状态：正常部分填充 */
    KFIFO_HALFFULL = 3, /* 半满状态：数据量在 37.5% ~ 62.5% */
    KFIFO_ALMOST_FULL = 4, /* 将满水位：数据量超过 3/4（需警惕溢出） */
    KFIFO_FULL = 5 /* 队列全满：无法再写入 */
} kfifo_state;

/**
 * @brief kfifo 结构体
 */
typedef struct {
    unsigned char* buffer; /* 缓冲区指针 */
    unsigned int size; /* 缓冲区大小（必须为2的幂） */
    unsigned int in; /* 写入位置（生产者指针） */
    unsigned int out; /* 读取位置（消费者指针） */
    uint32_t* lock; /* 锁指针，用于线程安全 */
} kfifo_t;

/**
 * @brief kfifo 详细信息监控结构体
 */
typedef struct {
    unsigned int total_size; /* 总容量 */
    unsigned int used_len; /* 已用大小 */
    unsigned int free_len; /* 剩余大小 */
    uint8_t usage_percent; /* 使用率百分比 (0~100) */
    kfifo_state state; /* 当前枚举状态 */
} kfifo_info_t;

/* --- 函数声明 --- */

/**
 * @brief 初始化kfifo结构体（静态分配内存）
 * @param fifo kfifo结构体指针
 * @param buffer 缓冲区指针
 * @param size 缓冲区大小，必须为2的幂
 * @param lock 锁指针，用于线程安全
 */
void kfifo_init(kfifo_t* fifo, unsigned char* buffer, unsigned int size,
    uint32_t* lock);

/**
 * @brief 动态分配并初始化kfifo
 * @param size 缓冲区大小，会自动调整为2的幂
 * @param lock 锁指针，用于线程安全
 * @return 成功返回kfifo指针，失败返回NULL
 */
kfifo_t* kfifo_alloc(unsigned int size, uint32_t* lock);

/**
 * @brief 释放kfifo及其缓冲区
 * @param fifo kfifo指针
 */
void kfifo_free(kfifo_t* fifo);

/**
 * @brief 向kfifo放入数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际放入的数据长度
 */
unsigned int kfifo_put(kfifo_t* fifo, const unsigned char* buffer,
    unsigned int len);

/**
 * @brief 从kfifo取出数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际取出的数据长度
 */
unsigned int kfifo_get(kfifo_t* fifo, unsigned char* buffer, unsigned int len);

/**
 * @brief 查看环形缓冲区中的数据而不移除
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 期望查看的数据长度
 * @param offset 查看偏移量（从当前读取位置开始）
 * @return 实际查看的数据长度
 */
uint32_t kfifo_peek(kfifo_t* fifo, uint8_t* buffer, uint32_t len,
    uint32_t offset);

/**
 * @brief 跳过指定长度的数据而不读取
 * @param fifo kfifo指针
 * @param len 要跳过的数据长度
 * @return 实际跳过的数据长度
 */
uint32_t kfifo_skip(kfifo_t* fifo, uint32_t len);

/**
 * @brief 获取kfifo中数据长度
 * @param fifo kfifo指针
 * @return 数据长度
 */
unsigned int kfifo_len(kfifo_t* fifo);

/**
 * @brief 获取kfifo中可用空间
 * @param fifo kfifo指针
 * @return 可写入的数据长度
 */
unsigned int kfifo_avail(kfifo_t* fifo);

/**
 * @brief 重置kfifo
 * @param fifo kfifo指针
 */
void kfifo_reset(kfifo_t* fifo);

/**
 * @brief 清除环形缓冲区中的指定长度的数据
 * @param fifo kfifo指针
 * @param len 要清除的数据长度
 * @return 实际清除的数据长度
 */
unsigned int kfifo_clear_len(kfifo_t* fifo, unsigned int len);

/**
 * @brief 移动kfifo in的位置（用于DMA同步场景）
 * @param fifo kfifo指针
 * @param len 移动长度
 * @note 外部（如DMA）已将数据写入缓冲区，调用此函数通知FIFO有新数据
 */
void kfifo_skip_in(kfifo_t* fifo, const unsigned int len);

/**
 * @brief 移动kfifo in的位置（用于DMA同步场景）
 * @param fifo kfifo指针
 * @param len 移动长度
 * @note 外部（如DMA）已将数据写入缓冲区，调用此函数通知FIFO有新数据
 */
void kfifo_move_in(kfifo_t* fifo, unsigned int dma_hw_index);

/**
 * @brief 获取kfifo状态
 * @param fifo kfifo指针
 * @return kfifo状态枚举值
 */
kfifo_state kfifo_status(kfifo_t* fifo);

/**
 * @brief 一次性获取队列全状态视图
 * @param fifo kfifo指针
 * @param info 信息结构体指针
 */
void kfifo_get_info(kfifo_t* fifo, kfifo_info_t* info);

/**
 * @brief 获取kfifo的总大小
 * @param fifo kfifo指针
 * @return 总大小
 */
unsigned int kfifo_size(kfifo_t* fifo);

/**
 * @brief 检查kfifo是否为空
 * @param fifo kfifo指针
 * @return 1为空，0为非空
 */
int kfifo_is_empty(kfifo_t* fifo);

/**
 * @brief 检查kfifo是否为满
 * @param fifo kfifo指针
 * @return 1为满，0为不满
 */
int kfifo_is_full(kfifo_t* fifo);

/* --- 宏定义 --- */

/**
 * @brief 静态初始化kfifo
 */
#define KFIFO_INIT(name, buffer, size) { (buffer), (size), 0, 0, NULL }

/**
 * @brief 检查kfifo是否已初始化
 */
#define kfifo_initialized(fifo) ((fifo)->buffer != NULL)

#ifdef __cplusplus
}
#endif

#endif /* KFIFO_H */
