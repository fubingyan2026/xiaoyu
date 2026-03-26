#ifndef GIMBAL_CONTROL_H
#define GIMBAL_CONTROL_H

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float kd;
    float max_out;
    float max_iout;
    unsigned short control_cycle;
    const char *name;
} gimbal_pid_config_t;

typedef struct
{
    float get;
    float set;
    float error_delta;
    float gyro;
} gimbal_pid_update_t;

typedef struct gimbal_PID
{
    gimbal_pid_config_t config;
    struct
    {
        float set;
        float get;
        float err;
        float gyro;
        float derivative;
        float derivative_lpf;
        float error_delta;

        float Pout;
        float Iout;
        float Dout;

        float out;
        uint32_t time_tick;
    } data;

    struct gimbal_PID *next;
} gimbal_PID_t;

/* 全局链表头指针 */
extern gimbal_PID_t *g_gimbal_pid_list;

gimbal_PID_t *GimbalPidRegister(const gimbal_pid_config_t *config);
void GimbalPidUnregister(gimbal_PID_t *instance);
void GimbalPidUnregisterAll(void);

void GimbalPidUpdateFeedBack(gimbal_PID_t *instance, const gimbal_pid_update_t *update);
void GimbalPidLoop(void);
float GimbalPidLoopOUT(const gimbal_PID_t *instance);
void GimbalPidLoopFree(gimbal_PID_t *instance);

gimbal_PID_t *get_gimbal_pid_instance(const char *name);
uint32_t get_gimbal_pid_count(void);

#endif
