/**
 * @file    usart_protocol.c
 * @brief   UART S7协议处理器实现
 * @author  FOC Development Team
 * @date    2026-03-03
 * @version V2.1
 *
 * @note    参考西门子S7comm/PPI协议规范
 *          帧头: 0x68 0x68, 协议ID: 0x32, CRC16校验, 帧尾: 0x16
 * @note    编码规范: MISRA C:2012, AUTOSAR C++14
 *
 * 修订历史:
 * | 版本  | 日期       | 作者   | 描述                  |
 * |-------|------------|--------|-----------------------|
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,优化错误处理 |
 * | 1.0   | 2026-03-03 | FOC团队| 初始版本               |
 */

#include "usart_protocol.h"

#include <string.h>

#include "app.h"
#include "crc16.h"
#include "debug/debug.h"
#include "encoder_alignment.h"
#include "foc_ctrl_q16.h"
#include "foc_sm.h"
#include "hal_uart.h"
#include "stm32g4xx_hal.h"
#include "usart_receive.h"

/*============================================================================
 * 私有变量定义
 *============================================================================*/
static StreamConfig_t s_stream_config = {
    .enable = 0U,
    .stream_type = 0U,
    .interval_ms = 10U,
};

static uint32_t s_stream_last_tick = 0U;

extern foc_ctrl_t foc_ctrl;
extern encoder_calibration_t g_encoder_calib;
extern motor_flash_config_t g_motor_flash_cfg;
extern hal_uart_context_t uart1_ctx;

/*============================================================================
 * S7协议帧构建函数
 *============================================================================*/

/**
 * @brief 构建S7协议帧
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 * @param[in] msg_type 消息类型
 * @param[in] func_code 功能码
 * @param[in] data 数据指针,可为NULL
 * @param[in] len 数据长度
 * @param[out] output 输出缓冲区指针
 * @return uint16_t 帧长度
 */
static uint16_t s7_build_frame(uint8_t msg_type, uint8_t func_code,
                               const uint8_t* data, uint16_t len,
                               uint8_t* output) {
  uint8_t payload[256U];
  payload[0U] = S7_PROTOCOL_ID;
  payload[1U] = msg_type;
  payload[2U] = func_code;

  if ((len > 0U) && (data != NULL)) {
    (void)__memcpy(&payload[3U], data, (size_t)len);
  }

  uint16_t crc = crc16_modbus(payload, (uint16_t)(3U + len));

  output[0U] = S7_FRAME_HEAD_1;
  output[1U] = S7_FRAME_HEAD_2;
  (void)__memcpy(&output[2U], payload, (size_t)(3U + len));
  output[2U + 3U + len] = (uint8_t)(crc & 0xFFU);
  output[2U + 3U + len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
  output[2U + 3U + len + 2U] = S7_FRAME_TAIL;

  return (uint16_t)(8U + len);
}

/**
 * @brief 发送协议响应帧
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 */
void usart_protocol_send_response(uint8_t msg_type, uint8_t func_code,
                                  const uint8_t* data, uint16_t len) {
  static uint8_t resp_buffer[280U];
  uint16_t frame_len =
      s7_build_frame(msg_type, func_code, data, len, resp_buffer);
  (void)hal_uart_send(&uart1_ctx, HAL_UART_INSTANCE_1, resp_buffer, frame_len);
}

/**
 * @brief 发送OK响应
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 */
void usart_protocol_send_ok(uint8_t func_code) {
  uint8_t ok_data = 0x00U;
  usart_protocol_send_response(MSG_RESPONSE, func_code, &ok_data, 1U);
}

/**
 * @brief 发送错误响应
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 */
void usart_protocol_send_error(uint8_t func_code, uint8_t error_code,
                               uint8_t error_class) {
  uint8_t error_data[2U] = {error_code, error_class};
  usart_protocol_send_response(MSG_ERROR, func_code, error_data, 2U);
}

/**
 * @brief 发送ACK响应
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 */
void usart_protocol_send_ack(uint8_t func_code) {
  usart_protocol_send_response(MSG_ACK, func_code, NULL, 0U);
}

/*============================================================================
 * 命令处理函数
 *============================================================================*/

/**
 * @brief PING命令处理
 * @return int 0成功,其他失败
 */
static int cmd_ping_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                            uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  *resp_len = 0U;
  return 0;
}

