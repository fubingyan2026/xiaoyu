//
// Created by fubingyan on 25-4-5.
//

/**
 * @file    can_comm.c
 * @author  fubingyan
 * @version V2.0.0
 * @date    2025-04-05
 * @brief   CAN通信模块实现
 * @attention
 *
 * Copyright (c) 2025 Company Name.
 * All rights reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "can_comm.h"

#include <string.h>

#include "public.h"
#include "bsp_delay.h"

/* Private constants ---------------------------------------------------------*/

#define CAN_COMM_PRINTF(...)  // DEBUG_INFO(__VA_ARGS__)

#define CAN_COMM_PROTOCOL_LEN \
  (sizeof(can_comm_protocol_head_t) + sizeof(can_comm_protocol_end_t))
#define CAN_COMM_HEAD_LEN sizeof(can_comm_protocol_head_t)

/* Private variables ---------------------------------------------------------*/

can_comm_rx_t* g_can_comm_rx_list = NULL;
can_comm_tx_t* g_can_comm_tx_list = NULL;
can_comm_stats_t g_can_stats = {0};

/* Private function prototypes -----------------------------------------------*/

static uint8_t can_comm_validate_rx_config(const can_comm_rx_config_t* config);
static uint8_t can_comm_validate_tx_config(const can_comm_tx_config_t* config);
static uint8_t can_comm_floor_to_std(uint8_t input_value);
static uint16_t can_comm_get_frame_len(uint8_t* buffer, uint16_t len);
static protocol_parser_error_t can_comm_check_frame(uint8_t* buffer,
                                                    uint16_t len);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 验证CAN接收配置的有效性
 * @param config CAN接收配置指针
 * @return 有效返回1，无效返回0
 */
static uint8_t can_comm_validate_rx_config(const can_comm_rx_config_t* config) {
  if (config == NULL) {
    CAN_COMM_PRINTF("CAN RX config is NULL\n");
    return 0;
  }
  if (config->name == NULL) {
    CAN_COMM_PRINTF("CAN RX config name is NULL\n");
    return 0;
  }
  if (config->data_len == 0 || config->data_len > CAN_COMM_MAX_DATA_LEN) {
    CAN_COMM_PRINTF("CAN RX config data length invalid: %d\n",
                    config->data_len);
    return 0;
  }
  if (config->offline_ms < CAN_COMM_MIN_OFFLINE_MS) {
    CAN_COMM_PRINTF("CAN RX offline timeout too small: %d\n",
                    config->offline_ms);
  }
  return 1;
}

/**
 * @brief 验证CAN发送配置的有效性
 * @param config CAN发送配置指针
 * @return 有效返回1，无效返回0
 */
static uint8_t can_comm_validate_tx_config(const can_comm_tx_config_t* config) {
  if (config == NULL) {
    CAN_COMM_PRINTF("CAN TX config is NULL\n");
    return 0;
  }
  if (config->name == NULL) {
    CAN_COMM_PRINTF("CAN TX config name is NULL\n");
    return 0;
  }
  if (config->data_len == 0 || config->data_len > CAN_COMM_MAX_DATA_LEN) {
    CAN_COMM_PRINTF("CAN TX config data length invalid: %d\n",
                    config->data_len);
    return 0;
  }
  return 1;
}

/**
 * @brief 向下取整到最近标准值
 * @param input_value 输入数据长度
 * @return 标准数据长度
 */
static uint8_t can_comm_floor_to_std(uint8_t input_value) {
  if (input_value <= 8) return input_value;
  static const uint8_t STANDARD_VALUES[] = {8, 12, 16, 20, 24, 32, 48, 64};
  for (int i = sizeof(STANDARD_VALUES) / sizeof(STANDARD_VALUES[0]) - 1; i >= 0;
       --i) {
    if (input_value >= STANDARD_VALUES[i]) {
      return STANDARD_VALUES[i];
    }
  }
  return STANDARD_VALUES[0];
}

/**
 * @brief 计算CAN协议帧长度
 * @param buffer 帧数据缓冲区
 * @param len 当前数据长度
 * @return 完整帧长度，0表示数据不完整
 */
