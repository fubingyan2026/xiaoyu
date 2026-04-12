/**
 * @brief: copyright liunx: Kernel fifo
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

/* kfifo.c */
#include "kfifo/kfifo.h"

#include "main.h"
#include "string.h"

/* 判断是否为2的幂次方 */
static int is_power_of_2(unsigned int n) {
  return (n != 0) && ((n & (n - 1)) == 0);
}

/* 向上取整到最近的2的幂次方 */
static unsigned int roundup_pow_of_two(unsigned int n) {
  if (n == 0) return 1;
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

/* 加锁 */
static void lock(kfifo_t* fifo) { __disable_irq(); }

/* 解锁 */
static void unlock(kfifo_t* fifo) { __enable_irq(); }

/* 取最小值 */
static unsigned int min(const unsigned int a, const unsigned int b) {
  return a < b ? a : b;
}

/** 初始化kfifo（静态分配内存）
 * @param fifo kfifo指针
 * @param buffer 缓冲区指针
 * @param size 缓冲区大小
 * @param lock 自旋锁指针
 */
void kfifo_init_static(kfifo_t* fifo, unsigned char* buffer, unsigned int size,
                       uint32_t* lock) {
  fifo->buffer = buffer;
  fifo->size = size;
  fifo->in = fifo->out = 0;
  fifo->lock = lock;
}

/** 初始化kfifo
 * @param buffer 缓冲区指针
 * @param size 缓冲区大小
 * @param gfp_mask 内存分配标志（未使用）
 * @param lock 自旋锁指针
 * @return 成功返回kfifo指针，失败返回NULL
 */
kfifo_t* kfifo_init(unsigned char* buffer, const unsigned int size,
                    int gfp_mask, uint32_t* lock) {
  /* 检查size是否为2的幂 */
  if (!is_power_of_2(size)) {
    return NULL;
  }
  /* 分配kfifo结构体内存 */
  kfifo_t* fifo = __malloc(sizeof(kfifo_t));
  if (!fifo) {
    return NULL;
  }
  /* 初始化kfifo成员 */
  kfifo_init_static(fifo, buffer, size, lock);
  return fifo;
}

/** 分配并初始化kfifo
 * @param size 缓冲区大小
 * @param lock 自旋锁指针
 * @return 成功返回kfifo指针，失败返回NULL
 */
kfifo_t* kfifo_alloc(unsigned int size, uint32_t* lock) {
  /* 如果size不是2的幂，则调整为2的幂 */
  if (!is_power_of_2(size)) {
    /* 检查size是否过大 */
    if (size > 0x80000000) {
      return NULL;
    }
    /* 调整为2的幂 */
    size = roundup_pow_of_two(size);
  }

  /* 分配缓冲区内存 */
  unsigned char* buffer = __malloc(size);
  if (!buffer) {
    return NULL;
  }

  /* 初始化kfifo */
  kfifo_t* ret = kfifo_init(buffer, size, 0, lock);
  if (!ret) {
    __free(buffer);
    return NULL;
  }

  return ret;
}

/** 向kfifo放入数据（无锁版本）
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际放入的数据长度
 */
unsigned int __kfifo_put(kfifo_t* fifo, const unsigned char* buffer,
                         unsigned int len) {
  /* 计算可放入的数据长度 */
  len = min(len, fifo->size - fifo->in + fifo->out);
  /* 计算第一次拷贝的长度 */
  const unsigned int l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
  memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), (void*)buffer, l);
  /* 计算第二次拷贝的长度 */
  memcpy(fifo->buffer, (void*)(buffer + l), len - l);
  /* 更新in指针 */
  fifo->in += len;
  return len;
}

/** 向kfifo放入数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际放入的数据长度
 */
unsigned int kfifo_put(kfifo_t* fifo, const unsigned char* buffer,
                       unsigned int len) {
  lock(fifo);
  const unsigned int ret = __kfifo_put(fifo, buffer, len);
  unlock(fifo);
  return ret;
}

/** 从kfifo取出数据（无锁版本）
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际取出的数据长度
 */
unsigned int __kfifo_get(kfifo_t* fifo, unsigned char* buffer,
                         unsigned int len) {
  /* 计算可取出的数据长度 */
  len = min(len, fifo->in - fifo->out);
  /* 计算第一次拷贝的长度 */
  const unsigned int l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
  memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
  /* 计算第二次拷贝的长度 */
  memcpy(buffer + l, fifo->buffer, len - l);
  /* 更新out指针 */
  fifo->out += len;
  return len;
}

/** 从kfifo取出数据
 * @param fifo kfifo指针
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 实际取出的数据长度
 */
unsigned int kfifo_get(kfifo_t* fifo, unsigned char* buffer, unsigned int len) {
  /* 如果fifo为空，重置in和out指针 */
  lock(fifo);
  const unsigned int ret = __kfifo_get(fifo, buffer, len);
  if (fifo->in == fifo->out) {
    fifo->out = fifo->in = fifo->in & (fifo->size - 1);
  }
  unlock(fifo);
  return ret;
}

