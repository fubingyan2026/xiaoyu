#include "bsp_delay.h"
#include "main.h"

static uint8_t fac_us = 0;
static uint32_t fac_ms = 0;

/**
 * @brief 初始化延时函数
 *
 * 该函数用于初始化延时相关的参数，根据系统核心时钟计算出微妙和毫秒的系数。
 */
void delay_init(void)
{
    fac_us = SystemCoreClock / 1000000;
    fac_ms = SystemCoreClock / 1000;
}

/**
 * @brief 产生指定微秒数的延时
 *
 * 该函数通过SysTick定时器实现微秒级别的延时。根据输入的微秒数计算出需要等待的SysTick计数值，并通过循环比较当前SysTick值与目标值来实现延时。
 *
 * @param nus 需要延时的微秒数
 */
void delay_us(const uint16_t nus)
{
    uint32_t ticks = 0;
    uint32_t told = 0;
    uint32_t tnow = 0;
    uint32_t tcnt = 0;
    uint32_t reload = 0;
    reload = SysTick->LOAD;
    ticks = nus * fac_us;
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

/**
 * @brief 产生指定毫秒数的延时
 *
 * 该函数使用SysTick定时器来实现精确的毫秒级延时。用户需要通过`nms`参数指定延时的毫秒数。
 * 此函数依赖于之前初始化过程中计算出的毫秒系数`fac_ms`，因此在调用此函数前应确保已经正确初始化了延时相关的参数。
 *
 * @param nms 指定要延迟的毫秒数
 */
void delay_ms(const uint16_t nms)
{
    uint32_t ticks = 0;
    uint32_t told = 0;
    uint32_t tnow = 0;
    uint32_t tcnt = 0;
    uint32_t reload = 0;
    reload = SysTick->LOAD;
    ticks = nms * fac_ms;
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

/**
 * @brief 检查SysTick计数器是否至少递减到0一次
 * @return 如果COUNTFLAG位为1，表示计数器已经至少递减到0一次，并且该标志在读取后自动清零；否则返回0。
 */
__STATIC_INLINE uint32_t LL_SYSTICK_IsActiveCounterFlag(void)
{
    // 判断COUNTFLAG位是否为1是1则计数器已经递减 0了至少一次。读取该位后该位自动清零
    return (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == SysTick_CTRL_COUNTFLAG_Msk;
}

/**
 * @brief 获取微秒级时间
 * @return 自系统启动以来的微秒数
 */
uint32_t micros(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    LL_SYSTICK_IsActiveCounterFlag(); // 清除计数值"溢出"标志位
    uint32_t m = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (LL_SYSTICK_IsActiveCounterFlag())
    {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    return m * 1000 + u * 1000 / tms;
}

/**
 * @brief 获取系统运行时间（毫秒）
 *
 * 该函数返回自系统启动以来的毫秒数。
 *
 * @return 系统启动以来的毫秒数
 */
uint32_t millis(void)
{
    return HAL_GetTick();
}

uint32_t TimeCnt[6] = {0};
uint32_t lastTimeCnt[6] = {0};

/**
 * 获取运行时间(us级别)计时工具，计算运行周期或经过时间
 * @param count
 */
extern void Get_Time_Hook(uint8_t count)
{
    if (count > sizeof(TimeCnt) / sizeof(TimeCnt[0]) - 1)
    {
        count = sizeof(TimeCnt) / sizeof(TimeCnt[0]) - 1;
    }
    const uint32_t x = micros();
    TimeCnt[count] = x - lastTimeCnt[count];
    lastTimeCnt[count] = x;
}

/**
 * @brief 获取时间计数指针
 * @return 返回指向TimeCnt数组的指针
 */
extern const uint32_t* get_time_tick_pointer(void)
{
    return TimeCnt;
}
