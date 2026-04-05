//
// Created by fubingyan on 25-4-6.
//

/**
 * @file    usart_protocol.c
 * @author  fubingyan
 * @version V1.0.0
 * @date    2025-04-06
 * @brief   UART S7协议处理器实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "usart_protocol.h"

#include <string.h>

#include "app.h"
#include "crc16.h"
#include "encoder_alignment.h"
#include "foc_ctrl_q16.h"
#include "foc_sm.h"
#include "hal_uart.h"
#include "kfifo/kfifo.h"
#include "protocol_parser.h"
#include "stm32g4xx_hal.h"

/* Private constants ---------------------------------------------------------*/

#define PARSER_OUTPUT_BUFFER_SIZE 128U

/* Private variables ---------------------------------------------------------*/

extern foc_ctrl_t foc_ctrl;
extern encoder_calibration_t g_encoder_calib;
extern motor_flash_config_t g_motor_flash_cfg;

static usart_protocol_context_t* s_current_ctx = NULL;

/* Private function prototypes -----------------------------------------------*/

static uint16_t s7_build_frame(uint8_t msg_type, uint8_t func_code,
                               const uint8_t* data, uint16_t len,
                               uint8_t* output);

static usart_protocol_error_t cmd_ping_handler(uint8_t func_code, uint8_t* data,
                                               uint16_t len, uint8_t* response,
                                               uint16_t* resp_len);

static usart_protocol_error_t cmd_get_version_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_reset_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len);

static usart_protocol_error_t cmd_get_status_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len);

static usart_protocol_error_t cmd_get_state_handler(uint8_t func_code,
                                                    uint8_t* data, uint16_t len,
                                                    uint8_t* response,
                                                    uint16_t* resp_len);

static usart_protocol_error_t cmd_set_current_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_set_velocity_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len);

static usart_protocol_error_t cmd_set_position_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len);

static usart_protocol_error_t cmd_stop_handler(uint8_t func_code, uint8_t* data,
                                               uint16_t len, uint8_t* response,
                                               uint16_t* resp_len);

static usart_protocol_error_t cmd_start_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len);

static usart_protocol_error_t cmd_start_calib_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_get_calib_status_handler(uint8_t func_code,
                                                           uint8_t* data,
                                                           uint16_t len,
                                                           uint8_t* response,
                                                           uint16_t* resp_len);

static usart_protocol_error_t cmd_get_calib_data_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len);

static usart_protocol_error_t cmd_set_calib_data_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len);

static usart_protocol_error_t cmd_clear_calib_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_get_params_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len);

static usart_protocol_error_t cmd_set_params_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len);

static usart_protocol_error_t cmd_start_stream_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len);

static usart_protocol_error_t cmd_stop_stream_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_get_device_info_handler(uint8_t func_code,
                                                          uint8_t* data,
                                                          uint16_t len,
                                                          uint8_t* response,
                                                          uint16_t* resp_len);

static usart_protocol_error_t cmd_get_error_handler(uint8_t func_code,
                                                    uint8_t* data, uint16_t len,
                                                    uint8_t* response,
                                                    uint16_t* resp_len);

static usart_protocol_error_t cmd_brake_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len);

static usart_protocol_error_t cmd_save_calib_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len);

static usart_protocol_error_t cmd_save_params_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_load_params_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len);

static usart_protocol_error_t cmd_set_stream_cfg_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len);

static uint16_t s7_get_frame_len(uint8_t* buffer, uint16_t len);
static protocol_parser_error_t s7_check_cb(uint8_t* buffer, uint16_t len);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 初始化串口协议模块
 * @param ctx 串口协议模块上下文指针
 * @param config 配置结构体指针
 * @return 操作结果错误码
 */
