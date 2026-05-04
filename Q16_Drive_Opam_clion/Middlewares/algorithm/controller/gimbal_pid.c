/**
 * @brief:  云台旋转控制实现文件.
 * @FilePath: gimbal_control.c
 * @author: fubingyan qq:3245784484
 * @Date: 2025-09-22 09:49:55
 * @LastEditTime: 2025-09-26 20:30:30
 * @version: V1.0.0
 * @note: add your note!
 * @copyright (c) 2025 by fubingyan, All Rights Reserved.
 */

#include "stdint.h"
#include "bsp_delay.h"
#include "controller/gimbal_pid.h"
#include "memory_pool.h"

/* 全局链表头指针 */
gimbal_PID_t* g_gimbal_pid_list = NULL;

static const float pi = 3.141592653f;

static float gimbal_PID_calc(gimbal_PID_t* pid);
static float loop_float_constrain_fast(float Input, float minValue, float maxValue);
static float fast_fabsf_ptr(float x);
static void abs_limit_v2(float* num, float Limit);
static uint32_t get_times(void);

/**
 * @brief: 获取当前时间
 * @return {uint32_t} 当前时间(毫秒)
 */
static uint32_t get_times(void)
{
    return millis();
}

/**
 * @brief: 注册一个云台PID
 * @param {gimbal_pid_config_t} *config: PID配置结构体指针
 * @return {gimbal_PID_t*} 返回注册的PID指针，失败返回NULL
 * @note: 如果同名PID已存在，则返回已存在的实例
 */
gimbal_PID_t* GimbalPidRegister(const gimbal_pid_config_t* config)
{
    if (config == NULL || config->name == NULL)
    {
        return NULL;
    }

    /* 检查是否已存在同名实例 */
    gimbal_PID_t* existing = get_gimbal_pid_instance(config->name);
    if (existing != NULL)
    {
        return existing;
    }

    /* 创建新实例 */
    gimbal_PID_t* new_instance = (gimbal_PID_t*)__malloc(sizeof(gimbal_PID_t));
    if (new_instance == NULL)
    {
        return NULL;
    }

    /* 初始化内存 */
    __memset(new_instance, 0, sizeof(gimbal_PID_t));

    /* 复制配置 */
    __memcpy(&new_instance->config, config, sizeof(gimbal_pid_config_t));
    
    /* 添加到链表头部 */
    new_instance->next = g_gimbal_pid_list;
    g_gimbal_pid_list = new_instance;

    return new_instance;
}

/**
 * @brief: 注销云台PID实例
 * @param {gimbal_PID_t*} instance: 要注销的实例指针
 */
void GimbalPidUnregister(gimbal_PID_t* instance)
{
    if (instance == NULL)
    {
        return;
    }

    gimbal_PID_t** current = &g_gimbal_pid_list;
    while (*current != NULL)
    {
        if (*current == instance)
        {
            *current = instance->next;
            __free(instance);
            return;
        }
        current = &(*current)->next;
    }
}

/**
 * @brief: 注销所有云台PID实例
 */
void GimbalPidUnregisterAll(void)
{
    gimbal_PID_t* current = g_gimbal_pid_list;
    while (current != NULL)
    {
        gimbal_PID_t* next = current->next;
        __free(current);
        current = next;
    }
    g_gimbal_pid_list = NULL;
}

/**
 * @brief: 计算并更新云台PID控制器的状态
 * @param: 无
 * @return: 无
 * @note: 此函数通过遍历PID链表，根据时间差和控制周期来决定是否计算新的PID输出。
 *        如果当前PID的时间刻度小于配置的控制周期，则增加时间刻度；否则重置时间刻度，并调用gimbal_PID_calc函数进行PID计算。
 */
void GimbalPidLoop(void)
{
    static uint32_t times = 0, times_last = 0, diff_times = 1;

    times_last = times;
    times = get_times();
    diff_times = times - times_last;

    /* 防止除零错误 */
    if (diff_times == 0)
    {
        diff_times = 1;
    }

    gimbal_PID_t* current = g_gimbal_pid_list;
    while (current != NULL)
    {
        if (current->config.control_cycle > 0)
        {
            if (++current->data.time_tick >= current->config.control_cycle / diff_times)
            {
                current->data.time_tick = 0;
                gimbal_PID_calc(current);
            }
        }
        current = current->next;
    }
}

/**
 * @brief 更新云台PID的反馈信息
 * @param instance 云台PID实例指针
 * @param update 包含更新信息的结构体指针，包含get, set, error_delta等字段
 * @note 如果传入的instance为NULL，则函数直接返回，不做任何处理
 */
void GimbalPidUpdateFeedBack(gimbal_PID_t* instance, const gimbal_pid_update_t* update)
{
    if (instance == NULL || update == NULL)
    {
        return;
    }

    instance->data.get = update->get;
    instance->data.set = update->set;
    instance->data.derivative = update->gyro;
    instance->data.error_delta = update->error_delta;
}

/**
 * @brief 计算并返回指定云台PID实例的输出值
 * @param {const gimbal_PID_t*} instance: 指向要计算输出值的云台PID实例的指针
 * @return {float} 返回计算得到的PID输出值，如果输入实例为NULL，则返回0
 */
