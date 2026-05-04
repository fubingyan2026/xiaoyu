/**
 * @file    fsm.h
 * @brief   通用有限状态机框架 - 头文件
 * @author  FSM Framework Team
 * @date    2026-02-03
 * @version 1.0.0
 *
 * @description
 * 从FOC控制系统提取的轻量级可复用有限状态机框架。
 * 功能特性:
 * - 状态进入/退出回调
 * - 状态转换验证
 * - 用户自定义状态处理器
 * - 上下文数据支持
 * - 调试状态名称映射
 */

#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================
 * 配置宏定义
 *============================================================================*/
#ifndef FSM_MAX_STATES
#define FSM_MAX_STATES (8U) ///< 最大状态数量
#endif

#ifndef FSM_ENABLE_DEBUG
#define FSM_ENABLE_DEBUG (1) ///< 启用调试功能（状态名称、日志）
#endif

#ifndef FSM_ENABLE_CALLBACKS
#define FSM_ENABLE_CALLBACKS (1) ///< 启用进入/退出回调
#endif

    /*============================================================================
     * 类型定义
     *============================================================================*/
    /**
     * @brief 状态标识符类型
     * 用户可以将其typedef为自己的枚举类型
     */
    typedef uint8_t fsm_state_t;

    /**
     * @brief 返回码枚举
     */
    typedef enum __attribute__((packed))
    {
        FSM_OK = 0,                   ///< 操作成功
        FSM_ERROR,                    ///< 通用错误
        FSM_ERROR_NULL_PTR,           ///< 空指针错误
        FSM_ERROR_INVALID_STATE,      ///< 无效状态ID
        FSM_ERROR_INVALID_TRANSITION, ///< 无效状态转换
        FSM_ERROR_NO_HANDLER,         ///< 状态没有注册处理器
        FSM_ERROR_FULL                ///< 转换表已满
    } fsm_ret_t;

    /* 前向声明 */
    typedef struct fsm_context_s fsm_context_t;

    /**
     * @brief 状态处理器函数类型
     * @param ctx 状态机上下文指针
     * @return 要转换到的下一个状态（返回当前状态以保持）
     */
    typedef fsm_state_t (*fsm_handler_t)(fsm_context_t *ctx);

#if FSM_ENABLE_CALLBACKS
    /**
     * @brief 状态进入回调函数类型
     * @param ctx 状态机上下文指针
     * @param state 正在进入的状态
     */
    typedef void (*fsm_on_entry_t)(fsm_context_t *ctx, fsm_state_t state);

    /**
     * @brief 状态退出回调函数类型
     * @param ctx 状态机上下文指针
     * @param state 正在退出的状态
     */
    typedef void (*fsm_on_exit_t)(fsm_context_t *ctx, fsm_state_t state);
