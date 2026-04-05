#ifndef __B_PROTOCOL_CORE_H__
#define __B_PROTOCOL_CORE_H__

#include "debug/debug.h"
#include "kfifo/kfifo.h"
#include "stdint.h"
#include "string.h"

typedef enum __attribute__((packed)) {
  B_SUCCESS = 0,
  B_ERROR_NULL_PTR = 1,
  B_ERROR_BUFFER_OVERFLOW = 2,
  B_ERROR_TIMEOUT = 3,
  B_ERROR_CHECKSUM = 4,
  B_ERROR_INIT_FAILED = 5,
  B_ERROR_PARAM_INVALID = 6,
  B_ERROR_BUFFER_TOO_SMALL = 7,
  B_ERROR_FRAME_INVALID = 8,
  B_ERROR_MEMORY_ALLOC = 9,
  B_ERROR_UNINITIALIZED = 10,
  B_ERROR_GENERIC = 255
} b_error_t;

#define EN_FRAME_DEBUG_LV 0

#define IDLE_TIMER_US (1000 * 5)

#define FRAME_BAUD_RATE 115200

#define IDLE_FACTOR 4

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

typedef uint8_t b_check_cb_t(uint8_t* buffer, uint16_t len);

typedef uint16_t b_get_frame_len_cb_t(uint8_t* buffer, uint16_t len);

typedef struct {
  const char* pname;
  const uint8_t* head;
  const uint8_t* end;
  uint8_t* out_frame_buffer;
  b_get_frame_len_cb_t* get_frame_len_cb;
  b_check_cb_t* check_cb;
  uint16_t head_len;
  uint16_t end_len;
  uint16_t in_buffer_len;
  uint16_t out_buffer_len;
} b_frame_init_type;

typedef struct {
  b_frame_init_type frame_init;
  kfifo_t* _frame_ring;
  uint32_t _idle_timer;
  uint32_t _systick;
  uint32_t _last_log_time;
  uint8_t head_match_flg;
  uint8_t _is_initialized;
} b_frame_type;

void printHexAscii(const unsigned char* data, size_t length);

b_error_t b_frame_init(b_frame_type* pframe,
                       const b_frame_init_type* pframeinit);

void b_frame_deinit(b_frame_type* pframe);

void b_frame_idle_timer(b_frame_type* pframe);

b_error_t b_frame_put(b_frame_type* pframe, const uint8_t* dat, uint32_t len);

void b_frame_fifo_clear(b_frame_type* pframe);

uint32_t b_frame_fifo_get(b_frame_type* pframe, uint8_t* dat, uint32_t len,
                          uint32_t timeout);

const uint8_t* b_frame_check_get(b_frame_type* pframe, uint16_t* len);

uint8_t b_frame_is_initialized(const b_frame_type* pframe);

#endif
