/**
 * @file    usart_protocol.h
 * @brief   UART S7协议处理器 - 西门子S7风格协议处理
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
 * | 2.1   | 2026-03-03 | FOC团队| 按规范重构,添加中文注释 |
 * | 2.0   | 2026-03-03 | FOC团队| 初始版本               |
 */

#ifndef USART_PROTOCOL_H
#define USART_PROTOCOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

    /*============================================================================
     * Q16.16 定点数类型定义
     * @note 在协议层使用int32_t,避免嵌入math.h依赖
     *============================================================================*/
    typedef int32_t q16_16_t;

/*============================================================================
 * 协议常量定义
 *============================================================================*/
#define S7_FRAME_HEAD_1 0x68U
#define S7_FRAME_HEAD_2 0x68U
#define S7_PROTOCOL_ID 0x32U
#define S7_FRAME_TAIL 0x16U

/*============================================================================
 * 消息类型定义
 *============================================================================*/
#define MSG_JOB 0x01U
#define MSG_ACK 0x02U
#define MSG_RESPONSE 0x03U
#define MSG_ERROR 0x07U

/*============================================================================
 * 功能码定义 - 系统功能 (0x00-0x0F)
 *============================================================================*/
#define FUNC_PING 0x01U
#define FUNC_GET_VERSION 0x02U
#define FUNC_RESET 0x03U
#define FUNC_GET_DEVICE_INFO 0x04U

/*============================================================================
 * 功能码定义 - 状态功能 (0x10-0x1F)
 *============================================================================*/
#define FUNC_GET_STATUS 0x10U
#define FUNC_GET_STATE 0x11U
#define FUNC_GET_ERROR 0x12U

/*============================================================================
 * 功能码定义 - 控制功能 (0x20-0x2F)
 *============================================================================*/
#define FUNC_SET_CURRENT 0x20U
#define FUNC_SET_VELOCITY 0x21U
#define FUNC_SET_POSITION 0x22U
#define FUNC_STOP 0x23U
#define FUNC_START 0x24U
#define FUNC_BRAKE 0x25U

/*============================================================================
 * 功能码定义 - 校准功能 (0x30-0x3F)
 *============================================================================*/
#define FUNC_START_CALIB 0x30U
#define FUNC_GET_CALIB_STATUS 0x31U
#define FUNC_GET_CALIB_DATA 0x32U
#define FUNC_SET_CALIB_DATA 0x33U
#define FUNC_CLEAR_CALIB 0x34U
#define FUNC_SAVE_CALIB 0x35U

/*============================================================================
 * 功能码定义 - 参数功能 (0x40-0x4F)
 *============================================================================*/
#define FUNC_GET_PARAMS 0x40U
#define FUNC_SET_PARAMS 0x41U
#define FUNC_SAVE_PARAMS 0x42U
#define FUNC_LOAD_PARAMS 0x43U

/*============================================================================
 * 功能码定义 - 数据流功能 (0x50-0x5F)
 *============================================================================*/