#endif

    /**
     * @brief 转换条件函数类型
     * @param ctx 状态机上下文指针
     * @return 转换允许返回true，否则返回false
     */
    typedef bool (*fsm_condition_t)(const fsm_context_t *ctx);

    /**     * @brief 特殊标记：表示存在无条件转换
     * 使用指向此函数的指针来区分"无转换"和"无条件转换"
     */
    static inline bool fsm_always_true(const fsm_context_t *ctx)
    {
        (void)ctx;
        return true;
    }

    /**     * @brief 状态转换规则结构
     */
    typedef struct
    {
        fsm_state_t from_state;    ///< 源状态
        fsm_state_t to_state;      ///< 目标状态
        fsm_condition_t condition; ///< 可选条件（NULL = 始终允许）
    } fsm_transition_t;

    /**
     * @brief 状态机上下文结构
     */
    struct fsm_context_s
    {
        fsm_state_t current_state; ///< 当前状态
        fsm_state_t last_state;    ///< 前一个状态
        bool state_changed;        ///< 状态改变标志

        /* 状态处理器 */
        fsm_handler_t handlers[FSM_MAX_STATES]; ///< 状态处理器函数表

        /* 转换规则（邻接矩阵：O(1)查询） */
        fsm_condition_t transitions[FSM_MAX_STATES][FSM_MAX_STATES]; ///< 转换条件矩阵

#if FSM_ENABLE_CALLBACKS
        /* 全局回调（所有状态变化时调用） */
        fsm_on_entry_t on_entry; ///< 全局进入回调
        fsm_on_exit_t on_exit;   ///< 全局退出回调
#endif

#if FSM_ENABLE_DEBUG
        /* 调试信息 */
        const char **state_names; ///< 状态名称查找表
        uint8_t state_count;      ///< 状态数量
#endif

        /* 用户数据 */
        void *user_data; ///< 用户定义的上下文数据
    };

    /*============================================================================
     * 公共API函数
     *============================================================================*/

    /**
     * @brief 初始化状态机上下文
     * @param ctx 状态机上下文指针
     * @param initial_state 初始状态
     * @param user_data 可选的用户数据指针
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_init(fsm_context_t *ctx, fsm_state_t initial_state, void *user_data);

    /**
     * @brief 注册状态处理器
     * @param ctx 状态机上下文指针
     * @param state 状态ID
     * @param handler 处理器函数
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_register_handler(fsm_context_t *ctx, fsm_state_t state, fsm_handler_t handler);

    /**
     * @brief 添加状态转换规则
     * @param ctx 状态机上下文指针
     * @param from_state 源状态
     * @param to_state 目标状态
     * @param condition 可选条件函数（NULL = 始终允许）
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_add_transition(fsm_context_t *ctx, fsm_state_t from_state, fsm_state_t to_state,
                                 fsm_condition_t condition);

#if FSM_ENABLE_CALLBACKS
    /**
     * @brief 设置全局进入/退出回调
     * @param ctx 状态机上下文指针
     * @param on_entry 进入回调（NULL表示禁用）
     * @param on_exit 退出回调（NULL表示禁用）
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_set_callbacks(fsm_context_t *ctx, fsm_on_entry_t on_entry, fsm_on_exit_t on_exit);
#endif

#if FSM_ENABLE_DEBUG
    /**
     * @brief 设置调试用状态名称查找表
     * @param ctx 状态机上下文指针
     * @param state_names 状态名称字符串数组
     * @param state_count 状态数量
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_set_state_names(fsm_context_t *ctx, const char **state_names, uint8_t state_count);

    /**
     * @brief 获取状态名称字符串
     * @param ctx 状态机上下文指针
     * @param state 状态ID
     * @return 状态名称字符串或"UNKNOWN"
     */
    const char *fsm_get_state_name(const fsm_context_t *ctx, fsm_state_t state);
#endif

    /**
     * @brief 执行状态机的一步
     * 调用当前状态处理器并处理转换
     * @param ctx 状态机上下文指针
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_step(fsm_context_t *ctx);

    /**
     * @brief 请求状态转换
     * @param ctx 状态机上下文指针
     * @param target_state 目标状态
     * @return 成功返回FSM_OK，否则返回错误码
     */
    fsm_ret_t fsm_request_transition(fsm_context_t *ctx, fsm_state_t target_state);

    /**
     * @brief 获取当前状态
     * @param ctx 状态机上下文指针
     * @return 当前状态ID
     */
    static inline fsm_state_t fsm_get_current_state(const fsm_context_t *ctx)
    {
        return ctx ? ctx->current_state : 0;
    }

    /**
     * @brief 获取前一个状态
     * @param ctx 状态机上下文指针
     * @return 前一个状态ID
     */
    static inline fsm_state_t fsm_get_last_state(const fsm_context_t *ctx)
    {
        return ctx ? ctx->last_state : 0;
    }

    /**
     * @brief 检查状态在最后一步是否改变
     * @param ctx 状态机上下文指针
     * @return 状态改变返回true，否则返回false
     */
    static inline bool fsm_is_state_changed(const fsm_context_t *ctx)
    {
        return ctx ? ctx->state_changed : false;
    }

    /**
     * @brief 获取用户数据指针
     * @param ctx 状态机上下文指针
     * @return 用户数据指针
     */
    static inline void *fsm_get_user_data(const fsm_context_t *ctx)
    {
        return ctx ? ctx->user_data : NULL;
    }

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */
