/**
 * @brief:  通用协议解析器(依赖kfifo模块) - 优化版本
 * @FilePath: b_protocol_core.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-10-13 19:25:52
 * @LastEditTime: 2026-01-21 修复缓冲区溢出、类型问题、逻辑错误等
 * @version: V1.1.0
 * @note        Author         Email                   Notes
 * 2023-09-08   Initial version
 * 2023-09-18   修改接口设计，支持多协议，优化调试日志
 * 2023-10-23	增加接口函数b_frame_fifo_get，用于从fifo中阻塞式获取预期长度数据
 * 2024-07-04	优化帧头帧尾长度为0情况下的支持
 * 2024-07-05	优化uart_get_frame_len_cb返回值为0时的处理策略
 * 2024-07-09	帧头为0时,校验不通过应至少丢弃1个数据
 * 2024-07-09	输入/输出缓存大小分开配置，增加调试信息等级配置
 * 2024-12-20	修复对多协议的支持的相关bug
 * 2025-09-04	输入输出缓冲区大小不通过宏定义配置，在初始化时进行配置，方便多协议解析器时合理利用内存
 * 2025-10-13   修复安全隐患：内存管理、缓冲区溢出、类型转换、空指针等问题
 * 2026-01-21   修复严重缓冲区溢出、kfifo_clear_len逻辑错误、类型不一致等问题
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "b_protocol_core.h"
#include "memory_pool/memory_pool.h"

// 空闲定时器的帧长度因子计算（基于波特率和空闲定时器周期，考虑每个字节10位：1起始位 + 8数据位 + 1停止位）
const float IDIE_FRAME_FACTOR = (((1000000.0f / FRAME_BAUD_RATE) * 10 * (IDIE_FACTOR + 1)) / IDIE_TIMER_US);

/**
 * 安全的缓冲区访问函数
 * @param ptr 缓冲区指针
 * @param idx 索引位置
 * @param max 最大索引值
 * @return 如果索引有效返回对应字节，否则返回0
 */
static inline uint8_t b_buf_safe_access(const uint8_t *ptr, size_t idx, size_t max)
{
    return (ptr && idx < max) ? ptr[idx] : 0;
}

/**
 * @brief 打印原始数据的十六进制和ASCII码
 * @param data 指向要打印数据的指针
 * @param length 要打印的数据长度
 */
void printHexAscii(const unsigned char *data, const size_t length)
{
#if (EN_FRAME_DEBUG_LV != 0)
    if (!data || length == 0)
    {
        return;
    }

    for (size_t i = 0; i < length; i++)
    {
        FRAME_RAW_INFO_PRINTF("%02X ", data[i]);
    }
    FRAME_RAW_INFO_PRINTF("\n");
#endif
}

/**
 * @brief 协议帧空闲定时器
 * @param pframe 指向b_frame_type结构体的指针，表示协议帧
 */
void b_frame_idie_timer(b_frame_type *pframe)
{
    if (!pframe)
    {
        return;
    }

    pframe->_idie_timer++;
    pframe->_systick++;
}

/**
 * 初始化协议帧
 * @param pframe 指向b_frame_type结构体的指针，表示协议帧
 * @param pframeinit 指向b_frame_init_type结构体的指针，包含初始化参数
 * @return 如果初始化成功返回B_SUCCESS，否则返回具体的错误码
 */
