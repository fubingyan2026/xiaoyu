/**
 * @file    fsm_example.c
 * @brief   通用FSM框架使用示例
 * @author  FSM Framework Team
 * @date    2026-02-03
 *
 * @description
 * 本示例演示了如何使用通用FSM框架
 * 实现一个简单的交通灯控制器。
 */

#include "fsm.h"
#include <stdint.h>
#include <stdio.h>
#include <locale.h>

/* Windows 控制台中文显示支持 */
#ifdef _WIN32
#include <windows.h>
#endif

/*============================================================================
 * 交通灯状态机示例
 *============================================================================*/

/* 定义状态 */
typedef enum
{
    TRAFFIC_STATE_RED = 0,
    TRAFFIC_STATE_YELLOW,
    TRAFFIC_STATE_GREEN,
    TRAFFIC_STATE_COUNT
} traffic_state_e;

/* 用户上下文数据 */
typedef struct
{
    uint32_t timer;           /* 定时器计数器 */
    uint32_t red_duration;    /* 红灯持续时间 */
    uint32_t yellow_duration; /* 黄灯持续时间 */
    uint32_t green_duration;  /* 绿灯持续时间 */
    bool emergency_stop;      /* 紧急停止标志 */
} traffic_context_t;

/* 用于调试的状态名称 */
static const char *traffic_state_names[] = {"红灯", "黄灯", "绿灯"};

/*============================================================================
 * 状态处理器函数
 *============================================================================*/

/**
 * @brief 红灯状态处理器
 */
static fsm_state_t handler_red(fsm_context_t *ctx)
{
    traffic_context_t *traffic = (traffic_context_t *)fsm_get_user_data(ctx);

    if (traffic->emergency_stop)
    {
        return TRAFFIC_STATE_RED; /* 保持红灯 */
    }

    traffic->timer++;

    if (traffic->timer >= traffic->red_duration)
    {
        traffic->timer = 0;
        return TRAFFIC_STATE_GREEN; /* 切换到绿灯 */
    }

    return TRAFFIC_STATE_RED; /* 保持红灯 */
}

/**
 * @brief 黄灯状态处理器
 */
static fsm_state_t handler_yellow(fsm_context_t *ctx)
{
    traffic_context_t *traffic = (traffic_context_t *)fsm_get_user_data(ctx);

    traffic->timer++;

    if (traffic->timer >= traffic->yellow_duration)
    {
        traffic->timer = 0;
        return TRAFFIC_STATE_GREEN; /* 切换到红灯 */
    }

    return TRAFFIC_STATE_YELLOW; /* 保持黄灯 */
}

/**
 * @brief 绿灯状态处理器
 */
static fsm_state_t handler_green(fsm_context_t *ctx)
{
    traffic_context_t *traffic = (traffic_context_t *)fsm_get_user_data(ctx);

    if (traffic->emergency_stop)
    {
        return TRAFFIC_STATE_YELLOW; /* 紧急情况：切换到黄灯然后红灯 */
    }

    traffic->timer++;

    if (traffic->timer >= traffic->green_duration)
    {
        traffic->timer = 0;
        return TRAFFIC_STATE_YELLOW; /* 切换到黄灯 */
    }

    return TRAFFIC_STATE_GREEN; /* 保持绿灯 */
}

/*============================================================================
 * 回调函数
 *============================================================================*/

/**
 * @brief 状态进入回调
 */
static void on_state_entry(fsm_context_t *ctx, fsm_state_t state)
{
    printf(">>> 进入状态: %s\n", fsm_get_state_name(ctx, state));

    /* 执行状态特定的初始化 */
    switch (state)
    {
    case TRAFFIC_STATE_RED:
        printf("    红灯亮\n");
        break;
    case TRAFFIC_STATE_YELLOW:
        printf("    黄灯亮\n");
        break;
    case TRAFFIC_STATE_GREEN:
        printf("    绿灯亮\n");
        break;
    default:
        break;
    }
}

/**
 * @brief 状态退出回调
 */
static void on_state_exit(fsm_context_t *ctx, fsm_state_t state)
{
    printf("<<< 退出状态: %s\n", fsm_get_state_name(ctx, state));
    /* 执行状态特定的初始化 */
    switch (state)
    {
    case TRAFFIC_STATE_RED:
        printf("    红灯灭\n");
        break;
    case TRAFFIC_STATE_YELLOW:
        printf("    黄灯灭\n");
        break;
    case TRAFFIC_STATE_GREEN:
        printf("    绿灯灭\n");
        break;
    default:
        break;
    }
}

/*============================================================================
 * 主示例函数
 *============================================================================*/

int main(void)
{
    fsm_context_t fsm;
    traffic_context_t traffic_data = {.timer = 0,
                                      .red_duration = 5,    /* 5步 */
                                      .yellow_duration = 2, /* 2步 */
                                      .green_duration = 5,  /* 5步 */
                                      .emergency_stop = false};

#ifdef _WIN32
    /* Windows 控制台设置为 UTF-8 代码页 */
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    setlocale(LC_ALL, "");

    printf("=== 通用FSM框架 - 交通灯示例 ===\n\n");

    /* 初始化FSM */
    if (fsm_init(&fsm, TRAFFIC_STATE_RED, &traffic_data) != FSM_OK)
    {
        printf("错误: 初始化FSM失败\n");
        return -1;
    }

    /* 设置调试状态名称 */
    fsm_set_state_names(&fsm, traffic_state_names, TRAFFIC_STATE_COUNT);

    /* 设置回调 */
    fsm_set_callbacks(&fsm, on_state_entry, on_state_exit);

    /* 注册状态处理器 */
    fsm_register_handler(&fsm, TRAFFIC_STATE_RED, handler_red);
    fsm_register_handler(&fsm, TRAFFIC_STATE_YELLOW, handler_yellow);
    fsm_register_handler(&fsm, TRAFFIC_STATE_GREEN, handler_green);

    /* 定义有效转换 */
    fsm_add_transition(&fsm, TRAFFIC_STATE_RED, TRAFFIC_STATE_GREEN, NULL);
    fsm_add_transition(&fsm, TRAFFIC_STATE_GREEN, TRAFFIC_STATE_YELLOW, NULL);
    fsm_add_transition(&fsm, TRAFFIC_STATE_YELLOW, TRAFFIC_STATE_RED, NULL);
    fsm_add_transition(&fsm, TRAFFIC_STATE_YELLOW, TRAFFIC_STATE_GREEN, NULL);

    /* 运行状态机多个周期 */
    printf("开始交通灯序列...\n");
    printf("初始化状态：%s\n", fsm_get_state_name(&fsm, fsm_get_current_state(&fsm)));

    for (int step = 0; step < 30; step++)
    {

        // /* 在第15步触发紧急停止 */
        // if (step == 15)
        // {
        //     printf("\n!!! 紧急停止触发 !!!\n\n");
        //     traffic_data.emergency_stop = true;
        // }

        // /* 在第20步清除紧急状态 */
        // if (step == 20)
        // {
        //     printf("\n*** 紧急状态已清除 ***\n\n");
        //     traffic_data.emergency_stop = false;
        // }

        fsm_ret_t ret = fsm_step(&fsm);
        if (ret != FSM_OK)
        {
            printf("错误: FSM步骤失败,错误码%d\n", ret);
            break;
        }
    }

    printf("=== 示例完成 ===\n");
    return 0;
}
