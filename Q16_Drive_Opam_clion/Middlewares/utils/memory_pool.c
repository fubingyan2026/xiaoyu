//
// Created by fubingyan on 2024/5/4 004.
//

#include "memory_pool.h"

#include "lwmem/lwmem.h"
#include "perf_counter.h"

#if defined(__IS_COMPILER_ARM_COMPILER_5__)
__attribute__((
    aligned(LWMEM_CFG_ALIGN_NUM))) static uint8_t region1_data[1024 * 6];
#elif defined(__IS_COMPILER_ARM_COMPILER_6__)
#define __INT_TO_STR(x) #x
#define INT_TO_STR(x) __INT_TO_STR(x)
__attribute__((
    aligned(LWMEM_CFG_ALIGN_NUM))) static uint8_t region1_data[1024 * 6];
#else
__attribute__((
    section(".ram"),
    aligned(LWMEM_CFG_ALIGN_NUM))) static uint8_t region1_data[1024 * 4];
#endif

int memcmp_fast(const void* s1, const void* s2, size_t n);
int strcmp_fast(const char* s1, const char* s2);
void* memset_fast(void* s, int c, size_t n);
void* memcpy_fast(void* dst, const void* src, size_t n);
size_t strnlen_fast(const char* s, size_t maxlen);
char* strcpy_fast(char* dst, const char* src);
size_t fast_strlen4(const char* restrict s);
int fast_strncmp4(const char* restrict s1, const char* restrict s2, size_t n);
char* fast_strncpy4(char* restrict dst, const char* restrict src, size_t n);
void* memmove_optimized(void* dst, const void* src, size_t count);

static lwmem_region_t regions[] = {{region1_data, sizeof(region1_data)},
                                   /* Add more regions if needed */
                                   {NULL, 0}};

void memInit(void) { lwmem_assignmem(regions); }

void* __malloc(const unsigned int size) {
  static uint8_t mem_init_flag = 0;
  if (mem_init_flag == 0) {
    mem_init_flag = 1;
    memInit();
  }
  return lwmem_malloc(size);
}

void* __realloc(void* ptr, unsigned int size) {
  return lwmem_realloc(ptr, size);
}

void __free(void* ptr) { lwmem_free(ptr); }

void __memcpy(void* des, const void* src, unsigned int n) {
  memcpy_fast(des, src, n);
}

void __memset(void* s, const unsigned char c, unsigned int count) {
  memset_fast(s, c, count);
}

int __strnlen(const char* s, unsigned int maxlen) {
  return strnlen_fast(s, maxlen);
}

int __strlen(const char* s) { return fast_strlen4(s); }

char* __strcpy(char* dst, const char* src) { return strcpy_fast(dst, src); }

char* __strncpy(char* dst, const char* src, unsigned int maxlen) {
  return fast_strncpy4(dst, src, maxlen);
}

int __strcmp(const char* s1, const char* s2) { return strcmp_fast(s1, s2); }

int __strncmp(const char* s1, const char* s2, unsigned int maxlen) {
  return fast_strncmp4(s1, s2, maxlen);
}

int __memcmp(const void* s1, const void* s2, int n) {
  return memcmp_fast(s1, s2, n);
}

void* __memmove(void* dst, const void* src, const size_t n) {
  return memmove_optimized(dst, src, n);
}

