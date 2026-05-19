//
// Created by fubingyan on 25-8-2.
//

#include "hall_adjustment.h"

#include "adc.h"
#include "easyflash.h"
#include "flash_task.h"
#include "foc_config_q16.h"
#include "pll.h"

pt1Filter_t hall_filter[ADC_CH_NUM];
pt1Filter_t hall_filter_normal[ADC_CH_NUM];

float adc_max_value[ADC_CH_NUM];
float adc_min_value[ADC_CH_NUM];
const uint16_t ADJUST_FLAG = 0xEEEE;
const float motor_adjust_velocity = 0.025f;
const uint16_t motor_align_time = 250;
const float rotation_circle = 2.5f;
const uint16_t hall_filter_fcut_Normal = 10000;
const uint16_t hall_filter_fcut_Adjust = 20;

pll_context_t pll_ctx;

const pll_config_t pll_config = {
    .kp = 250.0f,
    .ki = 32000.0f,
    .sample_time = FOC_PWM_PERIOD,
    .filter_freq_dq = 500.0f,
    .filter_freq_omega = 20.0f,
};

static daemon_context_t* daemon_get_encoder;
#define HALL_ADJUST_PRINTF(...)  // BSP_Printf(__VA_ARGS__)

static void HALL_Adjust_Init(void);

static void HALL_Adjust_Task(void);

static void hall_filter_init(pt1Filter_t* filter, uint16_t f_cut, float dt);

static void hall_ADC_DMA_Start(void);

static void HALL_Adjust_EXE_Flow(hall_adjust_t* _hall_adjust);

static uint16_t HALL_Adjust_Get_Angle(void);

hall_adjust_t hall_adjust = {
    .motor_poles = MOTOR_POLES,
    .motorCurrentMax = ALIGN_CURRENT,
    .hall_adjust_init = HALL_Adjust_Init,
    .hall_adc_DMA_Start = hall_ADC_DMA_Start,
    .hall_adjust_task = HALL_Adjust_Task,
    .hall_adjust_get_angle = HALL_Adjust_Get_Angle,
    .motorTargetElectricalAngle = 0,
};

hall_save_param_t hall_save_param;

static void HALL_Adjust_EXE_Flow(hall_adjust_t* _hall_adjust) {
  static uint16_t filter_init_times = 0;
  for (uint8_t i = 0; i < 2; i++) {
    pt1FilterApply(&hall_filter[i], (float)_hall_adjust->adcOriginalValue[i]);
  }

  switch (_hall_adjust->hall_adjust_state) {
    case ADJUST_NONE:

      for (uint8_t i = 0; i < 2; i++) {
        adc_max_value[i] = 0;
        adc_min_value[i] = 0x1000;
      }
      HALL_ADJUST_PRINTF("STATE:ADJUST_FILTER\r\n");

      _hall_adjust->hall_adjust_state = ADJUST_FILTER;
      break;
    case ADJUST_FILTER:

      if (filter_init_times++ >= 500) {
        filter_init_times = 0;
        _hall_adjust->hall_adjust_state = ADJUST_ALIGN;
        HALL_ADJUST_PRINTF("STATE:ADJUST_ALIGN\r\n");
      }

      break;
    case ADJUST_ALIGN:

      if (_hall_adjust->motorTargetCurrent < _hall_adjust->motorCurrentMax) {
        _hall_adjust->motorTargetCurrent += 0.05f;
      } else {
        static uint16_t align_times = 0;
        if (align_times < motor_align_time) {
          align_times++;
        } else {
          align_times = 0;
          _hall_adjust->hall_adjust_state = ADJUST_ROTATION;
          HALL_ADJUST_PRINTF("STATE:ADJUST_ROTATION\r\n");
        }
      }

      break;
    case ADJUST_ROTATION:

      for (uint8_t i = 0; i < 2; i++) {
        if (adc_max_value[i] < hall_filter[i].state) {
          adc_max_value[i] = hall_filter[i].state;
        }
        if (adc_min_value[i] > hall_filter[i].state) {
          adc_min_value[i] = hall_filter[i].state;
        }
      }
      if (_hall_adjust->motorTargetElectricalAngle <
          (float)hall_adjust.motor_poles * M_2PI * rotation_circle * 1.25f) {
        _hall_adjust->motorTargetElectricalAngle += motor_adjust_velocity;
      } else {
        _hall_adjust->motorTargetElectricalAngle = 0;
        _hall_adjust->motorTargetCurrent = 0;
        _hall_adjust->hall_adjust_state = ADJUST_PROCESS;
        HALL_ADJUST_PRINTF("STATE:ADJUST_PROCESS\r\n");
      }

      break;
    case ADJUST_PROCESS:

      hall_save_param.hall_adjust_flag = ADJUST_FLAG;

      for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
        hall_save_param.adcAmplitudeBias[i] =
            (int16_t)((adc_max_value[i] + adc_min_value[i]) * 0.5f);
        hall_save_param.adcAmplitudeMax[i] =
            (int16_t)((adc_max_value[i] - adc_min_value[i]) * 0.5f);
      }

      flash_task_request(FLASH_TASK_WRITE_HALL, &hall_save_param,
                         sizeof(hall_save_param));

      _hall_adjust->hall_adjust_state = ADJUST_DONE;
      HALL_ADJUST_PRINTF("STATE:ADJUST_DONE\r\n");

      break;
    case ADJUST_DONE:
      _hall_adjust->hall_adjust_state = ADJUST_NONE;
      HALL_ADJUST_PRINTF("STATE:ADJUST_NONE\r\n");

      break;
    default:
      break;
  }
}