#define FUNC_START_STREAM 0x50U
#define FUNC_STOP_STREAM 0x51U
#define FUNC_SET_STREAM_CFG 0x52U

    /*============================================================================
     * 错误码定义
     * @note 使用枚举,显式指定底层类型为uint8_t
     *============================================================================*/
    typedef enum
    {
        ERR_NONE = 0x00U,          /**< 无错误 */
        ERR_GENERAL = 0x01U,       /**< 通用错误 */
        ERR_INVALID_CMD = 0x02U,   /**< 无效命令 */
        ERR_INVALID_PARAM = 0x03U, /**< 无效参数 */
        ERR_DATA_LEN = 0x04U,      /**< 数据长度错误 */
        ERR_CHECKSUM = 0x05U,      /**< 校验和错误 */
        ERR_TIMEOUT = 0x06U,       /**< 超时错误 */
        ERR_STATE = 0x07U,         /**< 状态错误 */
        ERR_OVERCURRENT = 0x08U,   /**< 过流错误 */
        ERR_OVERTEMP = 0x09U,      /**< 过温错误 */
        ERR_OVERVOLTAGE = 0x0AU,   /**< 过压错误 */
        ERR_UNDERVOLTAGE = 0x0BU,  /**< 欠压错误 */
        ERR_ENCODER = 0x0CU,       /**< 编码器错误 */
        ERR_CALIB = 0x0DU,         /**< 校准错误 */
        ERR_NOT_READY = 0x0EU,     /**< 未就绪 */
        ERR_BUSY = 0x0FU           /**< 忙 */
    } ProtocolError_e;

    /*============================================================================
     * 错误类定义
     *============================================================================*/
    typedef enum
    {
        ERR_CLASS_NONE = 0x00U,     /**< 无错误类 */
        ERR_CLASS_COMM = 0x10U,     /**< 通信错误类 */
        ERR_CLASS_PROTOCOL = 0x20U, /**< 协议错误类 */
        ERR_CLASS_DEVICE = 0x30U,   /**< 设备错误类 */
        ERR_CLASS_APP = 0x40U       /**< 应用错误类 */
    } ProtocolErrorClass_e;

    /*============================================================================
     * 校准状态定义
     *============================================================================*/
    typedef enum
    {
        CALIB_STATUS_IDLE = 0x00U,     /**< 空闲 */
        CALIB_STATUS_RUNNING = 0x01U,  /**< 校准中 */
        CALIB_STATUS_FORWARD = 0x02U,  /**< 正向校准 */
        CALIB_STATUS_REVERSE = 0x03U,  /**< 反向校准 */
        CALIB_STATUS_COMPLETE = 0x04U, /**< 校准完成 */
        CALIB_STATUS_FAILED = 0x05U    /**< 校准失败 */
    } CalibStatus_e;

    /*============================================================================
     * 参数类型定义
     *============================================================================*/
    typedef enum
    {
        PARAM_PID_CURRENT = 0x01U,       /**< 电流环PID */
        PARAM_PID_VELOCITY = 0x02U,      /**< 速度环PID */
        PARAM_PID_POSITION = 0x03U,      /**< 位置环PID */
        PARAM_MOTOR_CONFIG = 0x04U,      /**< 电机配置 */
        PARAM_ENCODER_CONFIG = 0x05U,    /**< 编码器配置 */
        PARAM_PROTECTION_CONFIG = 0x06U, /**< 保护配置 */
        PARAM_STREAM_CONFIG = 0x07U      /**< 流配置 */
    } ParamType_e;

    /*============================================================================
     * 数据流类型定义
     *============================================================================*/
    typedef enum
    {
        STREAM_ALL = 0x00U,      /**< 全部数据 */
        STREAM_CURRENT = 0x01U,  /**< 电流数据 */
        STREAM_VELOCITY = 0x02U, /**< 速度数据 */
        STREAM_POSITION = 0x03U, /**< 位置数据 */
        STREAM_ERROR = 0x04U     /**< 错误数据 */
    } StreamType_e;

    /*============================================================================
     * 数据结构定义 (所有结构体使用__attribute__((packed))对齐)
     *============================================================================*/

    /**
     * @brief 版本信息结构体 (6 bytes)
     * @note 使用packed对齐,禁止优化对齐导致的大小变化
     */
    typedef struct __attribute__((packed))
    {
        uint8_t protocol_ver; /**< 协议版本 */
        uint8_t major_ver;    /**< 主版本号 */
        uint8_t minor_ver;    /**< 次版本号 */
        uint8_t patch_ver;    /**< 补丁版本号 */
        uint8_t hardware_ver; /**< 硬件版本 */
        uint8_t reserve;      /**< 保留 */
    } VersionInfo_t;

    /**
     * @brief 状态数据结构体 (16 bytes)
     * @note 使用packed对齐
     */
    typedef struct __attribute__((packed))
    {
        uint8_t state;           /**< 状态 */
        uint8_t error_code;     /**< 错误码 */
        int16_t id_current;     /**< D轴电流 */
        int16_t iq_current;    /**< Q轴电流 */
        int16_t velocity;       /**< 速度 */
        int32_t position;       /**< 位置 */
        uint16_t voltage;       /**< 电压 */
        int8_t temperature;     /**< 温度 */
        uint8_t reserve;        /**< 保留(对齐) */
    } StatusData_t;

    /**
     * @brief 校准状态数据结构体 (3 bytes)
     * @note 使用packed对齐
     */
    typedef struct __attribute__((packed))
    {
        uint8_t status;   /**< 校准状态 */
        uint8_t progress; /**< 进度 */
        int8_t direction; /**< 方向 */
    } CalibStatusData_t;

    /**
     * @brief PID参数结构体 (16 bytes)
     * @note 使用Q16定点数替代float,符合项目规范(ISR中禁止使用float)
     * @note 使用packed对齐
     */
    typedef struct __attribute__((packed))
    {
        q16_16_t kp;           /**< 比例增益 Q16格式 */
        q16_16_t ki;           /**< 积分增益 Q16格式 */
        q16_16_t kd;           /**< 微分增益 Q16格式 */
        q16_16_t output_limit; /**< 输出限幅 Q16格式 */
    } PidParam_t;

    /**
     * @brief 流数据结构体 (18 bytes)
     * @note 使用packed对齐
     */
    typedef struct __attribute__((packed))
    {
        uint32_t timestamp;     /**< 时间戳 */
        int16_t id_current;     /**< D轴电流 */
        int16_t iq_current;     /**< Q轴电流 */
        int16_t velocity;       /**< 速度 */
        int32_t position;       /**< 位置 */
        uint16_t elec_angle;    /**< 电角度 */
        uint16_t reserve;       /**< 保留(对齐) */
    } StreamData_t;

    /**
     * @brief 流配置结构体
     * @note 使用packed对齐,与协议其他结构体保持一致
     */
    typedef struct __attribute__((packed))
    {
        uint8_t enable;       /**< 使能标志 */
        uint8_t stream_type;  /**< 流类型 */
        uint16_t interval_ms; /**< 间隔时间(毫秒) */
    } StreamConfig_t;

    /*============================================================================
     * 函数声明
     *============================================================================*/

    /**
     * @brief 初始化串口协议模块
     * @note 须在系统初始化阶段调用
     * @return void
     */
    void usart_protocol_init(void);

    /**
     * @brief 处理接收到的协议帧
     * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
     * @param[in] data  接收到的数据指针,不能为NULL
     * @param[in] len   数据长度
     * @return void
     */
    void usart_protocol_process_frame(uint8_t *data, uint16_t len);

    /**
     * @brief 流数据任务处理函数
     * @note 须在主循环或专用任务中周期性调用
     * @return void
     */
    void usart_protocol_stream_task(void);

    /**
     * @brief 发送协议响应帧
     * @note ASIL-B | Reentrant: YES | ISR-Safe: NO (需确保output缓冲区有效)
     * @param[in] msg_type   消息类型
     * @param[in] func_code   功能码
     * @param[in] data        数据指针,可为NULL
     * @param[in] len         数据长度
     * @return void
     */
    void usart_protocol_send_response(uint8_t msg_type, uint8_t func_code, const uint8_t *data, uint16_t len);

    /**
     * @brief 发送OK响应
     * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
     * @param[in] func_code 功能码
     * @return void
     */
    void usart_protocol_send_ok(uint8_t func_code);

    /**
     * @brief 发送错误响应
     * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
     * @param[in] func_code    功能码
     * @param[in] error_code   错误码
     * @param[in] error_class  错误类
     * @return void
     */
    void usart_protocol_send_error(uint8_t func_code, uint8_t error_code, uint8_t error_class);

    /**
     * @brief 发送ACK响应
     * @note ASIL-B | Reentrant: YES | ISR-Safe: NO
     * @param[in] func_code 功能码
     * @return void
     */
    void usart_protocol_send_ack(uint8_t func_code);

#ifdef __cplusplus
}
#endif

#endif /* USART_PROTOCOL_H */