usart_protocol_error_t usart_protocol_init(
    usart_protocol_context_t* ctx, const usart_protocol_config_t* config) {
  // 检查参数有效性
  if (!ctx || !config) {
    return USART_PROTOCOL_ERROR_NULL_PTR;
  }

  // 如果已初始化，先反初始化
  if (ctx->initialized) {
    usart_protocol_deinit(ctx);
  }

  // 保存配置
  memcpy(&ctx->config, config, sizeof(usart_protocol_config_t));

  // 初始化运行时状态
  ctx->stream_cfg.enable = 0U;
  ctx->stream_cfg.stream_type = 0U;
  ctx->stream_cfg.interval_ms = 10U;
  ctx->stream_last_tick = 0U;
  ctx->initialized = true;

  // 初始化 S7 协议解析器
  static const uint8_t s7_header[] = {S7_FRAME_HEAD_1, S7_FRAME_HEAD_2};
  static const uint8_t s7_footer[] = {S7_FRAME_TAIL};

  protocol_parser_config_t parser_cfg = {
      .name = ctx->config.name,
      .header = s7_header,
      .header_len = sizeof(s7_header),
      .footer = s7_footer,
      .footer_len = sizeof(s7_footer),
      .input_buffer_len = PARSER_OUTPUT_BUFFER_SIZE,
      .output_buffer = ctx->parser_output_buffer,
      .output_buffer_len = sizeof(ctx->parser_output_buffer),
      .get_len_cb = s7_get_frame_len,
      .check_cb = s7_check_cb,
  };
  (void)protocol_parser_init(&ctx->parser_ctx, &parser_cfg);

  return USART_PROTOCOL_OK;
}

/**
 * @brief 反初始化串口协议模块
 * @param ctx 串口协议模块上下文指针
 */
void usart_protocol_deinit(usart_protocol_context_t* ctx) {
  if (!ctx) {
    return;
  }

  ctx->stream_cfg.enable = 0U;
  ctx->initialized = false;
}

/**
 * @brief 检查模块是否已初始化
 * @param ctx 串口协议模块上下文指针
 * @return true表示已初始化，false表示未初始化
 */
bool usart_protocol_is_initialized(const usart_protocol_context_t* ctx) {
  return (ctx && ctx->initialized);
}

/**
 * @brief 发送协议响应帧
 * @param ctx 串口协议模块上下文指针
 * @param msg_type 消息类型
 * @param func_code 功能码
 * @param data 数据指针,可为NULL
 * @param len 数据长度
 */
void usart_protocol_send_response(usart_protocol_context_t* ctx,
                                  uint8_t msg_type, uint8_t func_code,
                                  const uint8_t* data, uint16_t len) {
  if (!ctx || !ctx->initialized) {
    return;
  }

  uint16_t frame_len =
      s7_build_frame(msg_type, func_code, data, len, ctx->output_buffer);
  (void)hal_uart_send((hal_uart_context_t*)ctx->config.uart_ctx,
                      ctx->config.uart_instance, ctx->output_buffer, frame_len);
}

/**
 * @brief 发送OK响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 */
void usart_protocol_send_ok(usart_protocol_context_t* ctx, uint8_t func_code) {
  uint8_t ok_data = 0x00U;
  usart_protocol_send_response(ctx, MSG_RESPONSE, func_code, &ok_data, 1U);
}

/**
 * @brief 发送错误响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 * @param error_code 错误码
 * @param error_class 错误类
 */
void usart_protocol_send_error(usart_protocol_context_t* ctx, uint8_t func_code,
                               uint8_t error_code, uint8_t error_class) {
  uint8_t error_data[2U] = {error_code, error_class};
  usart_protocol_send_response(ctx, MSG_ERROR, func_code, error_data, 2U);
}

/**
 * @brief 发送ACK响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 */
void usart_protocol_send_ack(usart_protocol_context_t* ctx, uint8_t func_code) {
  usart_protocol_send_response(ctx, MSG_ACK, func_code, NULL, 0U);
}

/**
 * @brief 处理接收到的协议帧
 * @param ctx 串口协议模块上下文指针
 * @param data 接收到的数据指针
 * @param len 数据长度
 * @return 操作结果错误码
 */
