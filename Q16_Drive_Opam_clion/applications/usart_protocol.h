//
// Created by fubingyan on 25-4-6.
//

#ifndef __USART_PROTOCOL_H
#define __USART_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "public.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Q16.16 定点数类型定义
 * @note 在协议层使用int32_t,避免嵌入math.h依赖
 */
typedef int32_t q16_16_t;

/**
 * @brief 串口协议模块错误码枚举
 */
typedef enum __attribute__((packed)) {
  USART_PROTOCOL_OK = 0,              /**< 操作成功 */
  USART_PROTOCOL_ERROR_GENERAL,       /**< 通用错误 */
  USART_PROTOCOL_ERROR_INVALID_CMD,   /**< 无效命令 */
  USART_PROTOCOL_ERROR_INVALID_PARAM, /**< 无效参数 */
  USART_PROTOCOL_ERROR_DATA_LEN,      /**< 数据长度错误 */
  USART_PROTOCOL_ERROR_CHECKSUM,      /**< 校验和错误 */
  USART_PROTOCOL_ERROR_TIMEOUT,       /**< 超时错误 */
  USART_PROTOCOL_ERROR_STATE,         /**< 状态错误 */
  USART_PROTOCOL_ERROR_OVERCURRENT,   /**< 过流错误 */
  USART_PROTOCOL_ERROR_OVERTEMP,      /**< 过温错误 */
  USART_PROTOCOL_ERROR_OVERVOLTAGE,   /**< 过压错误 */
  USART_PROTOCOL_ERROR_UNDERVOLTAGE,  /**< 欠压错误 */
  USART_PROTOCOL_ERROR_ENCODER,       /**< 编码器错误 */
  USART_PROTOCOL_ERROR_CALIB,         /**< 校准错误 */
  USART_PROTOCOL_ERROR_NOT_READY,     /**< 未就绪 */
  USART_PROTOCOL_ERROR_BUSY,          /**< 忙 */
  USART_PROTOCOL_ERROR_NULL_PTR,      /**< 空指针错误 */
  USART_PROTOCOL_ERROR_UNINITIALIZED, /**< 未初始化 */
} usart_protocol_error_t;

/**
 * @brief 错误类枚举
 */
typedef enum __attribute__((packed)) {
  USART_PROTOCOL_ERR_CLASS_NONE = 0x00U,     /**< 无错误类 */
  USART_PROTOCOL_ERR_CLASS_COMM = 0x10U,     /**< 通信错误类 */
  USART_PROTOCOL_ERR_CLASS_PROTOCOL = 0x20U, /**< 协议错误类 */
  USART_PROTOCOL_ERR_CLASS_DEVICE = 0x30U,   /**< 设备错误类 */
  USART_PROTOCOL_ERR_CLASS_APP = 0x40U       /**< 应用错误类 */
} usart_protocol_error_class_t;

/**
 * @brief 校准状态枚举
 */
typedef enum __attribute__((packed)) {
  USART_PROTOCOL_CALIB_STATUS_IDLE = 0x00U,     /**< 空闲 */
  USART_PROTOCOL_CALIB_STATUS_RUNNING = 0x01U,  /**< 校准中 */
  USART_PROTOCOL_CALIB_STATUS_FORWARD = 0x02U,  /**< 正向校准 */
  USART_PROTOCOL_CALIB_STATUS_REVERSE = 0x03U,  /**< 反向校准 */
  USART_PROTOCOL_CALIB_STATUS_COMPLETE = 0x04U, /**< 校准完成 */
  USART_PROTOCOL_CALIB_STATUS_FAILED = 0x05U    /**< 校准失败 */
} usart_protocol_calib_status_t;

/**
 * @brief 参数类型枚举
 */
typedef enum __attribute__((packed)) {
  USART_PROTOCOL_PARAM_PID_CURRENT = 0x01U,       /**< 电流环PID */
  USART_PROTOCOL_PARAM_PID_VELOCITY = 0x02U,      /**< 速度环PID */
  USART_PROTOCOL_PARAM_PID_POSITION = 0x03U,      /**< 位置环PID */
  USART_PROTOCOL_PARAM_MOTOR_CONFIG = 0x04U,      /**< 电机配置 */
  USART_PROTOCOL_PARAM_ENCODER_CONFIG = 0x05U,    /**< 编码器配置 */
  USART_PROTOCOL_PARAM_PROTECTION_CONFIG = 0x06U, /**< 保护配置 */
  USART_PROTOCOL_PARAM_STREAM_CONFIG = 0x07U      /**< 流配置 */
} usart_protocol_param_type_t;

/**
 * @brief 数据流类型枚举
 */
typedef enum __attribute__((packed)) {
  USART_PROTOCOL_STREAM_ALL = 0x00U,      /**< 全部数据 */
  USART_PROTOCOL_STREAM_CURRENT = 0x01U,  /**< 电流数据 */
  USART_PROTOCOL_STREAM_VELOCITY = 0x02U, /**< 速度数据 */
  USART_PROTOCOL_STREAM_POSITION = 0x03U, /**< 位置数据 */
  USART_PROTOCOL_STREAM_ERROR = 0x04U     /**< 错误数据 */
} usart_protocol_stream_type_t;

