/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for stm32f10x platform.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug/debug.h"
#include "key_function.h"
#include "memory_pool/memory_pool.h"
#include "stm32g4xx_hal.h"

/* default environment variables set for user */
static const ef_env default_env_set[] = {
    {"BT", 0, 4},
    {"CAN-ID", &can_save_id, sizeof(can_save_id)},
};

/**
 * @brief   Flash编程粒度（STM32G4支持64位双字编程）
 */
#define FLASH_PROGRAM_SIZE 8

/**
 * @brief   日志缓冲区
 */
static char log_buf[256];

/**
 * @brief   Flash写入临时缓冲区（8字节对齐）
 */
static uint64_t write_data;

/**
 * @brief   Flash读取临时缓冲区（8字节对齐）
 */
static uint64_t read_data;

/**
 * @brief   环境锁嵌套深度计数
 * @note    用于支持嵌套锁调用
 */
static uint8_t env_lock_depth = 0;

/**
 * @brief   Flash端口层初始化
 * @param[out] default_env      默认环境变量指针输出
 * @param[out] default_env_size 默认环境变量数量输出
 * @retval   EF_NO_ERR           初始化成功
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size)
{
    const EfErrCode result = EF_NO_ERR;

    /* 设置默认环境变量表 */
    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

    return result;
}

/**
 * @brief   从Flash读取数据
 * @param[in] addr              Flash起始地址
 * @param[out] buf              数据接收缓冲区
 * @param[in] size              要读取的字节数
 * @retval   EF_NO_ERR          读取成功
 * @retval   EF_READ_ERR        读取失败
 *
 * @note    读取操作以字节为单位，从Flash直接复制到RAM
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, const size_t size)
{
    const EfErrCode result = EF_NO_ERR;
    uint8_t *buf_8 = (uint8_t *)buf;

    /* 从Flash复制数据到RAM */
    for (size_t i = 0; i < size; i++, addr++, buf_8++)
    {
        *buf_8 = *(uint8_t *)addr;
    }

    return result;
}

/**
 * @brief   擦除Flash扇区
 * @param[in] addr              要擦除的Flash起始地址（必须对齐到扇区边界）
 * @param[in] size              要擦除的字节数
 * @retval   EF_NO_ERR          擦除成功
 * @retval   EF_ERASE_ERR       擦除失败
 *
 * @note    STM32G431CBT6的Flash页大小为2KB(0x800)
 * @note    此操作不可逆，调用前请确保数据已备份
 * @note    使用goto语句确保Flash正确上锁
 */
EfErrCode ef_port_erase(const uint32_t addr, const size_t size)
{
    EfErrCode result = EF_NO_ERR;
    FLASH_EraseInitTypeDef erase_init;

    /* 确保起始地址对齐到最小擦除单位 */
    EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

    /* 计算需要擦除的页数 */
    size_t erase_pages = size / PAGE_SIZE;
    if (size % PAGE_SIZE != 0)
    {
        /* 如果不足一页，按一页计算 */
        erase_pages++;
    }

    /* 配置页擦除参数 */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;     ///< 页擦除模式
    erase_init.Page = (addr - 0x8000000) / PAGE_SIZE; ///< 计算页号
    erase_init.NbPages = 1;                           ///< 每次擦除1页
    erase_init.Banks = FLASH_BANK_1;                  ///< 擦除Bank1

    uint32_t error = 0;

    /* 解锁Flash以进行擦除操作 */
    HAL_FLASH_Unlock();

    /* 清除所有相关的Flash标志位 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY |     ///< 忙标志
                           FLASH_FLAG_EOP |     ///< 操作完成标志
                           FLASH_FLAG_WRPERR |  ///< 写保护错误
                           FLASH_FLAG_OPTVERR | ///< 选项字节校验错误
                           FLASH_FLAG_PROGERR | ///< 编程错误
                           FLASH_FLAG_PGSERR |  ///< 编程序列错误
                           FLASH_FLAG_PGAERR);  ///< 编程对齐错误

    /* 循环擦除所有页 */
    for (uint16_t i = 0; i < erase_pages; i++)
    {
        const HAL_StatusTypeDef flash_status = HAL_FLASHEx_Erase(&erase_init, &error);

        if (flash_status != HAL_OK)
        {
            /* 擦除失败，记录错误并退出 */
            ef_print("[Flash] 擦除错误[%d]，地址: 0x%08X\r\n", flash_status, addr);
            result = EF_ERASE_ERR;
            goto EXIT_ERASE; ///< 确保Flash正确上锁
        }

        /* 指向下一页 */
        erase_init.Page++;
    }

EXIT_ERASE:
    /* 无论成功或失败，都必须上锁Flash */
    HAL_FLASH_Lock();

    return result;
}