usart_protocol_error_t usart_protocol_process_frame(
    usart_protocol_context_t* ctx, uint8_t* data, uint16_t len) {
  // 检查参数有效性
  if (!ctx || !data) {
    return USART_PROTOCOL_ERROR_NULL_PTR;
  }

  // 检查初始化状态
  if (!ctx->initialized) {
    return USART_PROTOCOL_ERROR_UNINITIALIZED;
  }

  // 设置当前上下文指针，供命令处理函数使用
  s_current_ctx = ctx;

  if (len < 8U) {
    return USART_PROTOCOL_ERROR_DATA_LEN;
  }

  if ((data[0U] != S7_FRAME_HEAD_1) || (data[1U] != S7_FRAME_HEAD_2)) {
    return USART_PROTOCOL_ERROR_CHECKSUM;
  }

  if (data[2U] != S7_PROTOCOL_ID) {
    return USART_PROTOCOL_ERROR_CHECKSUM;
  }

  if (data[len - 1U] != S7_FRAME_TAIL) {
    return USART_PROTOCOL_ERROR_CHECKSUM;
  }

  uint8_t msg_type = data[3U];
  uint8_t func_code = data[4U];
  uint16_t payload_len = (uint16_t)(len - 8U);
  uint8_t* payload = NULL;
  if (payload_len > 0U) {
    payload = &data[5U];
  }

  uint16_t resp_len = 0U;
  usart_protocol_error_t ret = USART_PROTOCOL_ERROR_INVALID_CMD;

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

  const uint16_t cmd_table_size = sizeof(s_cmd_table) / sizeof(s_cmd_table[0]);

  for (uint16_t i = 0U; i < cmd_table_size; i++) {
    if (s_cmd_table[i].func_code == func_code) {
      ret = s_cmd_table[i].handler(func_code, payload, payload_len,
                                   ctx->response_buffer, &resp_len);
      break;
    }
  }

  if (ret == USART_PROTOCOL_OK) {
    if (resp_len > 0U) {
      usart_protocol_send_response(ctx, MSG_RESPONSE, func_code,
                                   ctx->response_buffer, resp_len);
    } else {
      usart_protocol_send_ok(ctx, func_code);
    }
  } else {
    usart_protocol_send_error(ctx, func_code, (uint8_t)ret,
                              USART_PROTOCOL_ERR_CLASS_APP);
  }

  return USART_PROTOCOL_OK;
}

/**
 * @brief 流数据任务处理函数
 * @param ctx 串口协议模块上下文指针
 * @note 须在主循环中周期性调用
 */