b_error_t b_frame_init(b_frame_type *pframe, b_frame_init_type *pframeinit)
{
    if ((!pframeinit) || (!pframe))
    {
        FRAME_LOG_INFO_PRINTF("b_frame_init-err:frame parameter error\r\n");
        return B_ERROR_NULL_PTR;
    }

    if (!pframeinit->out_frame_buffer)
    {
        FRAME_LOG_INFO_PRINTF("b_frame_init-err:out_frame_buffer is null\r\n");
        return B_ERROR_NULL_PTR;
    }

    // 检查缓冲区大小是否足够
    if (pframeinit->out_buffer_len < (pframeinit->head_len + pframeinit->end_len + 1))
    {
        FRAME_LOG_INFO_PRINTF("b_frame_init-err:out_buffer_len too small (need at least head_len + end_len + 1)\r\n");
        return B_ERROR_BUFFER_TOO_SMALL;
    }

    // 如果已经初始化，先清理资源
    if (pframe->_is_initialized)
    {
        b_frame_deinit(pframe);
    }

    // 安全地处理空指针
    if (pframeinit->head == NULL)
        pframeinit->head_len = 0;
    if (pframeinit->end == NULL)
        pframeinit->end_len = 0;

    // 复制初始化参数
    pframe->frame_init.pname = pframeinit->pname;
    pframe->frame_init.check_cb = pframeinit->check_cb;
    pframe->frame_init.end = (const uint8_t *)pframeinit->end; // 类型安全转换
    pframe->frame_init.end_len = pframeinit->end_len;
    pframe->frame_init.get_frame_len_cb = pframeinit->get_frame_len_cb;
    pframe->frame_init.head = (const uint8_t *)pframeinit->head; // 类型安全转换
    pframe->frame_init.head_len = pframeinit->head_len;
    pframe->frame_init.out_frame_buffer = pframeinit->out_frame_buffer;
    pframe->frame_init.in_buffer_len = pframeinit->in_buffer_len;
    pframe->frame_init.out_buffer_len = pframeinit->out_buffer_len;
    pframe->head_match_flg = 0;

    // 分配环形缓冲区
    pframe->_frame_ring = kfifo_alloc(pframe->frame_init.in_buffer_len, NULL);
    if (pframe->_frame_ring == NULL)
    {
        FRAME_LOG_INFO_PRINTF("%s-错误:环形缓冲区初始化失败\r\n",
                              pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
        return B_ERROR_MEMORY_ALLOC;
    }

    // 初始化帧状态
    b_frame_fifo_clear(pframe);
    pframe->_is_initialized = 1;

    FRAME_LOG_INFO_PRINTF("%s-成功:协议帧初始化成功\r\n", pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
    return B_SUCCESS;
}

/**
 * 反初始化协议帧，释放相关资源
 * @param pframe 指向b_frame_type结构体的指针，表示协议帧
 */
void b_frame_deinit(b_frame_type *pframe)
{
    if (!pframe)
    {
        return;
    }

    if (pframe->_frame_ring != NULL)
    {
        kfifo_free(pframe->_frame_ring);
        pframe->_frame_ring = NULL;
    }

    b_frame_fifo_clear(pframe);
    pframe->_is_initialized = 0;
    FRAME_LOG_INFO_PRINTF("%s-信息:协议帧反初始化成功\r\n",
                          pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
}

/**
 * 检查协议帧是否已初始化
 * @param pframe 指向b_frame_type结构体的指针，表示协议帧
 * @return 如果协议帧已初始化且指针有效返回1，否则返回0
 * @note 此函数用于安全地检查协议帧的初始化状态，避免对未初始化的帧进行操作
 */
uint8_t b_frame_is_initialized(const b_frame_type *pframe)
{
    return (pframe && pframe->_is_initialized) ? 1 : 0;
}

/**
 * 将数据放入帧缓冲区
 * @param pframe 指向b_frame_type结构体的指针，表示协议帧
 * @param dat 指向要放入帧缓冲区的数据的指针
 * @param len 要放入帧缓冲区的数据长度
 * @return 如果操作成功返回B_SUCCESS，否则返回具体的错误码
 */
b_error_t b_frame_put(b_frame_type *pframe, uint8_t *dat, const uint32_t len)
{
    if (!pframe || !dat || len == 0 || !pframe->_frame_ring || !pframe->_is_initialized)
    {
        FRAME_LOG_INFO_PRINTF("%s-错误:b_frame_put参数错误\r\n",
                              pframe && pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
        return B_ERROR_PARAM_INVALID;
    }

    pframe->_idie_timer = 0;
    uint32_t putlen = kfifo_put(pframe->_frame_ring, dat, len);
    if (putlen != len)
    {
        kfifo_reset(pframe->_frame_ring);
        FRAME_LOG_INFO_PRINTF("%s-错误:环形缓冲区已满，实际写入=%lu，期望写入=%lu\r\n", pframe->frame_init.pname,
                              (unsigned long)putlen, (unsigned long)len);
        return B_ERROR_BUFFER_OVERFLOW;
    }

    return B_SUCCESS;
}

/**
 * 清空帧FIFO并重置相关标志
 * @param pframe 协议帧结构体指针
 */
void b_frame_fifo_clear(b_frame_type *pframe)
{
    if (!pframe)
    {
        FRAME_LOG_INFO_PRINTF("b_frame_fifo_clear-错误:pframe为空指针\r\n");
        return;
    }

    if (pframe->_frame_ring)
    {
        kfifo_reset(pframe->_frame_ring);
    }

    pframe->head_match_flg = 0;
    pframe->_idie_timer = 0;
}

/**
 * 从帧FIFO中获取数据
 * @param pframe 协议帧结构体指针
 * @param dat 用于存储获取到的数据的缓冲区
 * @param len 要从FIFO中读取的数据长度
 * @param timeout 等待数据可用的最大时间（单位：毫秒）
 * @return 返回实际从FIFO中读取到的数据长度，如果超时或参数错误则返回0
 */
uint32_t b_frame_fifo_get(b_frame_type *pframe, uint8_t *dat, uint32_t len, uint32_t timeout)
{
    if (!pframe || !dat || !pframe->_frame_ring || !pframe->_is_initialized)
    {
        FRAME_LOG_INFO_PRINTF("%s-错误:b_frame_fifo_get参数错误\r\n",
                              pframe && pframe->frame_init.pname ? pframe->frame_init.pname : "未知");
        return 0;
    }

    const uint32_t start_ticks = pframe->_systick;
    uint32_t elapsed_ticks = 0;

    do
    {
        const uint32_t ring_len = kfifo_len(pframe->_frame_ring);

        if (ring_len < len)
        {
            // 增强时间计算安全性
            uint32_t current_ticks = pframe->_systick;

            // 处理systick溢出的情况
            if (current_ticks >= start_ticks)
            {
                elapsed_ticks = current_ticks - start_ticks;
            }
            else
            {
                // 安全处理32位计数器溢出
                elapsed_ticks = (0xFFFFFFFFu - start_ticks) + current_ticks + 1;
            }

            // 防止乘法溢出并安全转换为毫秒
            if (elapsed_ticks > (UINT32_MAX / 1000u))
            {
                elapsed_ticks = timeout; // 强制超时
            }
            else
            {
                // 安全的时间转换，避免除零错误
                if (IDIE_TIMER_US > 0)
                {
                    elapsed_ticks = (elapsed_ticks * 1000ul) / IDIE_TIMER_US;
                }
                else
                {
                    elapsed_ticks = 0; // 防止除零
                }
            }

            // 防止无限循环
            if (timeout == 0)
            {
                break; // 非阻塞模式立即返回
            }
        }
        else
        {
            FRAME_LOG_INFO_PRINTF("%s-成功:b_frame_fifo_get>>> 环形缓冲区长度 = %lu，期望长度 = %lu\r\n",
                                  pframe->frame_init.pname, (unsigned long)ring_len, (unsigned long)len);
            break;
        }

        // 添加小延迟避免过度消耗CPU
        // 在实际应用中可能需要根据具体平台调整

    } while (elapsed_ticks < timeout);

    if (elapsed_ticks >= timeout)
    {
        FRAME_LOG_INFO_PRINTF("%s-错误:b_frame_fifo_get>>> 超时 %lu毫秒\r\n", pframe->frame_init.pname,
                              (unsigned long)elapsed_ticks);
        return 0;
    }

    uint32_t getlen = kfifo_get(pframe->_frame_ring, dat, len);
    return getlen;
}

/**
 * 检查帧头是否匹配
 * @param pframe 协议帧结构体指针
 * @return 返回B_SUCCESS表示帧头匹配成功，返回B_ERROR_FRAME_INVALID表示帧头匹配失败
 */
static b_error_t b_check_head(const b_frame_type *pframe)
{
    if (pframe->frame_init.head_len == 0)
        return B_SUCCESS;

    uint8_t i = 0;
    uint8_t tmp;
    const uint16_t len = kfifo_len(pframe->_frame_ring);

    if (len < (pframe->frame_init.head_len + pframe->frame_init.end_len + 1))
    {
        return B_ERROR_FRAME_INVALID;
    }

    do
    {
        if (!kfifo_get(pframe->_frame_ring, &tmp, 1))
        {
            break;
        }

        // 安全类型比较，避免符号扩展问题
        if (tmp == b_buf_safe_access(pframe->frame_init.head, i, pframe->frame_init.head_len))
        {
            if (++i == pframe->frame_init.head_len)
            {
                return B_SUCCESS;
            }
        }
        else
        {
            i = 0;
        }
    } while (1);

    return B_ERROR_FRAME_INVALID;
}

/**
 * 输出解析结果
 * @param pframe 传入协议类指针
 * @param len 存放解析出来的数据长度
 * @return 解析出来的数据指针,NULL代表未解析出数据
 */
const uint8_t *b_frame_check_get(b_frame_type *pframe, uint16_t *len)
{
    if (!pframe || !len || !pframe->_frame_ring || !pframe->_is_initialized)
    {
        FRAME_LOG_INFO_PRINTF("b_frame_check_get-错误:参数错误\r\n");
        return NULL;
    }

    uint16_t i, j;
    uint16_t unhead_frame_len;

    if (!pframe->head_match_flg)
    {
        const uint8_t err = b_check_head(pframe);
        if (err != B_SUCCESS)
        {
            return NULL;
        }
        if (pframe->frame_init.head_len != 0)
        {
            __memcpy(pframe->frame_init.out_frame_buffer, (void *)(pframe->frame_init.head),
                     pframe->frame_init.head_len);
        }
        pframe->head_match_flg = 1;
        FRAME_LOG_INFO_PRINTF("%s-成功:帧头匹配成功\r\n", pframe->frame_init.pname);
    }

    if (pframe->frame_init.get_frame_len_cb != NULL)
    {
        // 安全计算剩余缓冲区大小，防止溢出
        uint16_t remaining_buffer = (pframe->frame_init.out_buffer_len > pframe->frame_init.head_len)
                                        ? (pframe->frame_init.out_buffer_len - pframe->frame_init.head_len)
                                        : 0;

        // 安全地从FIFO中读取数据
        uint16_t data_len =
            kfifo_peek(pframe->_frame_ring, &pframe->frame_init.out_frame_buffer[pframe->frame_init.head_len],
                       remaining_buffer, 0);

        // 安全地调用帧长度计算回调
        if (data_len > 0 && data_len <= remaining_buffer)
        {
            unhead_frame_len = pframe->frame_init.get_frame_len_cb(pframe->frame_init.out_frame_buffer,
                                                                   data_len + pframe->frame_init.head_len);
        }
        else
        {
            unhead_frame_len = 0;
        }

        if (unhead_frame_len >= pframe->frame_init.head_len)
        {
            unhead_frame_len = unhead_frame_len - pframe->frame_init.head_len;
        }

        // 增强缓冲区边界检查
        if ((pframe->frame_init.head_len + unhead_frame_len) > pframe->frame_init.out_buffer_len)
        {
            FRAME_LOG_INFO_PRINTF("%s-错误:帧长度超出缓冲区大小 (帧头=%u, 数据=%u, 缓冲区=%u)\r\n",
                                  pframe->frame_init.pname, pframe->frame_init.head_len, unhead_frame_len,
                                  pframe->frame_init.out_buffer_len);
            pframe->head_match_flg = 0;
            if (pframe->frame_init.head_len == 0)
            {
                kfifo_skip(pframe->_frame_ring, 1);
            }
            return NULL;
        }

        if ((data_len < unhead_frame_len) || (unhead_frame_len == 0))
        {
            static uint32_t last_log_time = 0;

            if (pframe->_idie_timer > (uint32_t)(IDIE_FRAME_FACTOR * (unhead_frame_len + 1)))
            {
                FRAME_LOG_INFO_PRINTF("%s-错误:idie_timer 超时 %dms (需要 %dms)\r\n", pframe->frame_init.pname,
                                      pframe->_idie_timer, (uint32_t)(IDIE_FRAME_FACTOR * (unhead_frame_len + 1)));
                pframe->_idie_timer = 0;
                pframe->head_match_flg = 0;
                last_log_time = 0; // 重置日志时间，避免过度输出
                if (pframe->frame_init.head_len == 0)
                {
                    kfifo_skip(pframe->_frame_ring, 1);
                }
            }
            else
            {
                // 数据不足但未超时，保持head_match_flg为1，等待更多数据
                // 但需要限制调试信息输出频率，避免过度占用资源
#if (EN_FRAME_DEBUG_LV == 2)

                if (last_log_time == 0)
                {
                    last_log_time = 1;
                    FRAME_LOG_INFO_PRINTF("%s-信息:等待更多数据 (需要 %d, 当前 %d)\r\n", pframe->frame_init.pname,
                                          unhead_frame_len, data_len);
                }
#endif
            }
            return NULL;
        }
    }
    else
    {
        FRAME_LOG_INFO_PRINTF("%s-错误:get_frame_len_cb为空指针\r\n", pframe->frame_init.pname);
        return NULL;
    }

    i = unhead_frame_len + pframe->frame_init.head_len - pframe->frame_init.end_len;

    for (j = 0; j < pframe->frame_init.end_len;)
    {
        // 安全类型比较，避免符号扩展问题
        if (b_buf_safe_access(pframe->frame_init.out_frame_buffer, i, pframe->frame_init.out_buffer_len) ==
            b_buf_safe_access(pframe->frame_init.end, j, pframe->frame_init.end_len))
        {
            i++;
            j++;
        }
        else
        {
            break;
        }
    }

    FRAME_LOG_INFO_PRINTF("%s-信息:数据>>>\r\n", pframe->frame_init.pname);
    printHexAscii(pframe->frame_init.out_frame_buffer, pframe->frame_init.head_len + unhead_frame_len);

    if (j == pframe->frame_init.end_len)
    {
        *len = pframe->frame_init.head_len + unhead_frame_len;
        if (pframe->frame_init.check_cb != NULL)
        {
            const uint8_t err = pframe->frame_init.check_cb(pframe->frame_init.out_frame_buffer, *len);
            if (err == B_SUCCESS)
            {
                pframe->head_match_flg = 0;
                kfifo_skip(pframe->_frame_ring, unhead_frame_len);
                FRAME_LOG_INFO_PRINTF("%s-成功:帧完整匹配成功\r\n", pframe->frame_init.pname);
                return pframe->frame_init.out_frame_buffer;
            }
            if (pframe->frame_init.head_len == 0)
            {
                kfifo_skip(pframe->_frame_ring, 1);
            }
            FRAME_LOG_INFO_PRINTF("%s-错误:帧校验失败\r\n", pframe->frame_init.pname);
        }
    }
    pframe->head_match_flg = 0;
    return NULL;
}