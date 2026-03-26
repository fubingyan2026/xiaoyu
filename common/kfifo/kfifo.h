/* kfifo.h */
#ifndef KFIFO_H
#define KFIFO_H

#include "memory_pool/memory_pool.h"
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    KFIFO_ERROR = -1,
    KFIFO_EMPTY = 0,
    KFIFO_HALFFULL = 1,
    KFIFO_FULL = 2
} kfifo_state;

typedef struct
{
    unsigned char *buffer;
    unsigned int size;
    unsigned int in;
    unsigned int out;
    uint32_t *lock;
} kfifo_t;

/* 函数声明 */
kfifo_t *kfifo_init(unsigned char *buffer, unsigned int size, int gfp_mask, uint32_t *lock);
kfifo_t *kfifo_alloc(unsigned int size, uint32_t *lock);
unsigned int kfifo_put(kfifo_t *fifo, const unsigned char *buffer, unsigned int len);
unsigned int kfifo_get(kfifo_t *fifo, unsigned char *buffer, unsigned int len);
unsigned int kfifo_len(kfifo_t *fifo);
void kfifo_reset(kfifo_t *fifo);
unsigned int kfifo_clear_len(kfifo_t *fifo, unsigned int len);
void kfifo_free(kfifo_t *fifo);

void kfifo_move_in(kfifo_t *fifo, unsigned int len);

kfifo_state kfifo_status(kfifo_t *fifo);

uint32_t kfifo_skip(kfifo_t *fifo, uint32_t len);
uint32_t kfifo_peek(const kfifo_t *fifo, uint8_t *buffer, uint32_t len, uint32_t offset);

#endif /* KFIFO_H */