int memcmp_fast(const void* s1, const void* s2, size_t n) {
  const unsigned char* p1 = s1;
  const unsigned char* p2 = s2;
  /* 1. 先对齐到 sizeof(uintptr_t) 边界 */
  while (n && ((uintptr_t)p1 & (sizeof(uintptr_t) - 1))) {
    if (*p1 != *p2) return *p1 - *p2;
    ++p1;
    ++p2;
    --n;
  }
  /* 2. 按字长比较 */
  const uintptr_t* w1 = (const uintptr_t*)p1;
  const uintptr_t* w2 = (const uintptr_t*)p2;
  while (n >= sizeof(uintptr_t)) {
    if (*w1 != *w2) {
      /* 发现不相等，回到字节级定位到第一个不同字节 */
      p1 = (const unsigned char*)w1;
      p2 = (const unsigned char*)w2;
      for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
      }
    }
    ++w1;
    ++w2;
    n -= sizeof(uintptr_t);
  }
  /* 3. 尾部剩余字节 */
  p1 = (const unsigned char*)w1;
  p2 = (const unsigned char*)w2;
  while (n--) {
    if (*p1 != *p2) return *p1 - *p2;
    ++p1;
    ++p2;
  }
  return 0;
}

int strcmp_fast(const char* s1, const char* s2) {
  /* 补 C99 缺失的 uintptr_t 常量宏 */
#ifndef UINTPTR_C
#define UINTPTR_C(v) (v##U) /* 32 位 MCU 用 U 后缀即可 */
#endif
  const unsigned char* p1 = (const unsigned char*)s1;
  const unsigned char* p2 = (const unsigned char*)s2;
  /* 先对齐到 sizeof(uintptr_t) 边界 */
  while (((uintptr_t)p1 & (sizeof(uintptr_t) - 1)) && *p1 && *p1 == *p2) {
    ++p1;
    ++p2;
  }
  const uintptr_t* w1 = (const uintptr_t*)p1;
  const uintptr_t* w2 = (const uintptr_t*)p2;
  /* 按字长比较，直到发现不相等或某字长里含 '\0' */
  for (;;) {
    uintptr_t v1 = *w1;
    uintptr_t v2 = *w2;
    if (v1 != v2) break; /* 肯定存在不同字节 */
    /* 检查当前字长里有没有 0 字节 */
    if (((v1 - UINTPTR_C(0x01010101)) & ~v1 & UINTPTR_C(0x01010101)))
      goto found_zero;
    ++w1;
    ++w2;
  }
  /* 回退到字节级定位第一个不同字节 */
  p1 = (const unsigned char*)w1;
  p2 = (const unsigned char*)w2;
  while (*p1 && *p1 == *p2) {
    ++p1;
    ++p2;
  }
  return *p1 - *p2;
found_zero:
  /* 回退到字节级定位第一个 '\0' */
  p1 = (const unsigned char*)w1;
  p2 = (const unsigned char*)w2;
  while (*p1 && *p1 == *p2) {
    ++p1;
    ++p2;
  }
  return *p1 - *p2;
}

static uint32_t has_zero_u32(const uint32_t v) {
  return (v - 0x01010101U) & ~v & 0x80808080U;
}

static uint32_t has_diff_u32(const uint32_t a, const uint32_t b) {
  return has_zero_u32(a ^ b); /* 差异即零 */
}

int fast_strncmp4(const char* restrict s1, const char* restrict s2, size_t n) {
  if (n == 0) return 0;
  const uint8_t* p1 = (const uint8_t*)s1;
  const uint8_t* p2 = (const uint8_t*)s2;
  /* 1. 按字节对齐到 4 字节边界 */
  while (((uintptr_t)p1 & 3) && n) {
    if (*p1 != *p2) return *p1 - *p2;
    if (*p1 == 0) return 0;
    p1++;
    p2++;
    n--;
  }
  const uint32_t* w1 = (const uint32_t*)p1;
  const uint32_t* w2 = (const uint32_t*)p2;
  /* 2. 一次比较 4 字节 */
  while (n >= 4) {
    uint32_t v1 = *w1, v2 = *w2;
    if (has_diff_u32(v1, v2)) break; /* 有差异→跳尾部 */
    if (has_zero_u32(v1)) return 0;  /* 遇到 \0 */
    w1++;
    w2++;
    n -= 4;
  }
  /* 3. 尾部 0~3 字节 */
  p1 = (const uint8_t*)w1;
  p2 = (const uint8_t*)w2;
  while (n--) {
    if (*p1 != *p2) return *p1 - *p2;
    if (*p1 == 0) return 0;
    p1++;
    p2++;
  }
  return 0;
}