/**
 * @brief 跳过指定长度的数据而不读取
 * @param fifo 环形缓冲区指针
 * @param len 要跳过的数据长度
 * @return 实际跳过的数据长度
 * @note 这个函数用于丢弃缓冲区中的部分数据，常用于协议解析时跳过无效数据
 */
uint32_t kfifo_skip(kfifo_t* fifo, uint32_t len) {
  if (fifo == NULL || len == 0) {
    return 0;
  }

  lock(fifo);
  /* 计算可跳过的数据长度（不超过缓冲区中实际存在的数据量） */
  len = min(len, fifo->in - fifo->out);

  if (len > 0) {
    /* 直接更新读取位置，相当于跳过了这些数据 */
    fifo->out += len;

    /* 如果跳过后为空，重置指针以优化内存使用 */
    if (fifo->in == fifo->out) {
      fifo->out = fifo->in = fifo->in & (fifo->size - 1);
    }
  }
  unlock(fifo);

  return len;
}

/**
 * @brief 查看环形缓冲区中的数据而不移除
 * @param fifo 环形缓冲区指针
 * @param buffer 数据缓冲区
 * @param len 期望查看的数据长度
 * @param offset 查看偏移量（从当前读取位置开始）
 * @return 实际查看的数据长度
 * @note 这个函数用于预览数据而不改变读取位置
 */
uint32_t kfifo_peek(const kfifo_t* fifo, uint8_t* buffer, uint32_t len,
                    uint32_t offset) {
  if (fifo == NULL || buffer == NULL || len == 0) {
    return 0;
  }
  const uint32_t available = fifo->in - fifo->out;
  if (offset >= available) {
    return 0; /* 偏移量超出可用数据范围 */
  }
  uint32_t l;
  uint32_t out = fifo->out + offset;
  /* 计算实际可查看的数据长度 */
  len = min(len, available - offset);
  /* 首先查看从out+offset位置到缓冲区末尾的数据 */
  l = min(len, fifo->size - (out & (fifo->size - 1)));
  memcpy(buffer, fifo->buffer + (out & (fifo->size - 1)), l);
  /* 然后查看剩余的数据（如果有回绕） */
  memcpy(buffer + l, fifo->buffer, len - l);
  return len;
}

/** 获取kfifo中数据长度（无锁版本）
 * @param fifo kfifo指针
 * @return 数据长度
 */
static unsigned int __kfifo_len(kfifo_t* fifo) { return fifo->in - fifo->out; }

/** 获取kfifo中数据长度
 * @param fifo kfifo指针
 * @return 数据长度
 */
unsigned int kfifo_len(kfifo_t* fifo) {
  lock(fifo);
  const unsigned int ret = __kfifo_len(fifo);
  unlock(fifo);
  return ret;
}

/**重置kfifo（无锁版本） */
static void __kfifo_reset(kfifo_t* fifo) { fifo->in = fifo->out = 0; }

/* 重置kfifo */
void kfifo_reset(kfifo_t* fifo) {
  lock(fifo);
  __kfifo_reset(fifo);
  unlock(fifo);
}

/** 清除环形缓冲区中的指定长度的数据
 * @param fifo 指向kfifo结构的指针
 * @param len 要清除的数据长度
 * @return
 * 实际清除的数据长度，如果请求的长度大于当前缓冲区中的数据长度，则返回当前缓冲区中的数据长度
 */
unsigned int kfifo_clear_len(kfifo_t* fifo, unsigned int len) {
  lock(fifo);
  const unsigned int total_len = fifo->in - fifo->out;
  const unsigned int clr_len = min(len, total_len);

  // 修复：清除数据应该移动out指针，而不是in指针
  fifo->out += clr_len;

  // 如果清除后为空，重置指针以优化内存使用
  if (fifo->in == fifo->out) {
    fifo->out = fifo->in = fifo->in & (fifo->size - 1);
  }

  unlock(fifo);
  return clr_len;
}

/* 释放kfifo */
void kfifo_free(kfifo_t* fifo) {
  if (fifo) {
    __free(fifo->buffer);
    __free(fifo);
  }
}

/** 移动kfifo in 的位置（用于串口环形DMA）
 * @param fifo kfifo指针
 * @param len
 * @return None
 */
void kfifo_move_in(kfifo_t* fifo, const unsigned int len) {
  lock(fifo);
  const unsigned int current_in = fifo->in & (fifo->size - 1);
  const unsigned int move_in_len =
      len >= current_in ? len - current_in : len + fifo->size - current_in;
  fifo->in = fifo->in + move_in_len;
  unlock(fifo);
}

/** 获取kfifo状态
 * @param fifo kfifo指针
 * @return kfifo状态
 */
kfifo_state kfifo_status(kfifo_t* fifo) {
  if (fifo == NULL) {
    return KFIFO_ERROR;
  }
  const unsigned int len = __kfifo_len(fifo);
  if (len == 0) {
    return KFIFO_EMPTY;
  }
  if (len == fifo->size) {
    return KFIFO_FULL;
  }
  return KFIFO_HALFFULL;
}

// end of the file!
