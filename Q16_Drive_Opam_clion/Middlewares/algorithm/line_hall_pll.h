
#ifndef LINE_HALL_PLL_H
#define LINE_HALL_PLL_H

#include "stdint.h"
#include "filter.h"
/* 状态变量 */
typedef struct
{
    float theta; /* 估计角度 [rad] */
    float omega; /* 估计角速度 [rad/s] */
    float amp;   /* 估计幅值 */
    float off_a; /* 通道 alpha 偏置 */
    float off_b; /* 通道 beta  偏置 */
    /* PI 积分器 */
    float intg_pll;
    float intg_amp;
    float intg_off_a;
    float intg_off_b;
} pll_t;

typedef struct
{
    pll_t pll;
    pt1Filter_t lp_filter_ud_a;
    pt1Filter_t lp_filter_uq_a;
    pt1Filter_t lp_filter_ud_b;
    pt1Filter_t lp_filter_uq_b;
    pt1Filter_t lp_filter_omega;

} hall_pll_handle_t;

extern hall_pll_handle_t hall_pll;

void hall_pll_init(hall_pll_handle_t *hall);

void hall_pll_update(hall_pll_handle_t *hall, float adc_a, float adc_b);

float hall_pll_get_angle_rad(const hall_pll_handle_t *hall);

float hall_pll_get_speed_rads(const hall_pll_handle_t *hall);

#endif // LINE_HALL_PLL_H