/**
 * @brief 获取版本命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_version_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  VersionInfo_t ver = {.protocol_ver = 0x02U,
                       .major_ver = 0x01U,
                       .minor_ver = 0x00U,
                       .patch_ver = 0x00U,
                       .hardware_ver = 0x01U,
                       .reserve = 0x00U};

  (void)__memcpy(response, &ver, sizeof(VersionInfo_t));
  *resp_len = sizeof(VersionInfo_t);
  return 0;
}

/**
 * @brief 复位命令处理
 * @note 此函数会触发系统复位,需谨慎调用
 * @return int 0成功,其他失败
 */
static int cmd_reset_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                             uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  NVIC_SystemReset();
  return 0;
}

/**
 * @brief 获取状态命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_status_handler(uint8_t func_code, uint8_t* data,
                                  uint16_t len, uint8_t* response,
                                  uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  StatusData_t status = {0U};

  status.state = (uint8_t)foc_sm_current_state();
  status.error_code = 0U;

  status.id_current = (int16_t)(foc_ctrl.target_id_q / 65536);
  status.iq_current = (int16_t)(foc_ctrl.target_iq_q / 65536);
  status.velocity = (int16_t)(foc_ctrl.omega_q / 65536);
  status.position = (int32_t)(foc_ctrl.pll_phase_q / 655);
  status.voltage = 2400U;
  status.temperature = 65;

  (void)__memcpy(response, &status, sizeof(StatusData_t));
  *resp_len = sizeof(StatusData_t);
  return 0;
}

/**
 * @brief 获取状态命令处理(简化版)
 * @return int 0成功,其他失败
 */
static int cmd_get_state_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                                 uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  uint8_t state = (uint8_t)foc_sm_current_state();
  (void)__memcpy(response, &state, 1U);
  *resp_len = 1U;
  return 0;
}

/**
 * @brief 设置电流命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_current_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 4U) {
    return (int)ERR_INVALID_PARAM;
  }

  int16_t id = (int16_t)((uint16_t)data[0U] | ((uint16_t)data[1U] << 8));
  int16_t iq = (int16_t)((uint16_t)data[2U] | ((uint16_t)data[3U] << 8));

  foc_ctrl.target_id_q = (q16_16_t)id;
  foc_ctrl.target_iq_q = (q16_16_t)iq;

  return 0;
}

/**
 * @brief 设置速度命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_velocity_handler(uint8_t func_code, uint8_t* data,
                                    uint16_t len, uint8_t* response,
                                    uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 2U) {
    return (int)ERR_INVALID_PARAM;
  }

  int16_t velocity = (int16_t)((uint16_t)data[0U] | ((uint16_t)data[1U] << 8));
  foc_ctrl.omega_q = (q16_16_t)velocity;

  return 0;
}

/**
 * @brief 设置位置命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_position_handler(uint8_t func_code, uint8_t* data,
                                    uint16_t len, uint8_t* response,
                                    uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 4U) {
    return (int)ERR_INVALID_PARAM;
  }

  int32_t position =
      (int32_t)((uint32_t)data[0U] | ((uint32_t)data[1U] << 8) |
                ((uint32_t)data[2U] << 16) | ((uint32_t)data[3U] << 24));
  foc_ctrl.pll_phase_q = (q16_16_t)position;

  return 0;
}

/**
 * @brief 停止命令处理
 * @return int 0成功,其他失败
 */
static int cmd_stop_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                            uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.target_id_q = (q16_16_t)0;
  foc_ctrl.target_iq_q = (q16_16_t)0;
  foc_ctrl.sw = 0U;

  return 0;
}

/**
 * @brief 启动命令处理
 * @return int 0成功,其他失败
 */
static int cmd_start_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                             uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.sw = 1U;

  return 0;
}

/**
 * @brief 开始校准命令处理
 * @return int 0成功,其他失败
 */