/**
 * @brief 版本信息结构体 (6 bytes)
 * @note 使用packed对齐,禁止优化对齐导致的大小变化
 */
typedef struct __attribute__((packed)) {
  uint8_t protocol_ver; /**< 协议版本 */
  uint8_t major_ver;    /**< 主版本号 */
  uint8_t minor_ver;    /**< 次版本号 */
  uint8_t patch_ver;    /**< 补丁版本号 */
  uint8_t hardware_ver; /**< 硬件版本 */
  uint8_t reserve;      /**< 保留 */
} usart_protocol_version_info_t;

/**
 * @brief 状态数据结构体 (16 bytes)
 * @note 使用packed对齐
 */
typedef struct __attribute__((packed)) {
  uint8_t state;      /**< 状态 */
  uint8_t error_code; /**< 错误码 */
  int16_t id_current; /**< D轴电流 */
  int16_t iq_current; /**< Q轴电流 */
  int16_t velocity;   /**< 速度 */
  int32_t position;   /**< 位置 */
  uint16_t voltage;   /**< 电压 */
  int8_t temperature; /**< 温度 */
  uint8_t reserve;    /**< 保留(对齐) */
} usart_protocol_status_data_t;

/**
 * @brief 校准状态数据结构体 (3 bytes)
 * @note 使用packed对齐
 */
typedef struct __attribute__((packed)) {
  uint8_t status;   /**< 校准状态 */
  uint8_t progress; /**< 进度 */
  int8_t direction; /**< 方向 */
} usart_protocol_calib_status_data_t;

/**
 * @brief PID参数结构体 (16 bytes)
 * @note 使用Q16定点数替代float,符合项目规范(ISR中禁止使用float)
 * @note 使用packed对齐
 */
typedef struct __attribute__((packed)) {
  q16_16_t kp;           /**< 比例增益 Q16格式 */
  q16_16_t ki;           /**< 积分增益 Q16格式 */
  q16_16_t kd;           /**< 微分增益 Q16格式 */
  q16_16_t output_limit; /**< 输出限幅 Q16格式 */
} usart_protocol_pid_param_t;

/**
 * @brief 流数据结构体 (18 bytes)
 * @note 使用packed对齐
 */
typedef struct __attribute__((packed)) {
  uint32_t timestamp;  /**< 时间戳 */
  int16_t id_current;  /**< D轴电流 */
  int16_t iq_current;  /**< Q轴电流 */
  int16_t velocity;    /**< 速度 */
  int32_t position;    /**< 位置 */
  uint16_t elec_angle; /**< 电角度 */
  uint16_t reserve;    /**< 保留(对齐) */
} usart_protocol_stream_data_t;

/**
 * @brief 流配置结构体
 * @note 使用packed对齐,与协议其他结构体保持一致
 */
typedef struct __attribute__((packed)) {
  uint8_t enable;       /**< 使能标志 */
  uint8_t stream_type;  /**< 流类型 */
  uint16_t interval_ms; /**< 间隔时间(毫秒) */
} usart_protocol_stream_config_t;

/**
 * @brief 串口协议模块配置结构体
 */
typedef struct __attribute__((packed)) {
  const char* name;      /**< 模块名称 */
  void* uart_ctx;        /**< UART上下文指针 */
  uint8_t uart_instance; /**< UART实例号 */
} usart_protocol_config_t;

/**
 * @brief 串口协议模块上下文结构体前向声明
 */
typedef struct usart_protocol_context usart_protocol_context_t;

/**
 * @brief 串口协议模块上下文结构体
 * @note 包含所有运行时状态和缓冲区，支持多实例
 */
struct usart_protocol_context {
  usart_protocol_config_t config;            /**< 配置参数 */
  usart_protocol_stream_config_t stream_cfg; /**< 流配置 */
  protocol_parser_context_t parser_ctx;      /**< 协议解析器上下文 */
  uint8_t output_buffer[280U];               /**< 输出缓冲区 */
  uint8_t parser_output_buffer[128U];        /**< 解析器输出缓冲区 */
  uint8_t response_buffer[256U];             /**< 响应数据缓冲区 */
  uint32_t stream_last_tick;                 /**< 流上次发送时间 */
  bool initialized;                          /**< 初始化标志 */
};

// 命令处理表
typedef usart_protocol_error_t (*protocol_cmd_handler_t)(uint8_t func_code,
                                                         uint8_t* data,
                                                         uint16_t len,
                                                         uint8_t* response,
                                                         uint16_t* resp_len);

/**
 * @brief 命令处理表结构体
 */
typedef struct {
  uint8_t func_code;
  protocol_cmd_handler_t handler;
} protocol_cmd_entry_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/**
 * @brief 协议常量定义
 */