/**
 * @brief   向Flash写入数据
 * @param[in] offset            Flash偏移地址
 * @param[in] buf              要写入的数据缓冲区
 * @param[in] size             要写入的字节数
 * @retval   EF_NO_ERR         写入成功
 * @retval   EF_WRITE_ERR      写入失败
 *
 * @note    STM32G4支持64位双字(8字节)编程
 * @note    写入前必须先擦除对应的扇区
 * @note    写入过程中会进行数据校验，确保写入正确
 * @note    使用goto语句确保Flash正确上锁
 */
EfErrCode ef_port_write(uint32_t offset, const uint32_t *buf, size_t size)
{
    EfErrCode result = EF_NO_ERR;

    uint32_t addr = offset;                   ///< 当前写入地址
    HAL_StatusTypeDef flash_wstatus = HAL_OK; ///< Flash操作状态
    uint32_t *data = (uint32_t *)buf;         ///< 数据指针

    /* 解锁Flash以进行写入操作 */
    HAL_FLASH_Unlock();

    /* 清除所有相关的Flash标志位 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP |     ///< 操作完成标志
                           FLASH_FLAG_OPERR |   ///< 操作错误
                           FLASH_FLAG_WRPERR |  ///< 写保护错误
                           FLASH_FLAG_PGAERR |  ///< 编程对齐错误
                           FLASH_FLAG_PGSERR |  ///< 编程序列错误
                           FLASH_FLAG_PROGERR); ///< 编程错误

    /* 按8字节双字为单位循环写入 */
    for (size_t i = 0; (i < size); i += FLASH_PROGRAM_SIZE)
    {
        /* 复制8字节数据到对齐缓冲区 */
        __memcpy(&write_data, data, FLASH_PROGRAM_SIZE);

        /* 如果数据全为0xFF（已擦除状态），则跳过写入 */
        if (write_data != 0xFFFFFFFFFFFFFFFF)
        {
            /* 执行双字编程 */
            flash_wstatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, write_data);
        }

        /* 从Flash读取刚写入的数据进行校验 */
        read_data = *(volatile uint64_t *)addr;

        /* 校验数据一致性和操作状态 */
        if ((read_data != *(volatile uint64_t *)data) || (flash_wstatus != HAL_OK))
        {
            /* 写入错误，记录详细错误信息 */
            DEBUG_ERROR("[Flash] 写入错误! 状态=%d, 地址=0x%08X", flash_wstatus, addr);
            result = EF_WRITE_ERR;
            goto EXIT_WRITE; ///< 确保Flash正确上锁
        }

        /* 移动到下一个双字 */
        addr += FLASH_PROGRAM_SIZE;
        data += (FLASH_PROGRAM_SIZE / sizeof(uint32_t));
    }

EXIT_WRITE:
    /* 无论成功或失败，都必须上锁Flash */
    HAL_FLASH_Lock();

    return result;
}

/**
 * @brief   锁定环境变量缓存
 *
 * @note    使用嵌套锁机制，支持多次调用
 * @note    实际只在第一次调用时禁用中断
 */
void ef_port_env_lock(void)
{
    /* 只有第一次调用时才禁用中断 */
    if (env_lock_depth == 0)
    {
        __disable_irq();
    }

    /* 增加锁深度计数 */
    env_lock_depth++;
}

/**
 * @brief   解锁环境变量缓存
 *
 * @note    嵌套解锁，只有当锁深度归零时才启用中断
 */
void ef_port_env_unlock(void)
{
    /* 减少锁深度计数 */
    env_lock_depth--;

    /* 只有当锁深度归零时才启用中断 */
    if (env_lock_depth == 0)
    {
        __enable_irq();
    }
}

/**
 * @brief   输出调试日志
 * @param[in] file             调用此函数的源文件
 * @param[in] line             调用此函数的行号
 * @param[in] format           格式化字符串
 * @param[in] ...             可变参数列表
 *
 * @note    仅在PRINT_DEBUG为1时启用
 */
void ef_log_debug(const char *file, const long line, const char *format, ...)
{
#if PRINT_DEBUG
    /* 输出调试信息头 */
    BSP_Printf("[EasyFlash] [调试](%s:第%ld行) ", file, line);

    va_list args;
    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    BSP_Printf("%s\r\n", log_buf);
    va_end(args);

#endif
}

/**
 * @brief   输出普通信息日志
 * @param[in] format           格式化字符串
 * @param[in] ...             可变参数列表
 */
void ef_log_info(const char *format, ...)
{
    va_list args;

    /* 输出信息日志头 */
    BSP_Printf("[EasyFlash] [信息] ");

    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    BSP_Printf("%s\r\n", log_buf);
    va_end(args);
}

/**
 * @brief   输出一般消息
 * @param[in] format           格式化字符串
 * @param[in] ...             可变参数列表
 */
void ef_print(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    BSP_Printf("[EasyFlash] %s\r\n", log_buf);
    va_end(args);
}