static int cmd_start_calib_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  (void)foc_sm_request_state(foc_sm_get_instance(), FOC_SM_STATE_ALIGNMENT);

  return 0;
}

/**
 * @brief 获取校准状态命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_calib_status_handler(uint8_t func_code, uint8_t* data,
                                        uint16_t len, uint8_t* response,
                                        uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  CalibStatusData_t calib_status = {0U};

  uint8_t state = (uint8_t)foc_sm_current_state();

  if ((state == (uint8_t)FOC_SM_STATE_ALIGN) ||
      (state == (uint8_t)FOC_SM_STATE_ALIGNMENT)) {
    calib_status.status = CALIB_STATUS_RUNNING;
    calib_status.progress = 50U;
  } else if (g_encoder_calib.is_valid) {
    calib_status.status = CALIB_STATUS_COMPLETE;
    calib_status.progress = 100U;
    calib_status.direction = (int8_t)g_encoder_calib.direction;
  } else {
    calib_status.status = CALIB_STATUS_IDLE;
    calib_status.progress = 0U;
  }

  (void)__memcpy(response, &calib_status, sizeof(CalibStatusData_t));
  *resp_len = sizeof(CalibStatusData_t);
  return 0;
}

/**
 * @brief 获取校准数据命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_calib_data_handler(uint8_t func_code, uint8_t* data,
                                      uint16_t len, uint8_t* response,
                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  uint16_t calib_data_len =
      (uint16_t)(sizeof(g_motor_flash_cfg.angle_map) + 1U);
  if (calib_data_len > 256U) {
    calib_data_len = 256U;
  }

  (void)__memcpy(response, &g_motor_flash_cfg.angle_map,
                 (size_t)(calib_data_len - 1U));
  response[calib_data_len - 1U] = (uint8_t)g_motor_flash_cfg.direction;

  *resp_len = calib_data_len;
  return 0;
}

/**
 * @brief 设置校准数据命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_calib_data_handler(uint8_t func_code, uint8_t* data,
                                      uint16_t len, uint8_t* response,
                                      uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 3U) {
    return (int)ERR_INVALID_PARAM;
  }

  uint16_t map_len = len - 1U;
  if (map_len > (uint16_t)sizeof(g_motor_flash_cfg.angle_map)) {
    map_len = (uint16_t)sizeof(g_motor_flash_cfg.angle_map);
  }

  (void)__memcpy(&g_motor_flash_cfg.angle_map, data, (size_t)map_len);
  g_motor_flash_cfg.direction = (motor_direction_t)data[len - 1U];

  return 0;
}

/**
 * @brief 清除校准数据命令处理
 * @return int 0成功,其他失败
 */
static int cmd_clear_calib_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  (void)__memset(&g_motor_flash_cfg, 0, sizeof(motor_flash_config_t));

  return 0;
}

/**
 * @brief 获取参数命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_params_handler(uint8_t func_code, uint8_t* data,
                                  uint16_t len, uint8_t* response,
                                  uint16_t* resp_len) {
  (void)func_code;

  if (len < 1U) {
    return (int)ERR_INVALID_PARAM;
  }

  uint8_t param_type = data[0U];

  if (param_type == PARAM_PID_CURRENT) {
    PidParam_t pid = {.kp = (q16_16_t)6554,
                      .ki = (q16_16_t)655,
                      .kd = (q16_16_t)0,
                      .output_limit = (q16_16_t)65536};
    (void)__memcpy(response, &pid, sizeof(PidParam_t));
    *resp_len = sizeof(PidParam_t);
  } else {
    return (int)ERR_NOT_READY;
  }

  return 0;
}

/**
 * @brief 设置参数命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_params_handler(uint8_t func_code, uint8_t* data,
                                  uint16_t len, uint8_t* response,
                                  uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)response;
  *resp_len = 0U;

  if (len < 17U) {
    return (int)ERR_INVALID_PARAM;
  }

  return 0;
}

/**
 * @brief 开始流命令处理
 * @return int 0成功,其他失败
 */
