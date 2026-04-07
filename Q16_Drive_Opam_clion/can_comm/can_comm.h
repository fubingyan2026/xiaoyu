//
// Created by fubingyan on 25-4-5.
//

#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "daemon.h"
#include "hal_fdcan.h"
#include "kfifo/kfifo.h"
#include "protocol_parser.h"

/* Exported constants --------------------------------------------------------*/

#define CAN_COMM_ONE_FRAME_SEND_LEN 24  /**< 一帧发送长度 */
#define CAN_COMM_MAX_DATA_LEN 128       /**< 最大数据长度 */
#define CAN_COMM_FIFO_MULTIPLIER 2      /**< FIFO缓冲区倍数 */
#define CAN_COMM_DEFAULT_OFFLINE_MS 100 /**< 默认离线超时时间(ms) */
#define CAN_COMM_MIN_OFFLINE_MS 5       /**< 最小离线超时时间(ms) */
#define CAN_COMM_MAX_TX_BATCH_FRAMES 4  /**< 最大批量发送帧数 */
#define CAN_COMM_MAX_RETRY_COUNT 1      /**< 最大重试次数 */
#define CAN_COMM_RETRY_DELAY_MS 2       /**< 重试延迟(ms) */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief CAN通信错误码枚举
 */
typedef enum {
  CAN_COMM_OK = 0,                   /**< 操作成功 */
  CAN_COMM_ERROR_NULL_PTR,           /**< 空指针错误 */
  CAN_COMM_ERROR_INVALID_PARAM,      /**< 无效参数 */
  CAN_COMM_ERROR_UNINITIALIZED,      /**< 未初始化 */
  CAN_COMM_ERROR_MEMORY_ALLOC,       /**< 内存分配失败 */
  CAN_COMM_ERROR_INSTANCE_EXISTS,    /**< 实例已存在 */
  CAN_COMM_ERROR_INSTANCE_NOT_FOUND, /**< 实例未找到 */
  CAN_COMM_ERROR_BUFFER_OVERFLOW,    /**< 缓冲区溢出 */
  CAN_COMM_ERROR_PARSE_FAILED,       /**< 解析失败 */
  CAN_COMM_ERROR_CRC_FAILED,         /**< CRC校验失败 */
  CAN_COMM_ERROR_SEQUENCE_ERROR,     /**< 序列号错误 */
  CAN_COMM_ERROR_GENERIC,            /**< 通用错误 */
} can_comm_error_t;

/**
 * @brief CAN通信协议帧头结构体
 */
#pragma pack(push, 1)
typedef struct {
  char start;          /**< 'S' - 起始字节 */
  uint8_t data_length; /**< 数据长度 */
  uint8_t sequence;    /**< 序列号，防重放 */
} can_comm_protocol_head_t;

/**
 * @brief CAN通信协议帧尾结构体
 */
typedef struct {
  uint8_t crc; /**< CRC8校验 */
  char end;    /**< 'E' - 结束字节 */
} can_comm_protocol_end_t;
#pragma pack(pop)

/**
 * @brief CAN通信统计信息结构体
 */
typedef struct {
  uint32_t total_rx_bytes;  /**< 总接收字节数 */
  uint32_t total_tx_bytes;  /**< 总发送字节数 */
  uint32_t rx_errors;       /**< 接收错误计数 */
  uint32_t tx_errors;       /**< 发送错误计数 */
  uint32_t crc_errors;      /**< CRC校验错误计数 */
  uint32_t timeout_errors;  /**< 超时错误计数 */
  uint32_t protocol_errors; /**< 协议错误计数 */
  uint32_t total_rx_frames; /**< 总接收帧数 */
  uint32_t total_tx_frames; /**< 总发送帧数 */
} can_comm_stats_t;

/**
 * @brief 接收完成回调函数类型
 * @param user_data 用户数据指针
 * @param data 接收到的数据
 * @param len 数据长度
 */
typedef void (*can_comm_rx_callback_t)(void* user_data, const uint8_t* data,
                                       uint16_t len);

/**
 * @brief CAN接收配置结构体
 */
typedef struct {
  const char* name;               /**< 配置名称 */
  hal_fdcan_instance_t instance;  /**< CAN实例标识 */
  uint32_t can_id;                /**< 接收消息ID */
  uint16_t data_len;              /**< 接收数据长度 */
  uint16_t offline_ms;            /**< 离线超时时间(ms) */
  uint8_t priority;               /**< 接收优先级 0-最高, 255-最低 */
  uint8_t daemon_error_code;      /**< 守护进程错误码 */
  daemon_offline_cb_t offline_cb; /**< 离线回调函数 */
  can_comm_rx_callback_t rx_cb;   /**< 接收完成回调 */
} can_comm_rx_config_t;

/**
 * @brief CAN发送配置结构体
 */
typedef struct {
  const char* name;              /**< 配置名称 */
  hal_fdcan_instance_t instance; /**< CAN实例标识 */
  uint32_t can_id;               /**< 发送消息ID */
  uint16_t data_len;             /**< 发送数据长度 */
  uint8_t priority;              /**< 发送优先级 0-最高, 255-最低 */
} can_comm_tx_config_t;

/**
 * @brief CAN接收上下文结构体前向声明
 */
typedef struct can_comm_rx can_comm_rx_t;

/**
 * @brief CAN接收上下文结构体
 */