static uint16_t can_comm_get_frame_len(uint8_t* buffer, uint16_t len) {
  if (len < sizeof(can_comm_protocol_head_t)) {
    return 0;
  }
  can_comm_protocol_head_t* head = (can_comm_protocol_head_t*)buffer;
  if (head->start != 'S') {
    return 0;
  }
  return sizeof(can_comm_protocol_head_t) + head->data_length +
         sizeof(can_comm_protocol_end_t);
}

/**
 * @brief 校验CAN协议帧
 * @param buffer 帧数据缓冲区
 * @param len 帧数据长度
 * @return 校验结果错误码
 */
static protocol_parser_error_t can_comm_check_frame(uint8_t* buffer,
                                                    uint16_t len) {
  if (len <
      sizeof(can_comm_protocol_head_t) + sizeof(can_comm_protocol_end_t)) {
    return PROTOCOL_PARSER_ERROR_INCOMPLETE;
  }

  can_comm_protocol_head_t* head = (can_comm_protocol_head_t*)buffer;
  uint16_t payload_len = head->data_length;
  uint8_t* crc_ptr = buffer + sizeof(can_comm_protocol_head_t) + payload_len;

  uint8_t calc_crc = get_CRC8_check_sum(
      buffer + sizeof(can_comm_protocol_head_t), payload_len, 0);

  if (calc_crc != *crc_ptr) {
    return PROTOCOL_PARSER_ERROR_CHECKSUM;
  }
  return PROTOCOL_PARSER_OK;
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 注册CAN接收实例
 * @param config 接收配置结构体指针
 * @return 成功返回接收实例指针，失败返回NULL
 */
can_comm_rx_t* can_comm_rx_register(const can_comm_rx_config_t* config) {
  if (!can_comm_validate_rx_config(config)) {
    return NULL;
  }

  can_comm_rx_t* existing = can_comm_get_rx_instance(config->name);
  if (existing != NULL) {
    CAN_COMM_PRINTF("CAN RX instance %s already exists\n", config->name);
    return existing;
  }

  can_comm_rx_t* new_instance = (can_comm_rx_t*)__malloc(sizeof(can_comm_rx_t));
  if (new_instance == NULL) {
    CAN_COMM_PRINTF("Failed to allocate memory for CAN RX instance %s\n",
                    config->name);
    return NULL;
  }
  __memset(new_instance, 0, sizeof(can_comm_rx_t));
  new_instance->last_sequence = 0xFF;

  __memcpy(&new_instance->config, config, sizeof(can_comm_rx_config_t));

  new_instance->output_buffer =
      (uint8_t*)__malloc(config->data_len + CAN_COMM_PROTOCOL_LEN);
  if (new_instance->output_buffer == NULL) {
    CAN_COMM_PRINTF("Failed to allocate output buffer for CAN RX instance %s\n",
                    config->name);
    __free(new_instance);
    return NULL;
  }

  protocol_parser_config_t parser_config = {
      .name = config->name,
      .header = (const uint8_t*)"S",
      .header_len = 1,
      .footer = (const uint8_t*)"E",
      .footer_len = 1,
      .output_buffer = new_instance->output_buffer,
      .output_buffer_len = config->data_len + CAN_COMM_PROTOCOL_LEN,
      .input_buffer_len =
          (config->data_len + CAN_COMM_PROTOCOL_LEN) * CAN_COMM_FIFO_MULTIPLIER,
      .get_len_cb = can_comm_get_frame_len,
      .check_cb = can_comm_check_frame,
  };

  new_instance->parser =
      (protocol_parser_context_t*)__malloc(sizeof(protocol_parser_context_t));
  if (new_instance->parser == NULL) {
    CAN_COMM_PRINTF("Failed to allocate parser for CAN RX instance %s\n",
                    config->name);
    __free(new_instance->output_buffer);
    __free(new_instance);
    return NULL;
  }
  __memset(new_instance->parser, 0, sizeof(protocol_parser_context_t));

  protocol_parser_error_t err =
      protocol_parser_init(new_instance->parser, &parser_config);
  if (err != PROTOCOL_PARSER_OK) {
    CAN_COMM_PRINTF("Failed to init parser for CAN RX instance %s, error: %d\n",
                    config->name, err);
    __free(new_instance->parser);
    __free(new_instance->output_buffer);
    __free(new_instance);
    return NULL;
  }

  const daemon_config_t daemon_config = {
      .offline_cb = config->offline_cb,
      .owner_ptr = new_instance,
      .name = config->name,
      .reload_timeout_ms = config->offline_ms > 0 ? config->offline_ms
                                                  : CAN_COMM_DEFAULT_OFFLINE_MS,
      .init_wait_time_ms = 200,
  };
  new_instance->daemon = daemon_register(&daemon_config);
  if (new_instance->daemon == NULL) {
    CAN_COMM_PRINTF("Failed to register daemon for CAN RX instance %s\n",
                    config->name);
    protocol_parser_deinit(new_instance->parser);
    __free(new_instance->parser);
    __free(new_instance->output_buffer);
    __free(new_instance);
    return NULL;
  }

  new_instance->user_data = (void*)__malloc(config->data_len);
  if (new_instance->user_data == NULL) {
    CAN_COMM_PRINTF("Failed to allocate user_data for CAN RX instance %s\n",
                    config->name);
    daemon_unregister(new_instance->daemon->config.name);
    protocol_parser_deinit(new_instance->parser);
    __free(new_instance->parser);
    __free(new_instance->output_buffer);
    __free(new_instance);
    return NULL;
  }

  new_instance->initialized = true;
  new_instance->next = g_can_comm_rx_list;
  g_can_comm_rx_list = new_instance;

  CAN_COMM_PRINTF("CAN RX instance %s registered successfully\n", config->name);
  return new_instance;
}

/**
 * @brief 注册CAN发送实例
 * @param config 发送配置结构体指针
 * @return 成功返回发送实例指针，失败返回NULL
 */
can_comm_tx_t* can_comm_tx_register(const can_comm_tx_config_t* config) {
  if (!can_comm_validate_tx_config(config)) {
    return NULL;
  }

  can_comm_tx_t* existing = can_comm_get_tx_instance(config->name);
  if (existing != NULL) {
    CAN_COMM_PRINTF("CAN TX instance %s already exists\n", config->name);
    return existing;
  }

  can_comm_tx_t* new_instance = (can_comm_tx_t*)__malloc(sizeof(can_comm_tx_t));
  if (new_instance == NULL) {
    CAN_COMM_PRINTF("Failed to allocate memory for CAN TX instance %s\n",
                    config->name);
    return NULL;
  }
  __memset(new_instance, 0, sizeof(can_comm_tx_t));

  __memcpy(&new_instance->config, config, sizeof(can_comm_tx_config_t));

  new_instance->kfifo = kfifo_alloc(
      (config->data_len + CAN_COMM_PROTOCOL_LEN) * CAN_COMM_FIFO_MULTIPLIER,
      NULL);
  if (new_instance->kfifo == NULL) {
    CAN_COMM_PRINTF("Failed to allocate kfifo for CAN TX instance %s\n",
                    config->name);
    __free(new_instance);
    return NULL;
  }

  new_instance->tx_data = (uint8_t*)__malloc(config->data_len);
  if (new_instance->tx_data == NULL) {
    CAN_COMM_PRINTF("Failed to allocate tx_data for CAN TX instance %s\n",
                    config->name);
    kfifo_free(new_instance->kfifo);
    __free(new_instance);
    return NULL;
  }

  new_instance->initialized = true;
  new_instance->next = g_can_comm_tx_list;
  g_can_comm_tx_list = new_instance;

  CAN_COMM_PRINTF("CAN TX instance %s registered successfully\n", config->name);
  return new_instance;
}

/**
 * @brief 注销CAN接收实例
 * @param instance 接收实例指针
 */
void can_comm_rx_unregister(can_comm_rx_t* instance) {
  if (instance == NULL) {
    return;
  }

  can_comm_rx_t** current = &g_can_comm_rx_list;
  while (*current != NULL) {
    if (*current == instance) {
      *current = instance->next;

      if (instance->parser != NULL) {
        protocol_parser_deinit(instance->parser);
        __free(instance->parser);
      }
      if (instance->daemon != NULL) {
        daemon_unregister(instance->daemon->config.name);
      }
      if (instance->output_buffer != NULL) {
        __free(instance->output_buffer);
      }
      if (instance->user_data != NULL) {
        __free(instance->user_data);
      }
      __free(instance);

      CAN_COMM_PRINTF("CAN RX instance unregistered successfully\n");
      return;
    }
    current = &(*current)->next;
  }
}

/**
 * @brief 注销CAN发送实例
 * @param instance 发送实例指针
 */
void can_comm_tx_unregister(can_comm_tx_t* instance) {
  if (instance == NULL) {
    return;
  }

  can_comm_tx_t** current = &g_can_comm_tx_list;
  while (*current != NULL) {
    if (*current == instance) {
      *current = instance->next;

      if (instance->kfifo != NULL) {
        kfifo_free(instance->kfifo);
      }
      if (instance->tx_data != NULL) {
        __free(instance->tx_data);
      }
      __free(instance);

      CAN_COMM_PRINTF("CAN TX instance unregistered successfully\n");
      return;
    }
    current = &(*current)->next;
  }
}

/**
 * @brief 注销所有CAN实例
 */
void can_comm_unregister_all(void) {
  while (g_can_comm_rx_list != NULL) {
    can_comm_rx_unregister(g_can_comm_rx_list);
  }
  while (g_can_comm_tx_list != NULL) {
    can_comm_tx_unregister(g_can_comm_tx_list);
  }
  CAN_COMM_PRINTF("All CAN instances unregistered\n");
}

/**
 * @brief 初始化CAN通信模块
 */
void can_comm_init(void) {
  g_can_comm_rx_list = NULL;
  g_can_comm_tx_list = NULL;
  __memset(&g_can_stats, 0, sizeof(can_comm_stats_t));
  CAN_COMM_PRINTF("CAN communication module initialized\n");
}

/**
 * @brief 根据名称获取CAN发送实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_tx_t* can_comm_get_tx_instance(const char* name) {
  if (name == NULL) {
    return NULL;
  }
  can_comm_tx_t* current = g_can_comm_tx_list;
  while (current != NULL) {
    if (__strcmp(current->config.name, name) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief 根据名称获取CAN接收实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_rx_t* can_comm_get_rx_instance(const char* name) {
  if (name == NULL) {
    return NULL;
  }
  can_comm_rx_t* current = g_can_comm_rx_list;
  while (current != NULL) {
    if (__strcmp(current->config.name, name) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief 获取CAN接收实例数量
 * @return 接收实例数量
 */
uint32_t can_comm_get_rx_count(void) {
  uint32_t count = 0;
  const can_comm_rx_t* current = g_can_comm_rx_list;
  while (current != NULL) {
    count++;
    current = current->next;
  }
  return count;
}

/**
 * @brief 获取CAN发送实例数量
 * @return 发送实例数量
 */
uint32_t can_comm_get_tx_count(void) {
  uint32_t count = 0;
  const can_comm_tx_t* current = g_can_comm_tx_list;
  while (current != NULL) {
    count++;
    current = current->next;
  }
  return count;
}

/**
 * @brief 绑定接收数据缓冲区
 * @param rx 接收实例指针
 * @return 绑定的数据缓冲区指针
 */
void* can_comm_rx_bind_data(can_comm_rx_t* rx) {
  if (rx == NULL || rx->user_data == NULL) {
    return NULL;
  }
  return rx->user_data;
}

/**
 * @brief 绑定发送数据缓冲区
 * @param tx 发送实例指针
 * @return 绑定的数据缓冲区指针
 */
void* can_comm_tx_bind_data(can_comm_tx_t* tx) {
  if (tx == NULL || tx->tx_data == NULL) {
    return NULL;
  }
  tx->tx_ready = 1;
  return tx->tx_data;
}

/**
 * @brief 处理接收数据
 * @note 使用protocol_parser进行协议解析
 */
void can_comm_process_rx(void) {
  can_comm_rx_t* current = g_can_comm_rx_list;

  while (current != NULL) {
    if (current->parser == NULL || !current->initialized) {
      current = current->next;
      continue;
    }

    uint16_t frame_len = 0;
    uint8_t* frame_data = NULL;

    protocol_parser_error_t err =
        protocol_parser_parse(current->parser, &frame_len, &frame_data);

    if (err == PROTOCOL_PARSER_OK && frame_data != NULL && frame_len > 0) {
      can_comm_protocol_head_t* head = (can_comm_protocol_head_t*)frame_data;
      uint8_t* payload = frame_data + sizeof(can_comm_protocol_head_t);
      uint16_t payload_len = head->data_length;

      if (payload_len <= current->config.data_len) {
        __memcpy(current->user_data, payload, payload_len);

        current->last_sequence = head->sequence;
        daemon_reload(current->daemon);

        if (current->config.rx_cb != NULL) {
          current->config.rx_cb(current->user_data, payload, payload_len);
        }
      }
    } else if (err == PROTOCOL_PARSER_ERROR_CHECKSUM) {
      g_can_stats.crc_errors++;
    } else if (err == PROTOCOL_PARSER_ERROR_HEADER_MISMATCH ||
               err == PROTOCOL_PARSER_ERROR_FOOTER_MISMATCH) {
      g_can_stats.protocol_errors++;
    }

    current = current->next;
  }
}

/**
 * @brief 打包发送数据
 */
void can_comm_package_tx(void) {
  can_comm_tx_t* current = g_can_comm_tx_list;
  uint8_t temp_buffer[CAN_COMM_MAX_DATA_LEN + CAN_COMM_PROTOCOL_LEN];

  while (current != NULL) {
    if (!current->initialized) {
      current = current->next;
      continue;
    }

    uint8_t data_len = current->config.data_len;
    uint16_t package_total_len = data_len + CAN_COMM_PROTOCOL_LEN;

    if (current->kfifo == NULL) {
      current = current->next;
      continue;
    }

    if (kfifo_status(current->kfifo) == KFIFO_FULL ||
        (current->kfifo->size - kfifo_len(current->kfifo)) <
            package_total_len) {
      g_can_stats.tx_errors++;
      current = current->next;
      continue;
    }

    uint8_t has_new_data = 0;

    if (current->tx_data != NULL && current->tx_ready) {
      current->tx_ready = 0;
      __memcpy(temp_buffer + sizeof(can_comm_protocol_head_t), current->tx_data,
               data_len);
      has_new_data = 1;
    }

    if (has_new_data) {
      can_comm_protocol_head_t* p_head = (can_comm_protocol_head_t*)temp_buffer;
      p_head->start = 'S';
      p_head->data_length = data_len;
      p_head->sequence = current->sequence++;

      uint8_t* p_crc =
          &temp_buffer[sizeof(can_comm_protocol_head_t) + data_len];
      *p_crc = get_CRC8_check_sum(
          &temp_buffer[sizeof(can_comm_protocol_head_t)], data_len, 0);
      *(p_crc + 1) = 'E';

      kfifo_put(current->kfifo, temp_buffer, package_total_len);
    }

    current = current->next;
  }
}

/**
 * @brief 刷新发送缓冲区
 */
void can_comm_flush_tx(void) {
  const can_comm_tx_t* current = g_can_comm_tx_list;
  const uint8_t send_one_frame_len =
      can_comm_floor_to_std(CAN_COMM_ONE_FRAME_SEND_LEN);

  while (current != NULL) {
    if (current->kfifo == NULL) {
      current = current->next;
      continue;
    }

    const uint8_t one_packet_max_len =
        can_comm_floor_to_std(current->config.data_len + CAN_COMM_PROTOCOL_LEN);
    uint8_t get_fifo_len = one_packet_max_len < send_one_frame_len
                               ? one_packet_max_len
                               : send_one_frame_len;

    while (kfifo_len(current->kfifo) > 0) {
      if (can_comm_get_send_level(current->config.instance) > 0) {
        static hal_fdcan_message_t message;
        const uint8_t get_len =
            kfifo_get(current->kfifo, message.data, get_fifo_len);
        message.id = current->config.can_id;
        message.data_length = get_len;

        if (can_comm_send(current->config.instance, &message)) {
          g_can_stats.total_tx_bytes += get_len;
        }
        g_can_stats.total_tx_frames++;
      } else {
        break;
      }
    }

    current = current->next;
  }
}

/**
 * @brief 获取CAN通信统计信息
 * @return 统计信息结构体指针
 */
can_comm_stats_t* can_comm_get_stats(void) { return &g_can_stats; }

/**
 * @brief 重置统计信息
 */
void can_comm_reset_stats(void) {
  __memset(&g_can_stats, 0, sizeof(can_comm_stats_t));
  CAN_COMM_PRINTF("CAN statistics reset\n");
}

/**
 * @brief 获取接收实例的序列号错误统计
 * @param rx_name 接收实例名称
 * @return 序列号错误计数
 */
uint32_t can_comm_get_sequence_errors(const char* rx_name) {
  const can_comm_rx_t* rx_instance = can_comm_get_rx_instance(rx_name);
  if (rx_instance == NULL) {
    return 0;
  }
  return rx_instance->sequence_errors;
}

/* 总线负载计算相关常量 */
#define CAN_BAUD_NOMINAL 1000000.0f
#define CAN_BAUD_DATA 1000000.0f
#define CAN_FD_OVERHEAD_ARB 22
#define CAN_FD_OVERHEAD_DATA 30
#define CAN_STUFFING_FACTOR 1.15f

/**
 * @brief 检查CAN总线负载
 * @return 总线负载率(0-100)
 */
uint8_t can_comm_get_bus_load(void) {
  static uint32_t last_time = 0;
  static uint32_t last_rx_bytes = 0;
  static uint32_t last_tx_bytes = 0;
  static uint32_t last_rx_frames = 0;
  static uint32_t last_tx_frames = 0;
  static uint8_t load = 0;

  const uint32_t current_time = millis();

  if (current_time > last_time && (current_time - last_time) >= 100) {
    float interval_us = (current_time - last_time) * 1000.0f;

    uint32_t delta_rx_bytes = g_can_stats.total_rx_bytes - last_rx_bytes;
    uint32_t delta_tx_bytes = g_can_stats.total_tx_bytes - last_tx_bytes;
    uint32_t delta_rx_frames = g_can_stats.total_rx_frames - last_rx_frames;
    uint32_t delta_tx_frames = g_can_stats.total_tx_frames - last_tx_frames;

    uint32_t delta_total_frames = delta_rx_frames + delta_tx_frames;
    uint32_t delta_total_bytes = delta_rx_bytes + delta_tx_bytes;

    float time_arb_us = (float)delta_total_frames * CAN_FD_OVERHEAD_ARB *
                        (1000000.0f / CAN_BAUD_NOMINAL);

    float bits_data_phase =
        (delta_total_bytes * 8) + (delta_total_frames * CAN_FD_OVERHEAD_DATA);
    float time_data_us = bits_data_phase * (1000000.0f / CAN_BAUD_DATA);

    float total_bus_time_us =
        (time_arb_us + time_data_us) * CAN_STUFFING_FACTOR;

    float load_percent = (total_bus_time_us / interval_us) * 100.0f;

    if (load_percent > 100.0f) {
      load_percent = 100.0f;
    }
    load = (uint8_t)load_percent;

    last_time = current_time;
    last_rx_bytes = g_can_stats.total_rx_bytes;
    last_tx_bytes = g_can_stats.total_tx_bytes;
    last_rx_frames = g_can_stats.total_rx_frames;
    last_tx_frames = g_can_stats.total_tx_frames;
  }

  return load;
}