void* memset_fast(void* s, int c, size_t n) {
  unsigned char* p = s;
  unsigned char v = (unsigned char)c;
  /* 1. 头部逐字节对齐到 4 字节边界 */
  while (n && ((uintptr_t)p & 3)) {
    *p++ = v;
    --n;
  }
  /* 2. 构造 32-bit 重复字：0x01010101 -> 0x01010101 */
  uint32_t word = v | (v << 8) | (v << 16) | (v << 24);
  uint32_t* w = (uint32_t*)p;
  while (n >= 4) {
    *w++ = word;
    n -= 4;
  }
  /* 3. 尾部剩余字节 */
  p = (unsigned char*)w;
  while (n--) *p++ = v;
  return s;
}

void* memcpy_fast(void* dst, const void* src, size_t n) {
  unsigned char* d = dst;
  const unsigned char* s = src;
  if (n == 0) {
    return NULL;
  }
  /* 1. 头部逐字节对齐到 4 字节边界 */
  while (n && ((uintptr_t)d & 3)) {
    *d++ = *s++;
    --n;
  }
  /* 2. 按 32-bit 字拷贝 */
  uint32_t* dw = (uint32_t*)d;
  const uint32_t* sw = (const uint32_t*)s;
  while (n >= 4) {
    *dw++ = *sw++;
    n -= 4;
  }
  /* 3. 尾部剩余字节 */
  d = (unsigned char*)dw;
  s = (const unsigned char*)sw;
  while (n--) *d++ = *s++;
  return dst;
}

static inline void _memzero4(volatile uint32_t* d, size_t wc) {
  while (wc--) *d++ = 0;
}

char* fast_strncpy4(char* restrict dst, const char* restrict src, size_t n) {
  if (n == 0) return dst;
  char* d = dst;
  const char* s = src;
  /* 1. 按字节对齐到 4 字节边界 */
  while (((uintptr_t)d & 3) && n) {
    if ((*d = *s) == 0) goto pad_zero;
    d++;
    s++;
    n--;
  }
  uint32_t* wd = (uint32_t*)d;
  const uint32_t* ws = (const uint32_t*)s;
  /* 2. 一次复制 4 字节 */
  while (n >= 4) {
    uint32_t v = *ws;
    *wd = v;
    if (has_zero_u32(v)) goto find_null;
    wd++;
    ws++;
    n -= 4;
  }
  /* 3. 尾部 0~3 字节 */
  d = (char*)wd;
  s = (const char*)ws;
  while (n) {
    if ((*d = *s) == 0) goto pad_zero;
    d++;
    s++;
    n--;
  }
  return dst;
find_null:
  /* 找到 \0 所在字节，调整指针 */
  d = (char*)wd;
  s = (const char*)ws;
  while (n && *s) {
    *d++ = *s++;
    n--;
  }
  if (n) *d++ = 0;
  n--;
pad_zero:
  /* 剩余用 \0 填充 */
  _memzero4((uint32_t*)d, n >> 2);
  d += n & ~3;
  while (n & 3) {
    *d++ = 0;
    n--;
  }
  return dst;
}

size_t fast_strlen4(const char* restrict s) {
  const uint32_t* w = (const uint32_t*)s;
  size_t n = 0;
  /* 处理头 0~3 字节，使指针 4 字节对齐 */
  while ((uintptr_t)w & 3) {
    if (*((const uint8_t*)w) == 0) return n;
    w = (const uint32_t*)((const uint8_t*)w + 1);
    n++;
  }
  /* 一次读 4 字节 */
  while (!has_zero_u32(*w)) {
    w++;
    n += 4;
  }
  /* 尾部 0~3 字节 */
  const uint8_t* c = (const uint8_t*)w;
  while (*c) {
    c++;
    n++;
  }
  return n;
}