struct can_comm_rx {
  can_comm_rx_config_t config;       /**< 配置参数 */
  protocol_parser_context_t* parser; /**< 协议解析器实例 */
  daemon_context_t* daemon;          /**< 守护进程实例 */
  uint8_t* output_buffer;            /**< 输出缓冲区 */
  void* user_data;                   /**< 用户数据绑定指针 */
  uint8_t last_sequence;             /**< 最后接收的序列号 */
  uint32_t sequence_errors;          /**< 序列号错误计数 */
  bool initialized;                  /**< 初始化标志 */
  can_comm_rx_t* next;               /**< 链表指针 */
};

/**
 * @brief CAN发送上下文结构体前向声明
 */
typedef struct can_comm_tx can_comm_tx_t;

/**
 * @brief CAN发送上下文结构体
 */
struct can_comm_tx {
  can_comm_tx_config_t config; /**< 配置参数 */
  kfifo_t* kfifo;              /**< 发送缓冲区 */
  uint8_t* tx_data;            /**< 发送数据指针 */
  uint8_t sequence;            /**< 序列号计数器 */
  bool tx_ready;               /**< 发送就绪标志 */
  bool initialized;            /**< 初始化标志 */
  can_comm_tx_t* next;         /**< 链表指针 */
};

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* 实例管理 */
/**
 * @brief 注册CAN接收实例
 * @param config 接收配置结构体指针
 * @return 成功返回接收实例指针，失败返回NULL
 */
can_comm_rx_t* can_comm_rx_register(const can_comm_rx_config_t* config);

/**
 * @brief 注册CAN发送实例
 * @param config 发送配置结构体指针
 * @return 成功返回发送实例指针，失败返回NULL
 */
can_comm_tx_t* can_comm_tx_register(const can_comm_tx_config_t* config);

/**
 * @brief 注销CAN接收实例
 * @param instance 接收实例指针
 */
void can_comm_rx_unregister(can_comm_rx_t* instance);

/**
 * @brief 注销CAN发送实例
 * @param instance 发送实例指针
 */
void can_comm_tx_unregister(can_comm_tx_t* instance);

/**
 * @brief 注销所有CAN实例
 */
void can_comm_unregister_all(void);

/**
 * @brief 初始化CAN通信模块
 */
void can_comm_init(void);

/* 实例查询 */
/**
 * @brief 根据名称获取CAN发送实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_tx_t* can_comm_get_tx_instance(const char* name);

/**
 * @brief 根据名称获取CAN接收实例
 * @param name 实例名称
 * @return 找到返回实例指针，未找到返回NULL
 */
can_comm_rx_t* can_comm_get_rx_instance(const char* name);

/**
 * @brief 获取CAN接收实例数量
 * @return 接收实例数量
 */
uint32_t can_comm_get_rx_count(void);

/**
 * @brief 获取CAN发送实例数量
 * @return 发送实例数量
 */
uint32_t can_comm_get_tx_count(void);

/* 数据绑定 */
/**
 * @brief 绑定接收数据缓冲区
 * @param rx 接收实例指针
 * @return 绑定的数据缓冲区指针
 */
void* can_comm_rx_bind_data(can_comm_rx_t* rx);

/**
 * @brief 绑定发送数据缓冲区
 * @param tx 发送实例指针
 * @return 绑定的数据缓冲区指针
 */
void* can_comm_tx_bind_data(can_comm_tx_t* tx);

/* 数据处理 */
/**
 * @brief 处理接收数据
 * @note 需要在主循环中周期性调用
 */
void can_comm_process_rx(void);

/**
 * @brief 打包发送数据
 * @note 需要在主循环中周期性调用
 */
void can_comm_package_tx(void);

/**
 * @brief 刷新发送缓冲区
 * @note 需要在主循环中周期性调用
 */
void can_comm_flush_tx(void);

/* 统计信息 */
/**
 * @brief 获取CAN通信统计信息
 * @return 统计信息结构体指针
 */
can_comm_stats_t* can_comm_get_stats(void);

/**
 * @brief 重置统计信息
 */
void can_comm_reset_stats(void);

/**
 * @brief 检查CAN总线负载
 * @return 总线负载率(0-100)
 */
uint8_t can_comm_get_bus_load(void);

/**
 * @brief 获取接收实例的序列号错误统计
 * @param rx_name 接收实例名称
 * @return 序列号错误计数
 */
uint32_t can_comm_get_sequence_errors(const char* rx_name);

/* 硬件抽象层接口 */
/**
 * @brief 获取接收FIFO数据量
 * @param instance CAN实例
 * @param fifo_address FIFO地址
 * @return 数据量
 */
uint32_t can_comm_get_receive_level(hal_fdcan_instance_t instance,
                                    uint8_t fifo_address);

/**
 * @brief 获取发送FIFO空闲量
 * @param instance CAN实例
 * @return 空闲量
 */
uint32_t can_comm_get_send_level(hal_fdcan_instance_t instance);

/**
 * @brief 接收CAN消息
 * @param instance CAN实例
 * @param message 消息结构体指针
 * @return 成功返回true，失败返回false
 */
bool can_comm_receive(hal_fdcan_instance_t instance,
                      hal_fdcan_message_t* message);

/**
 * @brief 发送CAN消息
 * @param instance CAN实例
 * @param message 消息结构体指针
 * @return 成功返回true，失败返回false
 */
bool can_comm_send(hal_fdcan_instance_t instance,
                   const hal_fdcan_message_t* message);

/* 全局变量声明 */
extern can_comm_rx_t* g_can_comm_rx_list;
extern can_comm_tx_t* g_can_comm_tx_list;
extern can_comm_stats_t g_can_stats;

#ifdef __cplusplus
}
#endif

#endif /* __CAN_COMM_H */
