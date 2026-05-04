

#ifndef ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_
#define ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "stddef.h"

    // 用户调用函数
    extern void __free(void *ptr);
    // 内存释放(外部调用)
    extern void *__malloc(unsigned int size);
    // 内存分配(外部调用)
    extern void *__realloc(void *ptr, unsigned int size);
    // 重新分配内存(外部调用)

    void __memset(void *s, unsigned char c, unsigned int count);

    void __memcpy(void *des, const void *src, unsigned int n);

    int __memcmp(const void *s1, const void *s2, int n);

    int __strcmp(const char *s1, const char *s2);

    int __strlen(const char *s);

    char *__strcpy(char *dst, const char *src);

    int __strncmp(const char *s1, const char *s2, unsigned int maxlen);

    int __strnlen(const char *s, unsigned int maxlen);

    char *__strncpy(char *dst, const char *src, unsigned int maxlen);

    void *__memmove(void *dst, const void *src, const size_t n);

    /*---- C ----*/

#ifdef __cplusplus
}

/*---- Cpp ----*/

/*---- Cpp ----*/

#endif

#endif // ASTRA_CORE_SRC_HAL_HAL_DREAMCORE_MEMORY_POOL_H_
