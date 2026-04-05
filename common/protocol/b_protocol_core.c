/**
 * @brief:  通用协议解析器(依赖kfifo模块) - 优化版本
 * @FilePath: b_protocol_core.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-10-13 19:25:52
 * @LastEditTime: 2026-04-05 修复所有已知问题
 * @version: V1.2.0
 * @note        Author         Email                   Notes
 * 2023-09-08   Initial version
 * 2023-09-18   修改接口设计，支持多协议，优化调试日志
 * 2023-10-23	增加接口函数b_frame_fifo_get，用于从fifo中阻塞式获取预期长度数据
 * 2024-07-04	优化帧头帧尾长度为0情况下的支持
 * 2024-07-05	优化uart_get_frame_len_cb返回值为0时的处理策略
 * 2024-07-09	帧头为0时,校验不通过应至少丢弃1个数据
 * 2024-07-09	输入/输出缓存大小分开配置，增加调试信息等级配置
 * 2024-12-20	修复对多协议的支持的相关bug
 * 2025-09-04
 * 输入输出缓冲区大小不通过宏定义配置，在初始化时进行配置，方便多协议解析器时合理利用内存
 * 2025-10-13   修复安全隐患：内存管理、缓冲区溢出、类型转换、空指针等问题
 * 2026-01-21   修复严重缓冲区溢出、kfifo_clear_len逻辑错误、类型不一致等问题
 * 2026-04-05 修复：拼写错误IDIE->IDLE、浮点改定点、不修改用户参数、标准memcpy、
 *              移除静态变量、整数下溢防护、const正确性、UINT32_MAX
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "b_protocol_core.h"

#include "memory_pool/memory_pool.h"

#define IDLE_FRAME_FACTOR_NUMERATOR ((1000000UL * 10 * (IDLE_FACTOR + 1)))
#define IDLE_FRAME_FACTOR_DENOMINATOR (FRAME_BAUD_RATE * IDLE_TIMER_US)

static inline uint8_t b_buf_safe_access(const uint8_t* ptr, size_t idx,
                                        size_t max) {
  return (ptr && idx < max) ? ptr[idx] : 0;
}

void printHexAscii(const unsigned char* data, const size_t length) {
#if (EN_FRAME_DEBUG_LV != 0)
  if (!data || length == 0) {
    return;
  }

  for (size_t i = 0; i < length; i++) {
    FRAME_RAW_INFO_PRINTF("%02X ", data[i]);
  }
  FRAME_RAW_INFO_PRINTF("\n");
#else
  (void)data;
  (void)length;
#endif
}

void b_frame_idle_timer(b_frame_type* pframe) {
  if (!pframe) {
    return;
  }

  pframe->_idle_timer++;
  pframe->_systick++;
}

b_error_t b_frame_init(b_frame_type* pframe,
                       const b_frame_init_type* pframeinit) {
  if ((!pframeinit) || (!pframe)) {
    FRAME_LOG_INFO_PRINTF("b_frame_init-err:frame parameter error\r\n");
    return B_ERROR_NULL_PTR;
  }

  if (!pframeinit->out_frame_buffer) {
    FRAME_LOG_INFO_PRINTF("b_frame_init-err:out_frame_buffer is null\r\n");
    return B_ERROR_NULL_PTR;
  }

  uint16_t head_len = pframeinit->head_len;
  uint16_t end_len = pframeinit->end_len;

  if (pframeinit->head == NULL) head_len = 0;
  if (pframeinit->end == NULL) end_len = 0;

  if (pframeinit->out_buffer_len < (head_len + end_len + 1)) {
    FRAME_LOG_INFO_PRINTF(
        "b_frame_init-err:out_buffer_len too small (need at least head_len + "
        "end_len + 1)\r\n");
    return B_ERROR_BUFFER_TOO_SMALL;
  }

  if (pframe->_is_initialized) {
    b_frame_deinit(pframe);
  }

  pframe->frame_init.pname = pframeinit->pname;
  pframe->frame_init.check_cb = pframeinit->check_cb;
  pframe->frame_init.end = pframeinit->end;
  pframe->frame_init.end_len = end_len;
  pframe->frame_init.get_frame_len_cb = pframeinit->get_frame_len_cb;
  pframe->frame_init.head = pframeinit->head;
  pframe->frame_init.head_len = head_len;
  pframe->frame_init.out_frame_buffer = pframeinit->out_frame_buffer;
  pframe->frame_init.in_buffer_len = pframeinit->in_buffer_len;
  pframe->frame_init.out_buffer_len = pframeinit->out_buffer_len;
  pframe->head_match_flg = 0;
  pframe->_last_log_time = 0;

  pframe->_frame_ring = kfifo_alloc(pframe->frame_init.in_buffer_len, NULL);
  if (pframe->_frame_ring == NULL) {
    FRAME_LOG_INFO_PRINTF(
        "%s-错误:环形缓冲区初始化失败\r\n",
        pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
    return B_ERROR_MEMORY_ALLOC;
  }

  b_frame_fifo_clear(pframe);
  pframe->_is_initialized = 1;

  FRAME_LOG_INFO_PRINTF(
      "%s-成功:协议帧初始化成功\r\n",
      pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
  return B_SUCCESS;
}

void b_frame_deinit(b_frame_type* pframe) {
  if (!pframe) {
    return;
  }

  if (pframe->_frame_ring != NULL) {
    kfifo_free(pframe->_frame_ring);
    pframe->_frame_ring = NULL;
  }

  b_frame_fifo_clear(pframe);
  pframe->_is_initialized = 0;
  FRAME_LOG_INFO_PRINTF(
      "%s-信息:协议帧反初始化成功\r\n",
      pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
}

uint8_t b_frame_is_initialized(const b_frame_type* pframe) {
  return (pframe && pframe->_is_initialized) ? 1 : 0;
}

b_error_t b_frame_put(b_frame_type* pframe, const uint8_t* dat, uint32_t len) {
  if (!pframe || !dat || len == 0 || !pframe->_frame_ring ||
      !pframe->_is_initialized) {
    FRAME_LOG_INFO_PRINTF(
        "%s-错误:b_frame_put参数错误\r\n",
        pframe && pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
    return B_ERROR_PARAM_INVALID;
  }

  pframe->_idle_timer = 0;
  uint32_t putlen = kfifo_put(pframe->_frame_ring, (void*)dat, len);
  if (putlen != len) {
    kfifo_reset(pframe->_frame_ring);
    FRAME_LOG_INFO_PRINTF(
        "%s-错误:环形缓冲区已满，实际写入=%lu，期望写入=%lu\r\n",
        pframe->frame_init.pname, (unsigned long)putlen, (unsigned long)len);
    return B_ERROR_BUFFER_OVERFLOW;
  }

  return B_SUCCESS;
}

void b_frame_fifo_clear(b_frame_type* pframe) {
  if (!pframe) {
    FRAME_LOG_INFO_PRINTF("b_frame_fifo_clear-错误:pframe为空指针\r\n");
    return;
  }

  if (pframe->_frame_ring) {
    kfifo_reset(pframe->_frame_ring);
  }

  pframe->head_match_flg = 0;
  pframe->_idle_timer = 0;
  pframe->_last_log_time = 0;
}

uint32_t b_frame_fifo_get(b_frame_type* pframe, uint8_t* dat, uint32_t len,
                          uint32_t timeout) {
  if (!pframe || !dat || !pframe->_frame_ring || !pframe->_is_initialized) {
    FRAME_LOG_INFO_PRINTF(
        "%s-错误:b_frame_fifo_get参数错误\r\n",
        pframe && pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
    return 0;
  }

  const uint32_t start_ticks = pframe->_systick;
  uint32_t elapsed_ticks = 0;

  do {
    const uint32_t ring_len = kfifo_len(pframe->_frame_ring);

    if (ring_len < len) {
      uint32_t current_ticks = pframe->_systick;

      if (current_ticks >= start_ticks) {
        elapsed_ticks = current_ticks - start_ticks;
      } else {
        elapsed_ticks = (UINT32_MAX - start_ticks) + current_ticks + 1;
      }

      if (elapsed_ticks > (UINT32_MAX / 1000u)) {
        elapsed_ticks = timeout;
      } else {
        if (IDLE_TIMER_US > 0) {
          elapsed_ticks = (elapsed_ticks * 1000ul) / IDLE_TIMER_US;
        } else {
          elapsed_ticks = 0;
        }
      }

      if (timeout == 0) {
        break;
      }
    } else {
      FRAME_LOG_INFO_PRINTF(
          "%s-成功:b_frame_fifo_get>>> 环形缓冲区长度 = %lu，期望长度 = "
          "%lu\r\n",
          pframe->frame_init.pname, (unsigned long)ring_len,
          (unsigned long)len);
      break;
    }

  } while (elapsed_ticks < timeout);

  if (elapsed_ticks >= timeout) {
    FRAME_LOG_INFO_PRINTF("%s-错误:b_frame_fifo_get>>> 超时 %lu毫秒\r\n",
                          pframe->frame_init.pname,
                          (unsigned long)elapsed_ticks);
    return 0;
  }

  uint32_t getlen = kfifo_get(pframe->_frame_ring, dat, len);
  return getlen;
}

static b_error_t b_check_head(const b_frame_type* pframe) {
  if (pframe->frame_init.head_len == 0) return B_SUCCESS;

  uint8_t i = 0;
  uint8_t tmp;
  const uint16_t len = kfifo_len(pframe->_frame_ring);

  if (len < (pframe->frame_init.head_len + pframe->frame_init.end_len + 1)) {
    return B_ERROR_FRAME_INVALID;
  }

  do {
    if (!kfifo_get(pframe->_frame_ring, &tmp, 1)) {
      break;
    }

    if (tmp == b_buf_safe_access(pframe->frame_init.head, i,
                                 pframe->frame_init.head_len)) {
      if (++i == pframe->frame_init.head_len) {
        return B_SUCCESS;
      }
    } else {
      i = 0;
    }
  } while (1);

  return B_ERROR_FRAME_INVALID;
}

const uint8_t* b_frame_check_get(b_frame_type* pframe, uint16_t* len) {
  if (!pframe || !len || !pframe->_frame_ring || !pframe->_is_initialized) {
    FRAME_LOG_INFO_PRINTF("b_frame_check_get-错误:参数错误\r\n");
    return NULL;
  }

  uint16_t i, j;
  uint16_t unhead_frame_len;

  if (!pframe->head_match_flg) {
    const uint8_t err = b_check_head(pframe);
    if (err != B_SUCCESS) {
      return NULL;
    }
    if (pframe->frame_init.head_len != 0) {
      memcpy(pframe->frame_init.out_frame_buffer, pframe->frame_init.head,
             pframe->frame_init.head_len);
    }
    pframe->head_match_flg = 1;
    FRAME_LOG_INFO_PRINTF("%s-成功:帧头匹配成功\r\n", pframe->frame_init.pname);
  }

  if (pframe->frame_init.get_frame_len_cb != NULL) {
    uint16_t remaining_buffer =
        (pframe->frame_init.out_buffer_len > pframe->frame_init.head_len)
            ? (pframe->frame_init.out_buffer_len - pframe->frame_init.head_len)
            : 0;

    uint16_t data_len = kfifo_peek(
        pframe->_frame_ring,
        &pframe->frame_init.out_frame_buffer[pframe->frame_init.head_len],
        remaining_buffer, 0);

    if (data_len > 0 && data_len <= remaining_buffer) {
      unhead_frame_len = pframe->frame_init.get_frame_len_cb(
          pframe->frame_init.out_frame_buffer,
          data_len + pframe->frame_init.head_len);
    } else {
      unhead_frame_len = 0;
    }

    if (unhead_frame_len >= pframe->frame_init.head_len) {
      unhead_frame_len = unhead_frame_len - pframe->frame_init.head_len;
    }

    if ((pframe->frame_init.head_len + unhead_frame_len) >
        pframe->frame_init.out_buffer_len) {
      FRAME_LOG_INFO_PRINTF(
          "%s-错误:帧长度超出缓冲区大小 (帧头=%u, 数据=%u, 缓冲区=%u)\r\n",
          pframe->frame_init.pname, pframe->frame_init.head_len,
          unhead_frame_len, pframe->frame_init.out_buffer_len);
      pframe->head_match_flg = 0;
      if (pframe->frame_init.head_len == 0) {
        kfifo_skip(pframe->_frame_ring, 1);
      }
      return NULL;
    }

    if ((data_len < unhead_frame_len) || (unhead_frame_len == 0)) {
      uint32_t idle_threshold =
          (uint32_t)(((uint64_t)IDLE_FRAME_FACTOR_NUMERATOR *
                      (unhead_frame_len + 1)) /
                     IDLE_FRAME_FACTOR_DENOMINATOR);

      if (pframe->_idle_timer > idle_threshold) {
        FRAME_LOG_INFO_PRINTF("%s-错误:idle_timer 超时 %ums (需要 %ums)\r\n",
                              pframe->frame_init.pname, pframe->_idle_timer,
                              idle_threshold);
        pframe->_idle_timer = 0;
        pframe->head_match_flg = 0;
        pframe->_last_log_time = 0;
        if (pframe->frame_init.head_len == 0) {
          kfifo_skip(pframe->_frame_ring, 1);
        }
      } else {
#if (EN_FRAME_DEBUG_LV == 2)
        if (pframe->_last_log_time == 0) {
          pframe->_last_log_time = 1;
          FRAME_LOG_INFO_PRINTF("%s-信息:等待更多数据 (需要 %u, 当前 %u)\r\n",
                                pframe->frame_init.pname, unhead_frame_len,
                                data_len);
        }
#else
        (void)idle_threshold;
#endif
      }
      return NULL;
    }
  } else {
    FRAME_LOG_INFO_PRINTF("%s-错误:get_frame_len_cb为空指针\r\n",
                          pframe->frame_init.pname);
    return NULL;
  }

  uint16_t total_len = unhead_frame_len + pframe->frame_init.head_len;

  if (total_len < pframe->frame_init.end_len) {
    FRAME_LOG_INFO_PRINTF("%s-错误:帧长度小于帧尾长度\r\n",
                          pframe->frame_init.pname);
    pframe->head_match_flg = 0;
    return NULL;
  }

  i = total_len - pframe->frame_init.end_len;

  for (j = 0; j < pframe->frame_init.end_len;) {
    if (b_buf_safe_access(pframe->frame_init.out_frame_buffer, i,
                          pframe->frame_init.out_buffer_len) ==
        b_buf_safe_access(pframe->frame_init.end, j,
                          pframe->frame_init.end_len)) {
      i++;
      j++;
    } else {
      break;
    }
  }

  FRAME_LOG_INFO_PRINTF("%s-信息:数据>>>\r\n", pframe->frame_init.pname);
  printHexAscii(pframe->frame_init.out_frame_buffer,
                pframe->frame_init.head_len + unhead_frame_len);

  if (j == pframe->frame_init.end_len) {
    *len = pframe->frame_init.head_len + unhead_frame_len;
    if (pframe->frame_init.check_cb != NULL) {
      const uint8_t err = pframe->frame_init.check_cb(
          pframe->frame_init.out_frame_buffer, *len);
      if (err == B_SUCCESS) {
        pframe->head_match_flg = 0;
        pframe->_last_log_time = 0;
        kfifo_skip(pframe->_frame_ring, unhead_frame_len);
        FRAME_LOG_INFO_PRINTF("%s-成功:帧完整匹配成功\r\n",
                              pframe->frame_init.pname);
        return pframe->frame_init.out_frame_buffer;
      }
      if (pframe->frame_init.head_len == 0) {
        kfifo_skip(pframe->_frame_ring, 1);
      }
      FRAME_LOG_INFO_PRINTF("%s-错误:帧校验失败\r\n", pframe->frame_init.pname);
    }
  }
  pframe->head_match_flg = 0;
  pframe->_last_log_time = 0;
  return NULL;
}