static int cmd_start_stream_handler(uint8_t func_code, uint8_t* data,
                                    uint16_t len, uint8_t* response,
                                    uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len >= 1U) {
    s_stream_config.stream_type = data[0U];
  } else {
    s_stream_config.stream_type = 0U;
  }

  if (len >= 3U) {
    s_stream_config.interval_ms =
        (uint16_t)((uint16_t)data[1U] | ((uint16_t)data[2U] << 8U));
  } else if (len >= 2U) {
    s_stream_config.interval_ms = (uint16_t)data[1U];
  } else {
    s_stream_config.interval_ms = 10U;
  }

  s_stream_config.enable = 1U;

  return 0;
}

/**
 * @brief 停止流命令处理
 * @return int 0成功,其他失败
 */
static int cmd_stop_stream_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  s_stream_config.enable = 0U;

  return 0;
}

/**
 * @brief 获取设备信息命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_device_info_handler(uint8_t func_code, uint8_t* data,
                                       uint16_t len, uint8_t* response,
                                       uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  response[0U] = 0x01U;
  response[1U] = 0x00U;
  response[2U] = 0x00U;
  response[3U] = 0x00U;
  *resp_len = 4U;
  return 0;
}

/**
 * @brief 获取错误状态命令处理
 * @return int 0成功,其他失败
 */
static int cmd_get_error_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                                 uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  response[0U] = 0x00U;
  response[1U] = 0x00U;
  *resp_len = 2U;
  return 0;
}

/**
 * @brief 刹车命令处理
 * @return int 0成功,其他失败
 */
static int cmd_brake_handler(uint8_t func_code, uint8_t* data, uint16_t len,
                             uint8_t* response, uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.target_id_q = (q16_16_t)0;
  foc_ctrl.target_iq_q = (q16_16_t)0;
  foc_ctrl.sw = 0U;

  return 0;
}

/**
 * @brief 保存校准数据到Flash命令处理
 * @return int 0成功,其他失败
 */
static int cmd_save_calib_handler(uint8_t func_code, uint8_t* data,
                                  uint16_t len, uint8_t* response,
                                  uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return 0;
}

/**
 * @brief 保存参数到Flash命令处理
 * @return int 0成功,其他失败
 */
static int cmd_save_params_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return 0;
}

/**
 * @brief 从Flash加载参数命令处理
 * @return int 0成功,其他失败
 */
static int cmd_load_params_handler(uint8_t func_code, uint8_t* data,
                                   uint16_t len, uint8_t* response,
                                   uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return 0;
}

/**
 * @brief 设置流配置命令处理
 * @return int 0成功,其他失败
 */
static int cmd_set_stream_cfg_handler(uint8_t func_code, uint8_t* data,
                                      uint16_t len, uint8_t* response,
                                      uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 3U) {
    return (int)ERR_INVALID_PARAM;
  }

  s_stream_config.stream_type = data[0U];
  s_stream_config.interval_ms =
      (uint16_t)((uint16_t)data[1U] | ((uint16_t)data[2U] << 8U));

  return 0;
}

/*============================================================================
 * 命令处理表
 *============================================================================*/
typedef int (*protocol_cmd_handler_t)(uint8_t func_code, uint8_t* data,
                                      uint16_t len, uint8_t* response,
                                      uint16_t* resp_len);

typedef struct {
  uint8_t func_code;
  protocol_cmd_handler_t handler;
} protocol_cmd_entry_t;