#define S7_FRAME_HEAD_1 0x68U
#define S7_FRAME_HEAD_2 0x68U
#define S7_PROTOCOL_ID 0x32U
#define S7_FRAME_TAIL 0x16U

/**
 * @brief 消息类型定义
 */
#define MSG_JOB 0x01U
#define MSG_ACK 0x02U
#define MSG_RESPONSE 0x03U
#define MSG_ERROR 0x07U

/**
 * @brief 功能码定义 - 系统功能 (0x00-0x0F)
 */
#define FUNC_PING 0x01U
#define FUNC_GET_VERSION 0x02U
#define FUNC_RESET 0x03U
#define FUNC_GET_DEVICE_INFO 0x04U

/**
 * @brief 功能码定义 - 状态功能 (0x10-0x1F)
 */
#define FUNC_GET_STATUS 0x10U
#define FUNC_GET_STATE 0x11U
#define FUNC_GET_ERROR 0x12U

/**
 * @brief 功能码定义 - 控制功能 (0x20-0x2F)
 */
#define FUNC_SET_CURRENT 0x20U
#define FUNC_SET_VELOCITY 0x21U
#define FUNC_SET_POSITION 0x22U
#define FUNC_STOP 0x23U
#define FUNC_START 0x24U
#define FUNC_BRAKE 0x25U

/**
 * @brief 功能码定义 - 校准功能 (0x30-0x3F)
 */
#define FUNC_START_CALIB 0x30U
#define FUNC_GET_CALIB_STATUS 0x31U
#define FUNC_GET_CALIB_DATA 0x32U
#define FUNC_SET_CALIB_DATA 0x33U
#define FUNC_CLEAR_CALIB 0x34U
#define FUNC_SAVE_CALIB 0x35U

/**
 * @brief 功能码定义 - 参数功能 (0x40-0x4F)
 */
#define FUNC_GET_PARAMS 0x40U
#define FUNC_SET_PARAMS 0x41U
#define FUNC_SAVE_PARAMS 0x42U
#define FUNC_LOAD_PARAMS 0x43U

/**
 * @brief 功能码定义 - 数据流功能 (0x50-0x5F)
 */
#define FUNC_START_STREAM 0x50U
#define FUNC_STOP_STREAM 0x51U
#define FUNC_SET_STREAM_CFG 0x52U

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief 初始化串口协议模块
 * @param ctx 串口协议模块上下文指针
 * @param config 配置结构体指针
 * @return 操作结果错误码
 */
usart_protocol_error_t usart_protocol_init(
    usart_protocol_context_t* ctx, const usart_protocol_config_t* config);

/**
 * @brief 反初始化串口协议模块
 * @param ctx 串口协议模块上下文指针
 */
void usart_protocol_deinit(usart_protocol_context_t* ctx);

/**
 * @brief 检查模块是否已初始化
 * @param ctx 串口协议模块上下文指针
 * @return true表示已初始化，false表示未初始化
 */
bool usart_protocol_is_initialized(const usart_protocol_context_t* ctx);

/**
 * @brief 处理接收到的协议帧
 * @param ctx 串口协议模块上下文指针
 * @param data 接收到的数据指针,不能为NULL
 * @param len 数据长度
 * @return 操作结果错误码
 */
usart_protocol_error_t usart_protocol_process_frame(
    usart_protocol_context_t* ctx, uint8_t* data, uint16_t len);

/**
 * @brief 流数据任务处理函数
 * @param ctx 串口协议模块上下文指针
 * @note 须在主循环或专用任务中周期性调用
 */
void usart_protocol_stream_task(usart_protocol_context_t* ctx);

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
                                  const uint8_t* data, uint16_t len);

/**
 * @brief 发送OK响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 */
void usart_protocol_send_ok(usart_protocol_context_t* ctx, uint8_t func_code);

/**
 * @brief 发送错误响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 * @param error_code 错误码
 * @param error_class 错误类
 */
void usart_protocol_send_error(usart_protocol_context_t* ctx, uint8_t func_code,
                               uint8_t error_code, uint8_t error_class);

/**
 * @brief 发送ACK响应
 * @param ctx 串口协议模块上下文指针
 * @param func_code 功能码
 */
void usart_protocol_send_ack(usart_protocol_context_t* ctx, uint8_t func_code);

/**
 * @brief 喂入接收数据到协议解析器
 * @param ctx 串口协议模块上下文指针
 * @param data 接收到的数据指针
 * @param len 数据长度
 */
void usart_protocol_feed(usart_protocol_context_t* ctx, const uint8_t* data,
                         uint16_t len);

/**
 * @brief 协议解析任务处理函数
 * @param ctx 串口协议模块上下文指针
 * @note 须在主循环中周期性调用，自动解析并处理帧
 */
void usart_protocol_parse_task(usart_protocol_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* __USART_PROTOCOL_H */