size_t strnlen_fast(const char* s, size_t maxlen) {
  const unsigned char* p = (const unsigned char*)s;
  size_t n = 0;
  /* 1. 头部逐字节对齐到 4 字节边界 */
  while (n < maxlen && ((uintptr_t)p & 3) && *p) {
    ++p;
    ++n;
  }
  if (n == maxlen || !*p) return n;
  /* 2. 按 32-bit 字扫描 */
  const uint32_t* w = (const uint32_t*)p;
  while (n + 4 <= maxlen) {
    uint32_t v = *w++;
    /* 判断 4 字节中是否有 0 */
    if (((v - 0x01010101UL) & ~v & 0x80808080UL) != 0) {
      /* 回退到字节级定位第一个 '\0' */
      p = (const unsigned char*)w - 4;
      for (int i = 0; i < 4; ++i) {
        if (p[i] == 0) return n + i;
      }
    }
    n += 4;
  }
  /* 3. 尾部剩余字节 */
  p = (const unsigned char*)w;
  while (n < maxlen && *p) {
    ++p;
    ++n;
  }
  return n;
}

char* strcpy_fast(char* dst, const char* src) {
  char* d = dst;
  const char* s = src;
  /* 1. 头部逐字节对齐到 4 字节边界 */
  while (((uintptr_t)d & 3) && (*d = *s) != '\0') {
    ++d;
    ++s;
  }
  if (*s == 0) {
    *d = 0;
    return dst;
  }
  /* 2. 按 32-bit 字拷贝 */
  uint32_t* dw = (uint32_t*)d;
  const uint32_t* sw = (const uint32_t*)s;
  for (;;) {
    const uint32_t v = *sw++;
    *dw++ = v;
    if (((v - 0x01010101UL) & ~v & 0x80808080UL) != 0) {
      /* 回退到字节级定位 '\0' */
      d = (char*)dw - 4;
      s = (const char*)sw - 4;
      while ((*d++ = *s++) != '\0') {
      }
      return dst;
    }
  }
}

void* memmove_optimized(void* dst, const void* src, size_t count) {
  uint32_t* dst32;
  const uint32_t* src32;
  uint8_t* dst8;
  const uint8_t* src8;

  if (dst == src || count == 0) {
    return dst;
  }

  // 检查内存重叠
  if ((uintptr_t)dst > (uintptr_t)src &&
      (uintptr_t)dst < (uintptr_t)src + count) {
    // 重叠情况：从后向前复制
    dst8 = (uint8_t*)dst + count;
    src8 = (const uint8_t*)src + count;

    // 处理未对齐的尾部
    while (count && ((uintptr_t)dst8 & 3)) {
      *(--dst8) = *(--src8);
      count--;
    }

    // 32位块复制
    dst32 = (uint32_t*)dst8;
    src32 = (const uint32_t*)src8;
    while (count >= 4) {
      *(--dst32) = *(--src32);
      count -= 4;
    }

    // 处理剩余字节
    dst8 = (uint8_t*)dst32;
    src8 = (const uint8_t*)src32;
    while (count--) {
      *(--dst8) = *(--src8);
    }
  } else {
    // 无重叠情况：从前向后复制
    dst8 = (uint8_t*)dst;
    src8 = (const uint8_t*)src;

    // 处理未对齐的头部
    while (count && ((uintptr_t)dst8 & 3)) {
      *dst8++ = *src8++;
      count--;
    }

    // 32位块复制
    dst32 = (uint32_t*)dst8;
    src32 = (const uint32_t*)src8;
    while (count >= 4) {
      *dst32++ = *src32++;
      count -= 4;
    }

    // 处理剩余字节
    dst8 = (uint8_t*)dst32;
    src8 = (const uint8_t*)src32;
    while (count--) {
      *dst8++ = *src8++;
    }
  }
  return dst;
}
