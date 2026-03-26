#ifndef __B_PROTOCOL_CORE_H__
#define __B_PROTOCOL_CORE_H__

#include "debug/debug.h"
#include "kfifo/kfifo.h"
#include "stdint.h"

// 错误码定义
typedef enum __attribute__((packed))
{
    B_SUCCESS = 0,                /* 操作成功 */
    B_ERROR_NULL_PTR = 1,         /* 空指针错误 */
    B_ERROR_BUFFER_OVERFLOW = 2,  /* 缓冲区溢出 */
    B_ERROR_TIMEOUT = 3,          /* 超时错误 */
    B_ERROR_CHECKSUM = 4,         /* 校验错误 */
    B_ERROR_INIT_FAILED = 5,      /* 初始化失败 */
    B_ERROR_PARAM_INVALID = 6,    /* 参数无效 */
    B_ERROR_BUFFER_TOO_SMALL = 7, /* 缓冲区太小 */
    B_ERROR_FRAME_INVALID = 8,    /* 帧格式无效 */
    B_ERROR_MEMORY_ALLOC = 9,     /* 内存分配失败 */
    B_ERROR_UNINITIALIZED = 10,   /* 未初始化 */
    B_ERROR_GENERIC = 255         /* 通用错误 */
} b_error_t;

// 使能调试信息输出
// 0,不打印调试信息
// 1,仅打印原始接收数据
// 2,打印调试数据
#define EN_FRAME_DEBUG_LV 0

// 空闲钩子函数的计数周期
#define IDIE_TIMER_US (1000 * 5) // 5ms

// 波特率
#define FRAME_BAUD_RATE 115200

// 空闲因子，用于消除帧头/数据长度解算错误
#define IDIE_FACTOR 4

#if (EN_FRAME_DEBUG_LV != 0)
#include "stdio.h"
#define FRAME_LOG_PRINTF(...) BSP_Printf(__VA_ARGS__)

#if (EN_FRAME_DEBUG_LV == 1)
#define FRAME_RAW_INFO_PRINTF(...) BSP_Printf(__VA_ARGS__)
#define FRAME_LOG_INFO_PRINTF(...)
#elif (EN_FRAME_DEBUG_LV == 2)
#define FRAME_RAW_INFO_PRINTF(...) BSP_Printf(__VA_ARGS__)
#define FRAME_LOG_INFO_PRINTF(...) FRAME_LOG_PRINTF(__VA_ARGS__)
#endif

#else
#define FRAME_LOG_PRINTF(...)
#define FRAME_RAW_INFO_PRINTF(...)
#define FRAME_LOG_INFO_PRINTF(...)
#endif

/**
 * @brief 校验一帧数据的回调函数
 * @param buffer 指向一帧数据的指针
 * @param len 该帧数据的总长度
 * @return uint8_t 校验结果，0表示校验成功，非0表示校验失败
 */
typedef uint8_t b_check_cb_t(uint8_t *buffer, uint16_t len);

/**
 * @brief 校验一帧数据
 *
 * 该函数根据输入的一帧数据和其长度，进行校验并返回校验结果。
 * 具体实现根据协议定义，可能会根据校验和、校验字段等进行校验。
 *
 * @param buffer 指向一帧数据的指针
 * @param len 该帧数据的总长度
 * @return uint16_t 该帧数据的实际长度，如果无法解析则返回0
 */
typedef uint16_t b_get_frame_len_cb_t(uint8_t *buffer, uint16_t len);

/**
 * @brief 协议描述结构体
 *
 * 该结构体用于描述一个协议的相关信息，包括协议名称、帧头、帧尾、校验函数等。
 * 协议解析器根据该结构体中的信息，对输入的一帧数据进行解析和校验。
 */
typedef struct
{
    const char *pname;
    const uint8_t *head;                    /* 统一使用uint8_t类型，避免符号扩展问题 */
    const uint8_t *end;                     /* 统一使用uint8_t类型，避免符号扩展问题 */
    uint8_t *out_frame_buffer;              /* 解析数据缓冲区 */
    b_get_frame_len_cb_t *get_frame_len_cb; /* 获取帧长函数指针 */
    b_check_cb_t *check_cb;                 /* 帧校验函数指针 */
    uint16_t head_len;                      /* 帧头长度 */
    uint16_t end_len;                       /* 帧尾长度 */
    uint16_t in_buffer_len;                 /* 输入原始数据缓冲区大小，2的N次幂有效 */
    uint16_t out_buffer_len;                /* 输出数据缓冲区大小 */
} b_frame_init_type;

/**
 * @brief 协议结构体
 *
 * 该结构体用于描述一个协议的相关信息，包括协议名称、帧头、帧尾、校验函数等。
 * 协议解析器根据该结构体中的信息，对输入的一帧数据进行解析和校验。
 */
typedef struct
{
    b_frame_init_type frame_init;
    // 协议处理需要用到的中间变量，用户不可操作
    kfifo_t *_frame_ring;
    uint32_t _idie_timer;
    uint32_t _systick;
    uint8_t head_match_flg;
    uint8_t _is_initialized; /* 初始化状态标志 */
} b_frame_type;

void printHexAscii(const unsigned char *data, size_t length);

b_error_t b_frame_init(b_frame_type *pframe, b_frame_init_type *pframeinit);

void b_frame_deinit(b_frame_type *pframe);

void b_frame_idie_timer(b_frame_type *pframe);

b_error_t b_frame_put(b_frame_type *pframe, uint8_t *dat, uint32_t len);

void b_frame_fifo_clear(b_frame_type *pframe);

uint32_t b_frame_fifo_get(b_frame_type *pframe, uint8_t *dat, uint32_t len, uint32_t timeout);

const uint8_t *b_frame_check_get(b_frame_type *pframe, uint16_t *len);

uint8_t b_frame_is_initialized(const b_frame_type *pframe);

#endif