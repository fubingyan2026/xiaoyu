/**
 * @file    fsm.c
 * @brief   通用有限状态机框架 - 实现
 * @author  FSM Framework Team
 * @date    2026-02-03
 * @version 1.0.0
 */

#include "fsm.h"
#include <string.h>

/*============================================================================
 * 私有辅助函数
 *============================================================================*/

/**
 * @brief 检查状态转换是否有效（邻接矩阵O(1)查询）
 * @param ctx 状态机上下文指针
 * @param target 目标状态
 * @return 转换有效返回true，否则返回false
 */
static bool fsm_is_valid_transition(const fsm_context_t *ctx, fsm_state_t target)
{
    if (!ctx || ctx->current_state >= FSM_MAX_STATES || target >= FSM_MAX_STATES)
    {
        return false;
    }

    /* 允许保持当前状态 */
    if (ctx->current_state == target)
    {
        return true;
    }

    /* O(1)查询：检查邻接矩阵 */
    fsm_condition_t cond = ctx->transitions[ctx->current_state][target];

    if (cond != NULL)
    {
        /* 存在转换规则，检查条件（指向fsm_always_true表示无条件） */
        return cond(ctx);
    }

    /* NULL表示转换不存在 */
    return false;
}

/*============================================================================
 * 公共API实现
 *============================================================================*/

fsm_ret_t fsm_init(fsm_context_t *ctx, fsm_state_t initial_state, void *user_data)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    /* 清空整个上下文 */
    memset(ctx, 0, sizeof(fsm_context_t));

    /* 初始化状态 */
    ctx->current_state = initial_state;
    ctx->last_state = initial_state;
    ctx->state_changed = true;
    ctx->user_data = user_data;

    /* 默认不改变 last_state 以确保第一次步进能触发 entry 回调 */
    ctx->last_state = (fsm_state_t)0xFF; 
    
    return FSM_OK;
}

fsm_ret_t fsm_register_handler(fsm_context_t *ctx, fsm_state_t state, fsm_handler_t handler)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    if (state >= FSM_MAX_STATES)
    {
        return FSM_ERROR_INVALID_STATE;
    }

    if (!handler)
    {
        return FSM_ERROR_NULL_PTR;
    }

    ctx->handlers[state] = handler;
    return FSM_OK;
}

fsm_ret_t fsm_add_transition(fsm_context_t *ctx, fsm_state_t from_state, fsm_state_t to_state,
                             fsm_condition_t condition)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    if (from_state >= FSM_MAX_STATES || to_state >= FSM_MAX_STATES)
    {
        return FSM_ERROR_INVALID_STATE;
    }

    /* 在邻接矩阵中存储条件
     * NULL condition 转换为 fsm_always_true（无条件转换）
     * 不存在的转换保持为 NULL
     */
    if (condition == NULL)
    {
        condition = fsm_always_true;
    }
    ctx->transitions[from_state][to_state] = condition;
    return FSM_OK;
}

#if FSM_ENABLE_CALLBACKS
fsm_ret_t fsm_set_callbacks(fsm_context_t *ctx, fsm_on_entry_t on_entry, fsm_on_exit_t on_exit)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    ctx->on_entry = on_entry;
    ctx->on_exit = on_exit;
    return FSM_OK;
}
#endif

#if FSM_ENABLE_DEBUG
fsm_ret_t fsm_set_state_names(fsm_context_t *ctx, const char **state_names, uint8_t state_count)
{
    if (!ctx || !state_names)
    {
        return FSM_ERROR_NULL_PTR;
    }

    ctx->state_names = state_names;
    ctx->state_count = state_count;
    return FSM_OK;
}

const char *fsm_get_state_name(const fsm_context_t *ctx, fsm_state_t state)
{
    if (ctx && ctx->state_names && state < ctx->state_count)
    {
        return ctx->state_names[state];
    }
    return "UNKNOWN";
}
#endif

fsm_ret_t fsm_step(fsm_context_t *ctx)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    fsm_state_t current_state = ctx->current_state;

    /* 验证状态有效性 */
    if (current_state >= FSM_MAX_STATES)
    {
        return FSM_ERROR_INVALID_STATE;
    }

    /* 在转换后的第一步处理状态进入 */
    if (ctx->state_changed)
    {
#if FSM_ENABLE_CALLBACKS
        /* 只有真正发生状态变化时才触发回调 */
        if (ctx->last_state != current_state)
        {
            if (ctx->on_exit)
                ctx->on_exit(ctx, ctx->last_state);
            if (ctx->on_entry)
                ctx->on_entry(ctx, current_state);
        }
#endif
        ctx->last_state = current_state;
        ctx->state_changed = false;
    }

    /* 执行当前状态处理器 */
    if (!ctx->handlers[current_state])
    {
        return FSM_ERROR_NO_HANDLER;
    }

    fsm_state_t next_state = ctx->handlers[current_state](ctx);

    /* 检查状态是否改变 */
    if (next_state != current_state)
    {
        /* 验证转换 */
        if (fsm_is_valid_transition(ctx, next_state))
        {
            ctx->current_state = next_state;
            ctx->state_changed = true;
            return FSM_OK;
        }
        else
        {
            return FSM_ERROR_INVALID_TRANSITION;
        }
    }

    return FSM_OK;
}

fsm_ret_t fsm_request_transition(fsm_context_t *ctx, fsm_state_t target_state)
{
    if (!ctx)
    {
        return FSM_ERROR_NULL_PTR;
    }

    if (target_state >= FSM_MAX_STATES)
    {
        return FSM_ERROR_INVALID_STATE;
    }

    /* 验证转换 */
    if (fsm_is_valid_transition(ctx, target_state))
    {
        ctx->current_state = target_state;
        ctx->state_changed = true;
        return FSM_OK;
    }

    return FSM_ERROR_INVALID_TRANSITION;
}