static void HALL_Adjust_Init(void) {
  for (uint8_t i = 0; i < 2; i++) {
    hall_filter_init(&hall_filter_normal[i], hall_filter_fcut_Normal,
                     FOC_PWM_PERIOD);
  }
  for (uint8_t i = 0; i < 2; i++) {
    hall_filter_init(&hall_filter[i], hall_filter_fcut_Adjust, 0.001f);
  }

  ef_get_env_blob(FLASH_MAGIC_HALL, &hall_save_param, sizeof(hall_save_param),
                  NULL);

  pll_init(&pll_ctx, &pll_config);
  daemon_get_encoder = daemon_get_instance("encoder");
  DEBUG_ASSERT(daemon_get_encoder);
}

static void hall_filter_init(pt1Filter_t* filter, const uint16_t f_cut,
                             const float dt) {
  pt1FilterInit(filter, pt1FilterGain(f_cut, dt));
}

static void HALL_Adjust_Task(void) { HALL_Adjust_EXE_Flow(&hall_adjust); }

float angle = 0;

static uint16_t HALL_Adjust_Get_Angle(void) {
  for (uint8_t i = 0; i < ADC_CH_NUM; i++) {
    pt1FilterApply(&hall_filter_normal[i],
                   (float)hall_adjust.adcOriginalValue[i]);
  }

  float _y = (float)((int32_t)hall_filter_normal[1].state -
                     hall_save_param.adcAmplitudeBias[1]) /
             (float)hall_save_param.adcAmplitudeMax[1];
  float _x = (float)((int32_t)hall_filter_normal[0].state -
                     hall_save_param.adcAmplitudeBias[0]) /
             (float)hall_save_param.adcAmplitudeMax[0];

  if (hall_save_param.hall_adjust_flag != ADJUST_FLAG) {
    _y = _x = 0.0001f;
  }
  pll_update(&pll_ctx, _y, _x);
  angle = pll_get_angle(&pll_ctx);
  angle = atan2_approx(_y, _x);
  hall_adjust.angle = angle;
  UTILS_NAN_ZERO(angle);
  utils_norm_angle_rad(&angle);
  daemon_reload(daemon_get_encoder);
  return (uint16_t)((angle + M_PI) / (M_PI * 2) * 16384);
}

static void hall_ADC_DMA_Start(void) {
  HAL_ADC_Start_DMA(&hadc2, hall_adjust.adcOriginalValue, 2);
}