float GimbalPidLoopOUT(const gimbal_PID_t* instance)
{
    if (instance == NULL)
    {
        return 0.0f;
    }
    return instance->data.out;
}

/**
 * @brief 释放并重置云台PID实例的数据
 * @param {gimbal_PID_t*} instance: 需要被释放和重置的云台PID实例指针
 * @note 如果传入的实例指针为NULL，则函数直接返回，不做任何操作。
 *       该函数将重置PID实例中的所有相关数据字段至0.0f。
 */
void GimbalPidLoopFree(gimbal_PID_t* instance)
{
    if (instance == NULL)
    {
        return;
    }

    instance->data.err = 0.0f;
    instance->data.set = 0.0f;
    instance->data.get = 0.0f;
    instance->data.out = 0.0f;
    instance->data.Pout = 0.0f;
    instance->data.Iout = 0.0f;
    instance->data.Dout = 0.0f;
    instance->data.derivative = 0.0f;
    instance->data.derivative_lpf = 0.0f;
    instance->data.error_delta = 0.0f;
    instance->data.time_tick = 0;
}

/**
 * @brief 计算给定PID控制器的输出值
 * @param {gimbal_PID_t*} pid: 指向要计算的PID控制器结构体的指针
 * @return {float} 返回计算后的PID控制器输出值
 * @note 如果输入的PID控制器指针为NULL，则返回0.0f
 */
static float gimbal_PID_calc(gimbal_PID_t* pid)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    const float err = pid->data.set - pid->data.get;
    pid->data.err = loop_float_constrain_fast(err, -pi, pi);

    pid->data.Pout = pid->config.kp * pid->data.err;

    /* 死区积分，避免在接近目标值时积分饱和 */
    if (fast_fabsf_ptr(err) > 0.01f)
    {
        pid->data.Iout += pid->config.ki * pid->data.err;
    }

    pid->data.Dout = pid->config.kd * pid->data.error_delta;
    abs_limit_v2(&pid->data.Iout, pid->config.max_iout);

    /* 微分项（带滤波） */
    pid->data.derivative_lpf = 0.2f * pid->data.derivative_lpf + 0.8f * pid->data.derivative;

    pid->data.out = pid->data.Pout + pid->data.Iout + pid->data.Dout + pid->data.derivative_lpf * 0.05f;
    abs_limit_v2(&pid->data.out, pid->config.max_out);

    return pid->data.out;
}

/**
 * @brief 根据名称获取云台PID实例
 * @param name 云台PID实例的名称
 * @return 返回与给定名称匹配的gimbal_PID_t指针，如果未找到则返回NULL
 */
gimbal_PID_t* get_gimbal_pid_instance(const char* name)
{
    if (name == NULL)
    {
        return NULL;
    }

    gimbal_PID_t* current = g_gimbal_pid_list;
    while (current != NULL)
    {
        if (__strcmp(current->config.name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief 获取当前注册的云台PID实例数量
 * @return {uint32_t} PID实例数量
 */
uint32_t get_gimbal_pid_count(void)
{
    uint32_t count = 0;
    gimbal_PID_t* current = g_gimbal_pid_list;

    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

/**
 * @brief: 快速环绕约束函数
 * @param {float} Input: 输入值
 * @param {float} minValue: 最小值
 * @param {float} maxValue: 最大值
 * @return {float} 输出值
 * @note: add your note!
 */
static float loop_float_constrain_fast(float Input, const float minValue, const float maxValue)
{
    if (maxValue <= minValue)
    {
        return Input;
    }

    const float L = maxValue - minValue;

    /* 快速处理常见情况 */
    if (Input >= minValue && Input <= maxValue)
    {
        return Input;
    }

    /* 使用整数除法提高速度（但可能损失一些精度） */
    const int32_t n = (int32_t)((Input - minValue) / L);
    float result = Input - n * L;

    /* 确保结果在范围内 */
    if (result > maxValue)
    {
        result -= L;
    }
    else if (result < minValue)
    {
        result += L;
    }
    return result;
}

/**
 * @brief: 快速绝对值函数
 * @param {float} x: 输入值
 * @return {float} 输出值
 * @note: add your note!
 */
static float fast_fabsf_ptr(float x)
{
    uint32_t* ptr = (uint32_t*)&x;
    *ptr &= 0x7FFFFFFF; /* 清除符号位 */
    return x;
}

typedef union
{
    float f;
    uint32_t u;
} float_uint_t;

/**
 * @brief: 绝对值限制函数
 * @param {float*} num: 输入输出值指针
 * @param {float} Limit: 限制值
 * @return {void}
 * @note: add your note!
 */
static void abs_limit_v2(float* num, const float Limit)
{
    if (num == NULL)
    {
        return;
    }

    float_uint_t fu;
    fu.f = *num;

    /* 提取符号位 (0 或 0x80000000) */
    const uint32_t sign = fu.u & 0x80000000;

    /* 清除符号位得到绝对值 */
    fu.u &= 0x7FFFFFFF;

    /* 如果绝对值超过限制，则应用限制 */
    if (fu.f > Limit)
    {
        /* 将符号位于限制值组合 */
        float_uint_t limit_fu;
        limit_fu.f = Limit;
        limit_fu.u |= sign;
        *num = limit_fu.f;
    }
}

// end of file!