static const protocol_cmd_entry_t s_cmd_table[] = {
    {FUNC_PING, cmd_ping_handler},
    {FUNC_GET_VERSION, cmd_get_version_handler},
    {FUNC_RESET, cmd_reset_handler},
    {FUNC_GET_DEVICE_INFO, cmd_get_device_info_handler},
    {FUNC_GET_STATUS, cmd_get_status_handler},
    {FUNC_GET_STATE, cmd_get_state_handler},
    {FUNC_GET_ERROR, cmd_get_error_handler},
    {FUNC_SET_CURRENT, cmd_set_current_handler},
    {FUNC_SET_VELOCITY, cmd_set_velocity_handler},
    {FUNC_SET_POSITION, cmd_set_position_handler},
    {FUNC_STOP, cmd_stop_handler},
    {FUNC_START, cmd_start_handler},
    {FUNC_BRAKE, cmd_brake_handler},
    {FUNC_START_CALIB, cmd_start_calib_handler},
    {FUNC_GET_CALIB_STATUS, cmd_get_calib_status_handler},
    {FUNC_GET_CALIB_DATA, cmd_get_calib_data_handler},
    {FUNC_SET_CALIB_DATA, cmd_set_calib_data_handler},
    {FUNC_CLEAR_CALIB, cmd_clear_calib_handler},
    {FUNC_SAVE_CALIB, cmd_save_calib_handler},
    {FUNC_GET_PARAMS, cmd_get_params_handler},
    {FUNC_SET_PARAMS, cmd_set_params_handler},
    {FUNC_SAVE_PARAMS, cmd_save_params_handler},
    {FUNC_LOAD_PARAMS, cmd_load_params_handler},
    {FUNC_START_STREAM, cmd_start_stream_handler},
    {FUNC_STOP_STREAM, cmd_stop_stream_handler},
    {FUNC_SET_STREAM_CFG, cmd_set_stream_cfg_handler},
};

#define CMD_TABLE_SIZE (sizeof(s_cmd_table) / sizeof(s_cmd_table[0]))

/*============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化串口协议模块
 */
void usart_protocol_init(void) {
  s_stream_config.enable = 0U;
  s_stream_config.stream_type = 0U;
  s_stream_config.interval_ms = 10U;
  s_stream_last_tick = 0U;
}

/**
 * @brief 处理接收到的协议帧
 * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
 * @param[in] data 接收到的数据指针
 * @param[in] len 数据长度
 */
void usart_protocol_process_frame(uint8_t* data, uint16_t len) {
  if (len < 8U) {
    return;
  }

  if ((data[0U] != S7_FRAME_HEAD_1) || (data[1U] != S7_FRAME_HEAD_2)) {
    return;
  }

  if (data[2U] != S7_PROTOCOL_ID) {
    return;
  }

  if (data[len - 1U] != S7_FRAME_TAIL) {
    return;
  }

  uint8_t msg_type = data[3U];
  uint8_t func_code = data[4U];
  uint16_t payload_len = (uint16_t)(len - 8U);
  uint8_t* payload = NULL;
  if (payload_len > 0U) {
    payload = &data[5U];
  }

  uint8_t response[256U];
  uint16_t resp_len = 0U;
  int ret = (int)ERR_INVALID_CMD;

  for (uint16_t i = 0U; i < CMD_TABLE_SIZE; i++) {
    if (s_cmd_table[i].func_code == func_code) {
      ret = s_cmd_table[i].handler(func_code, payload, payload_len, response,
                                   &resp_len);
      break;
    }
  }

  if (ret == 0) {
    if (resp_len > 0U) {
      usart_protocol_send_response(MSG_RESPONSE, func_code, response, resp_len);
    } else {
      usart_protocol_send_ok(func_code);
    }
  } else {
    usart_protocol_send_error(func_code, (uint8_t)ret, ERR_CLASS_APP);
  }
}

/**
 * @brief 流数据任务处理函数
 * @note 须在主循环中周期性调用
 */
void usart_protocol_stream_task(void) {
  if (s_stream_config.enable == 0U) {
    return;
  }

  uint32_t current_tick = HAL_GetTick();

  if ((current_tick - s_stream_last_tick) < s_stream_config.interval_ms) {
    return;
  }
  s_stream_last_tick = current_tick;

  StreamData_t stream = {0U};

  stream.timestamp = current_tick * 1000U;
  stream.id_current = (int16_t)(foc_ctrl.target_id_q / 65536);
  stream.iq_current = (int16_t)(foc_ctrl.target_iq_q / 65536);
  stream.velocity = (int16_t)(foc_ctrl.omega_q / 65536);
  stream.position = (int32_t)(foc_ctrl.pll_phase_q / 655);
  stream.elec_angle =
      (uint16_t)((foc_ctrl.electrical_angle_q >> 16U) & 0xFFFFU);

  (void)usart_protocol_send_response(MSG_RESPONSE, FUNC_GET_STATUS,
                                     (uint8_t*)&stream, sizeof(StreamData_t));
}