void usart_protocol_stream_task(usart_protocol_context_t* ctx) {
  if (!ctx || !ctx->initialized) {
    return;
  }

  if (ctx->stream_cfg.enable == 0U) {
    return;
  }

  uint32_t current_tick = micros();

  if ((current_tick - ctx->stream_last_tick) < ctx->stream_cfg.interval_ms) {
    return;
  }
  ctx->stream_last_tick = current_tick;

  usart_protocol_stream_data_t stream = {0U};

  stream.timestamp = current_tick * 1000U;
  stream.id_current = (int16_t)(foc_ctrl.target_id_q / 65536);
  stream.iq_current = (int16_t)(foc_ctrl.target_iq_q / 65536);
  stream.velocity = (int16_t)(foc_ctrl.omega_q / 65536);
  stream.position = (int32_t)(foc_ctrl.pll_phase_q / 655);
  stream.elec_angle =
      (uint16_t)((foc_ctrl.electrical_angle_q >> 16U) & 0xFFFFU);

  (void)usart_protocol_send_response(ctx, MSG_RESPONSE, FUNC_GET_STATUS,
                                     (uint8_t*)&stream,
                                     sizeof(usart_protocol_stream_data_t));
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 构建S7协议帧
 * @param msg_type 消息类型
 * @param func_code 功能码
 * @param data 数据指针,可为NULL
 * @param len 数据长度
 * @param output 输出缓冲区指针
 * @return 帧长度
 */
static uint16_t s7_build_frame(uint8_t msg_type, uint8_t func_code,
                               const uint8_t* data, uint16_t len,
                               uint8_t* output) {
  // 直接在 output 中构建帧，避免使用中间缓冲区
  // 帧结构: head(2) + payload(3+len) + crc(2) + tail(1)
  // payload: protocol_id(1) + msg_type(1) + func_code(1) + data(n)

  output[2U] = S7_PROTOCOL_ID;
  output[3U] = msg_type;
  output[4U] = func_code;

  if ((len > 0U) && (data != NULL)) {
    (void)memcpy(&output[5U], data, (size_t)len);
  }

  // CRC 计算范围: 从 protocol_id 到数据结束
  uint16_t crc = crc16_modbus(&output[2U], (uint16_t)(3U + len));

  // 写入帧头
  output[0U] = S7_FRAME_HEAD_1;
  output[1U] = S7_FRAME_HEAD_2;

  // 写入 CRC 和帧尾
  output[2U + 3U + len] = (uint8_t)(crc & 0xFFU);
  output[2U + 3U + len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
  output[2U + 3U + len + 2U] = S7_FRAME_TAIL;

  return (uint16_t)(8U + len);
}

/**
 * @brief PING命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_ping_handler(uint8_t func_code, uint8_t* data,
                                               uint16_t len, uint8_t* response,
                                               uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;
  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取版本命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_version_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  usart_protocol_version_info_t ver = {.protocol_ver = 0x02U,
                                       .major_ver = 0x01U,
                                       .minor_ver = 0x00U,
                                       .patch_ver = 0x00U,
                                       .hardware_ver = 0x01U,
                                       .reserve = 0x00U};

  (void)memcpy(response, &ver, sizeof(usart_protocol_version_info_t));
  *resp_len = sizeof(usart_protocol_version_info_t);
  return USART_PROTOCOL_OK;
}

/**
 * @brief 复位命令处理
 * @note 此函数会触发系统复位,需谨慎调用
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_reset_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  {
    NVIC_SystemReset();
  }
  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取状态命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_status_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  usart_protocol_status_data_t status = {0U};

  status.state = (uint8_t)foc_sm_current_state();
  status.error_code = 0U;

  status.id_current = (int16_t)(foc_ctrl.target_id_q / 65536);
  status.iq_current = (int16_t)(foc_ctrl.target_iq_q / 65536);
  status.velocity = (int16_t)(foc_ctrl.omega_q / 65536);
  status.position = (int32_t)(foc_ctrl.pll_phase_q / 655);
  status.voltage = 2400U;
  status.temperature = 65;

  (void)memcpy(response, &status, sizeof(usart_protocol_status_data_t));
  *resp_len = sizeof(usart_protocol_status_data_t);
  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取状态命令处理(简化版)
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_state_handler(uint8_t func_code,
                                                    uint8_t* data, uint16_t len,
                                                    uint8_t* response,
                                                    uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  uint8_t state = (uint8_t)foc_sm_current_state();
  (void)memcpy(response, &state, 1U);
  *resp_len = 1U;
  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置电流命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_current_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 4U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  int16_t id = (int16_t)((uint16_t)data[0U] | ((uint16_t)data[1U] << 8));
  int16_t iq = (int16_t)((uint16_t)data[2U] | ((uint16_t)data[3U] << 8));

  foc_ctrl.target_id_q = (q16_16_t)id;
  foc_ctrl.target_iq_q = (q16_16_t)iq;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置速度命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_velocity_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 2U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  int16_t velocity = (int16_t)((uint16_t)data[0U] | ((uint16_t)data[1U] << 8));
  foc_ctrl.omega_q = (q16_16_t)velocity;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置位置命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_position_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 4U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  int32_t position =
      (int32_t)((uint32_t)data[0U] | ((uint32_t)data[1U] << 8) |
                ((uint32_t)data[2U] << 16) | ((uint32_t)data[3U] << 24));
  foc_ctrl.pll_phase_q = (q16_16_t)position;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 停止命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_stop_handler(uint8_t func_code, uint8_t* data,
                                               uint16_t len, uint8_t* response,
                                               uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.target_id_q = (q16_16_t)0;
  foc_ctrl.target_iq_q = (q16_16_t)0;
  foc_ctrl.sw = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 启动命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_start_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.sw = 1U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 开始校准命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_start_calib_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  (void)foc_sm_request_state(foc_sm_get_instance(), FOC_SM_STATE_ALIGNMENT);

  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取校准状态命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_calib_status_handler(uint8_t func_code,
                                                           uint8_t* data,
                                                           uint16_t len,
                                                           uint8_t* response,
                                                           uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  usart_protocol_calib_status_data_t calib_status = {0U};

  uint8_t state = (uint8_t)foc_sm_current_state();

  if ((state == (uint8_t)FOC_SM_STATE_ALIGN) ||
      (state == (uint8_t)FOC_SM_STATE_ALIGNMENT)) {
    calib_status.status = USART_PROTOCOL_CALIB_STATUS_RUNNING;
    calib_status.progress = 50U;
  } else if (g_encoder_calib.is_valid) {
    calib_status.status = USART_PROTOCOL_CALIB_STATUS_COMPLETE;
    calib_status.progress = 100U;
    calib_status.direction = (int8_t)g_encoder_calib.direction;
  } else {
    calib_status.status = USART_PROTOCOL_CALIB_STATUS_IDLE;
    calib_status.progress = 0U;
  }

  (void)memcpy(response, &calib_status,
               sizeof(usart_protocol_calib_status_data_t));
  *resp_len = sizeof(usart_protocol_calib_status_data_t);
  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取校准数据命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_calib_data_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  uint16_t calib_data_len =
      (uint16_t)(sizeof(g_motor_flash_cfg.angle_map) + 1U);
  if (calib_data_len > 256U) {
    calib_data_len = 256U;
  }

  (void)memcpy(response, &g_motor_flash_cfg.angle_map,
               (size_t)(calib_data_len - 1U));
  response[calib_data_len - 1U] = (uint8_t)g_motor_flash_cfg.direction;

  *resp_len = calib_data_len;
  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置校准数据命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_calib_data_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 3U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  uint16_t map_len = len - 1U;
  if (map_len > (uint16_t)sizeof(g_motor_flash_cfg.angle_map)) {
    map_len = (uint16_t)sizeof(g_motor_flash_cfg.angle_map);
  }

  (void)memcpy(&g_motor_flash_cfg.angle_map, data, (size_t)map_len);
  g_motor_flash_cfg.direction = (motor_direction_t)data[len - 1U];

  return USART_PROTOCOL_OK;
}

/**
 * @brief 清除校准数据命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_clear_calib_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  (void)__memset(&g_motor_flash_cfg, 0, sizeof(motor_flash_config_t));

  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取参数命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_params_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len) {
  (void)func_code;

  if (len < 1U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  uint8_t param_type = data[0U];

  if (param_type == USART_PROTOCOL_PARAM_PID_CURRENT) {
    usart_protocol_pid_param_t pid = {.kp = (q16_16_t)6554,
                                      .ki = (q16_16_t)655,
                                      .kd = (q16_16_t)0,
                                      .output_limit = (q16_16_t)65536};
    (void)memcpy(response, &pid, sizeof(usart_protocol_pid_param_t));
    *resp_len = sizeof(usart_protocol_pid_param_t);
  } else {
    return USART_PROTOCOL_ERROR_NOT_READY;
  }

  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置参数命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_params_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)response;
  *resp_len = 0U;

  if (len < 17U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  return USART_PROTOCOL_OK;
}

/**
 * @brief 开始流命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_start_stream_handler(uint8_t func_code,
                                                       uint8_t* data,
                                                       uint16_t len,
                                                       uint8_t* response,
                                                       uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (s_current_ctx == NULL) {
    return USART_PROTOCOL_ERROR_UNINITIALIZED;
  }

  if (len >= 1U) {
    s_current_ctx->stream_cfg.stream_type = data[0U];
  } else {
    s_current_ctx->stream_cfg.stream_type = 0U;
  }

  if (len >= 3U) {
    s_current_ctx->stream_cfg.interval_ms =
        (uint16_t)((uint16_t)data[1U] | ((uint16_t)data[2U] << 8U));
  } else if (len >= 2U) {
    s_current_ctx->stream_cfg.interval_ms = (uint16_t)data[1U];
  } else {
    s_current_ctx->stream_cfg.interval_ms = 10U;
  }

  s_current_ctx->stream_cfg.enable = 1U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 停止流命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_stop_stream_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  if (s_current_ctx == NULL) {
    return USART_PROTOCOL_ERROR_UNINITIALIZED;
  }

  s_current_ctx->stream_cfg.enable = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取设备信息命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_device_info_handler(uint8_t func_code,
                                                          uint8_t* data,
                                                          uint16_t len,
                                                          uint8_t* response,
                                                          uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  response[0U] = 0x01U;
  response[1U] = 0x00U;
  response[2U] = 0x00U;
  response[3U] = 0x00U;
  *resp_len = 4U;
  return USART_PROTOCOL_OK;
}

/**
 * @brief 获取错误状态命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_get_error_handler(uint8_t func_code,
                                                    uint8_t* data, uint16_t len,
                                                    uint8_t* response,
                                                    uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;

  response[0U] = 0x00U;
  response[1U] = 0x00U;
  *resp_len = 2U;
  return USART_PROTOCOL_OK;
}

/**
 * @brief 刹车命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_brake_handler(uint8_t func_code,
                                                uint8_t* data, uint16_t len,
                                                uint8_t* response,
                                                uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  foc_ctrl.target_id_q = (q16_16_t)0;
  foc_ctrl.target_iq_q = (q16_16_t)0;
  foc_ctrl.sw = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 保存校准数据到Flash命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_save_calib_handler(uint8_t func_code,
                                                     uint8_t* data,
                                                     uint16_t len,
                                                     uint8_t* response,
                                                     uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 保存参数到Flash命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_save_params_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 从Flash加载参数命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_load_params_handler(uint8_t func_code,
                                                      uint8_t* data,
                                                      uint16_t len,
                                                      uint8_t* response,
                                                      uint16_t* resp_len) {
  (void)func_code;
  (void)data;
  (void)len;
  (void)response;
  *resp_len = 0U;

  return USART_PROTOCOL_OK;
}

/**
 * @brief 设置流配置命令处理
 * @return 操作结果错误码
 */
static usart_protocol_error_t cmd_set_stream_cfg_handler(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len) {
  (void)func_code;
  (void)response;
  *resp_len = 0U;

  if (len < 3U) {
    return USART_PROTOCOL_ERROR_INVALID_PARAM;
  }

  if (s_current_ctx == NULL) {
    return USART_PROTOCOL_ERROR_UNINITIALIZED;
  }

  s_current_ctx->stream_cfg.stream_type = data[0U];
  s_current_ctx->stream_cfg.interval_ms =
      (uint16_t)((uint16_t)data[1U] | ((uint16_t)data[2U] << 8U));

  return USART_PROTOCOL_OK;
}

/**
 * @brief 喂入接收数据到协议解析器
 * @param ctx 串口协议模块上下文指针
 * @param data 接收到的数据指针
 * @param len 数据长度
 */
void usart_protocol_feed(usart_protocol_context_t* ctx, const uint8_t* data,
                         uint16_t len) {
  if (!ctx || !ctx->initialized) {
    return;
  }
  if (data != NULL && len > 0U) {
    (void)protocol_parser_feed(&ctx->parser_ctx, data, len);
  }
}

/**
 * @brief 协议解析任务处理函数
 * @param ctx 串口协议模块上下文指针
 * @note 须在主循环中周期性调用，自动解析并处理帧
 */
void usart_protocol_parse_task(usart_protocol_context_t* ctx) {
  if (!ctx || !ctx->initialized) {
    return;
  }

  uint16_t frame_len = 0U;
  uint8_t* frame_data = NULL;

  const protocol_parser_error_t err =
      protocol_parser_parse(&ctx->parser_ctx, &frame_len, &frame_data);

  if (err == PROTOCOL_PARSER_OK && frame_data != NULL) {
    (void)usart_protocol_process_frame(ctx, frame_data, frame_len);
  }

  (void)protocol_parser_tick(&ctx->parser_ctx);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 计算 S7 协议帧长度
 * @param buffer 数据缓冲区
 * @param len 当前数据长度
 * @return 完整帧长度，0表示不完整
 * @note S7帧格式: head(2) + proto_id(1) + msg_type(1) + func(1) + data(n) +
 *                 crc(2) + tail(1)
 *       最小帧长: 8字节 (无数据时)
 */
static uint16_t s7_get_frame_len(uint8_t* buffer, uint16_t len) {
  if (len < 8U) {
    return 0U;
  }

  for (uint16_t i = 7U; i < len; i++) {
    if (buffer[i] == S7_FRAME_TAIL) {
      return (uint16_t)(i + 1U);
    }
  }

  return 0U;
}

/**
 * @brief S7 协议帧校验 - CRC16 Modbus
 * @param buffer 数据缓冲区
 * @param len 数据长度
 * @return 协议解析器错误码
 */
static protocol_parser_error_t s7_check_cb(uint8_t* buffer, uint16_t len) {
  if (len < 8U) {
    return PROTOCOL_PARSER_ERROR_CHECKSUM;
  }

  // CRC16 计算范围: 从协议ID到CRC之前
  uint16_t calc = crc16_modbus(&buffer[2U], (uint16_t)(len - 5U));
  uint16_t recv =
      (uint16_t)buffer[len - 3U] | ((uint16_t)buffer[len - 2U] << 8U);

  return (calc == recv) ? PROTOCOL_PARSER_OK : PROTOCOL_PARSER_ERROR_CHECKSUM;
}
