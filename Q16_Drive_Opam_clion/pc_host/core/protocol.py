"""
西门子S7风格通信协议实现

参考规范: Siemens S7comm / PPI Protocol
- 帧头: 0x68 0x68
- 协议ID: 0x32
- 校验: CRC16 Modbus
- 帧尾: 0x16
"""

from enum import IntEnum
from typing import Optional, Callable
import struct


class MessageType(IntEnum):
    """消息类型"""
    JOB = 0x01       # 请求/命令
    ACK = 0x02       # 简单确认 (无数据)
    RESPONSE = 0x03  # 带数据的响应
    ERROR = 0x07     # 错误响应


class FunctionCode(IntEnum):
    """功能码定义"""
    # 系统功能 (0x00-0x0F)
    FUNC_PING = 0x01           # 心跳/连接测试
    FUNC_GET_VERSION = 0x02    # 获取版本信息
    FUNC_RESET = 0x03          # 系统复位
    FUNC_GET_DEVICE_INFO = 0x04  # 获取设备信息
    
    # 状态功能 (0x10-0x1F)
    FUNC_GET_STATUS = 0x10      # 获取运行状态
    FUNC_GET_STATE = 0x11       # 获取FOC状态机状态
    FUNC_GET_ERROR = 0x12       # 获取错误状态
    
    # 控制功能 (0x20-0x2F)
    FUNC_SET_CURRENT = 0x20     # 设置电流 (Id, Iq)
    FUNC_SET_VELOCITY = 0x21    # 设置速度
    FUNC_SET_POSITION = 0x22    # 设置位置
    FUNC_STOP = 0x23            # 停止
    FUNC_START = 0x24           # 启动
    FUNC_BRAKE = 0x25           # 刹车
    
    # 校准功能 (0x30-0x3F)
    FUNC_START_CALIB = 0x30     # 开始编码器校准
    FUNC_GET_CALIB_STATUS = 0x31  # 获取校准状态
    FUNC_GET_CALIB_DATA = 0x32    # 获取校准数据
    FUNC_SET_CALIB_DATA = 0x33    # 设置校准数据
    FUNC_CLEAR_CALIB = 0x34       # 清除校准数据
    FUNC_SAVE_CALIB = 0x35        # 保存校准到Flash
    
    # 参数功能 (0x40-0x4F)
    FUNC_GET_PARAMS = 0x40       # 获取参数
    FUNC_SET_PARAMS = 0x41       # 设置参数
    FUNC_SAVE_PARAMS = 0x42      # 保存参数
    FUNC_LOAD_PARAMS = 0x43       # 加载参数
    
    # 数据流功能 (0x50-0x5F)
    FUNC_START_STREAM = 0x50     # 开始数据流
    FUNC_STOP_STREAM = 0x51      # 停止数据流
    FUNC_SET_STREAM_CFG = 0x52    # 配置数据流


class ErrorCode(IntEnum):
    """错误码定义"""
    ERR_GENERAL = 0x01           # 通用错误
    ERR_INVALID_CMD = 0x02       # 无效命令
    ERR_INVALID_PARAM = 0x03     # 无效参数
    ERR_DATA_LEN = 0x04          # 数据长度错误
    ERR_CHECKSUM = 0x05          # 校验错误
    ERR_TIMEOUT = 0x06           # 超时
    ERR_STATE = 0x07             # 状态错误
    ERR_OVERCURRENT = 0x08       # 过流
    ERR_OVERTEMP = 0x09          # 过温
    ERR_OVERVOLTAGE = 0x0A       # 过压
    ERR_UNDERVOLTAGE = 0x0B      # 欠压
    ERR_ENCODER = 0x0C           # 编码器错误
    ERR_CALIB = 0x0D             # 校准错误
    ERR_NOT_READY = 0x0E         # 未就绪
    ERR_BUSY = 0x0F              # 忙


class ErrorClass(IntEnum):
    """错误类定义"""
    ERR_NONE = 0x00              # 无错误
    ERR_COMM = 0x10              # 通信错误
    ERR_PROTOCOL = 0x20          # 协议错误
    ERR_DEVICE = 0x30            # 设备错误
    ERR_APP = 0x40               # 应用错误


class StreamType(IntEnum):
    """数据流类型"""
    STREAM_ALL = 0x00            # 所有数据
    STREAM_CURRENT = 0x01        # 仅电流
    STREAM_VELOCITY = 0x02       # 仅速度
    STREAM_POSITION = 0x03       # 仅位置
    STREAM_ERROR = 0x04          # 仅错误状态


class FocState(IntEnum):
    """FOC状态机状态"""
    IDLE = 0x00                  # 空闲
    SELF_CHECK = 0x01            # 自检
    ALIGN = 0x02                 # 角度对齐
    ALIGNMENT = 0x03             # 对齐完成
    RUN = 0x04                   # 运行
    HALL = 0x05                  # 霍尔模式
    STOP = 0x06                  # 停止
    FAULT = 0x07                 # 故障


class ParamType(IntEnum):
    """参数类型"""
    PID_CURRENT = 0x01           # 电流环PID参数
    PID_VELOCITY = 0x02          # 速度环PID参数
    PID_POSITION = 0x03          # 位置环PID参数
    MOTOR_CONFIG = 0x04          # 电机配置
    ENCODER_CONFIG = 0x05        # 编码器配置
    PROTECTION_CONFIG = 0x06     # 保护配置
    STREAM_CONFIG = 0x07         # 流配置


# CRC16 Modbus 表
CRC16_TABLE = [
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0xF000, 0x30C1, 0x3180, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0x8C01, 0x4CC0, 0x4D80, 0x8D41, 0x8800, 0x48C1, 0x4980, 0x8941,
    0x7800, 0x38C1, 0x3980, 0x7941, 0x2800, 0x18C1, 0x1980, 0x5941,
    0xB000, 0x70C1, 0x7180, 0xB141, 0xA000, 0x60C1, 0x6180, 0xA141,
    0x9000, 0x50C1, 0x5180, 0x9141, 0x8C01, 0x4CC0, 0x4D80, 0x8D41,
    0x8800, 0x48C1, 0x4980, 0x8941, 0x7800, 0x38C1, 0x3980, 0x7941,
    0x2800, 0x18C1, 0x1980, 0x5941, 0xF000, 0x70C1, 0x7180, 0xB141,
    0xA000, 0x60C1, 0x6180, 0xA141, 0x9000, 0x50C1, 0x5180, 0x9141,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440, 0xC0C1, 0x0001, 0xC181, 0x0140,
    0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41,
    0xF000, 0x30C1, 0x3180, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440, 0x8C01, 0x4CC0, 0x4D80, 0x8D41,
    0x8800, 0x48C1, 0x4980, 0x8941, 0x7800, 0x38C1, 0x3980, 0x7941,
    0x2800, 0x18C1, 0x1980, 0x5941, 0xB000, 0x70C1, 0x7180, 0xB141,
    0xA000, 0x60C1, 0x6180, 0xA141, 0x9000, 0x50C1, 0x5180, 0x9141,
    0x8C01, 0x4CC0, 0x4D80, 0x8D41, 0x8800, 0x48C1, 0x4980, 0x8941,
    0x7800, 0x38C1, 0x3980, 0x7941, 0x2800, 0x18C1, 0x1980, 0x5941,
    0xF000, 0x70C1, 0x7180, 0xB141, 0xA000, 0x60C1, 0x6180, 0xA141,
    0x9000, 0x50C1, 0x5180, 0x9141, 0xCC01, 0x0CC0, 0x0D80, 0xCD41,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xC0C1, 0x0001, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0xF000, 0x30C1, 0x3180, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0x8C01, 0x4CC0, 0x4D80, 0x8D41, 0x8800, 0x48C1, 0x4980, 0x8941,
    0x7800, 0x38C1, 0x3980, 0x7941, 0x2800, 0x18C1, 0x1980, 0x5941,
    0xB000, 0x70C1, 0x7180, 0xB141, 0xA000, 0x60C1, 0x6180, 0xA141,
    0x9000, 0x50C1, 0x5180, 0x9141, 0x8C01, 0x4CC0, 0x4D80, 0x8D41,
    0x8800, 0x48C1, 0x4980, 0x8941, 0x7800, 0x38C1, 0x3980, 0x7941,
    0x2800, 0x18C1, 0x1980, 0x5941, 0xF000, 0x70C1, 0x7180, 0xB141,
    0xA000, 0x60C1, 0x6180, 0xA141, 0x9000, 0x50C1, 0x5180, 0x9141,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440, 0xC0C1, 0x0001, 0xC181, 0x0140,
    0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440
]


def crc16_modbus(data: bytes) -> int:
    """计算CRC16 Modbus校验"""
    crc = 0xFFFF
    for byte in data:
        crc = (crc >> 8) ^ CRC16_TABLE[(crc ^ byte) & 0xFF]
    return crc


class S7Frame:
    """S7协议帧处理"""
    
    HEAD = bytes([0x68, 0x68])
    PROTOCOL_ID = 0x32
    TAIL = 0x16
    MIN_FRAME_LEN = 8  # head(2) + id(1) + type(1) + param(1) + len(1) + crc(2) + tail(1)
    
    def __init__(self, msg_type: int = 0, function_code: int = 0, data: bytes = b''):
        self.msg_type = msg_type
        self.function_code = function_code
        self.data = data
    
    @classmethod
    def from_bytes(cls, frame_data: bytes) -> Optional['S7Frame']:
        """从字节流解析帧"""
        if len(frame_data) < cls.MIN_FRAME_LEN:
            return None
        
        # 检查帧头
        if frame_data[:2] != cls.HEAD:
            return None
        
        # 检查帧尾
        if frame_data[-1] != cls.TAIL:
            return None
        
        # 检查协议ID
        if frame_data[2] != cls.PROTOCOL_ID:
            return None
        
        # 解析
        msg_type = frame_data[3]
        function_code = frame_data[4]
        
        # 计算数据区长度 (从字节5开始到CRC之前)
        payload_len = len(frame_data) - 8  # 总长度 - 2(头) - 1(id) - 1(type) - 1(func) - 2(crc) - 1(tail)
        payload = frame_data[5:5+payload_len]
        
        # 验证CRC
        calc_crc = cls._calc_crc(frame_data[2:-3])  # 从协议ID到CRC前
        recv_crc = frame_data[-3] | (frame_data[-2] << 8)
        
        if calc_crc != recv_crc:
            return None
        
        return cls(msg_type, function_code, payload)
    
    def to_bytes(self) -> bytes:
        """将帧转换为字节流"""
        # 构建不含CRC的数据
        payload = bytes([self.PROTOCOL_ID, self.msg_type, self.function_code]) + self.data
        crc = self._calc_crc(payload)
        
        frame = bytearray()
        frame.extend(self.HEAD)
        frame.extend(payload)
        frame.append(crc & 0xFF)
        frame.append((crc >> 8) & 0xFF)
        frame.append(self.TAIL)
        
        return bytes(frame)
    
    @staticmethod
    def _calc_crc(data: bytes) -> int:
        """计算CRC16"""
        return crc16_modbus(data)
    
    def __repr__(self):
        return f"S7Frame(type=0x{self.msg_type:02X}, func=0x{self.function_code:02X}, len={len(self.data)})"


class ProtocolParser:
    """协议解析器 - 处理字节流并提取完整帧"""
    
    def __init__(self, on_frame_callback: Optional[Callable[[S7Frame], None]] = None):
        self.buffer = bytearray()
        self.on_frame = on_frame_callback
        self.max_buffer_size = 1024
    
    def feed(self, data: bytes):
        """喂入数据"""
        self.buffer.extend(data)
        
        if len(self.buffer) > self.max_buffer_size:
            self.buffer = self.buffer[-self.max_buffer_size:]
        
        self._parse()
    
    def _parse(self):
        """解析缓冲区中的帧"""
        while len(self.buffer) >= S7Frame.MIN_FRAME_LEN:
            # 查找帧头
            head_pos = self.buffer.find(S7Frame.HEAD)
            
            if head_pos == -1:
                self.buffer.clear()
                return
            
            # 丢弃帧头前的数据
            if head_pos > 0:
                self.buffer = self.buffer[head_pos:]
            
            # 检查是否有足够的数据
            if len(self.buffer) < S7Frame.MIN_FRAME_LEN:
                return
            
            # 尝试解析帧
            frame_data = bytes(self.buffer)
            frame = S7Frame.from_bytes(frame_data)
            
            if frame:
                if self.on_frame:
                    self.on_frame(frame)
                self.buffer = self.buffer[len(frame.to_bytes()):]
            else:
                # 跳过第一个字节重新同步
                self.buffer = self.buffer[1:]
    
    def reset(self):
        """重置解析器"""
        self.buffer.clear()


class CommandBuilder:
    """命令构建器 - 简化命令创建"""
    
    @staticmethod
    def ping() -> S7Frame:
        """心跳命令"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_PING)
    
    @staticmethod
    def get_version() -> S7Frame:
        """获取版本"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_VERSION)
    
    @staticmethod
    def get_status() -> S7Frame:
        """获取状态"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_STATUS)
    
    @staticmethod
    def get_state() -> S7Frame:
        """获取FOC状态"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_STATE)
    
    @staticmethod
    def set_current(id_current: float, iq_current: float) -> S7Frame:
        """设置电流 (float: 单位: 0.001A)"""
        id_int = int(id_current * 1000)
        iq_int = int(iq_current * 1000)
        data = struct.pack('<hh', id_int, iq_int)
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_SET_CURRENT, data)
    
    @staticmethod
    def set_velocity(velocity: float) -> S7Frame:
        """设置速度 (float: rad/s * 1000)"""
        vel_int = int(velocity * 1000)
        data = struct.pack('<h', vel_int)
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_SET_VELOCITY, data)
    
    @staticmethod
    def set_position(position: float) -> S7Frame:
        """设置位置 (float: rad * 10000)"""
        pos_int = int(position * 10000)
        data = struct.pack('<i', pos_int)
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_SET_POSITION, data)
    
    @staticmethod
    def stop() -> S7Frame:
        """停止命令"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_STOP)
    
    @staticmethod
    def start() -> S7Frame:
        """启动命令"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_START)
    
    @staticmethod
    def start_calibration() -> S7Frame:
        """开始校准"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_START_CALIB)
    
    @staticmethod
    def get_calibration_status() -> S7Frame:
        """获取校准状态"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_CALIB_STATUS)
    
    @staticmethod
    def get_calibration_data() -> S7Frame:
        """获取校准数据"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_CALIB_DATA)
    
    @staticmethod
    def clear_calibration() -> S7Frame:
        """清除校准数据"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_CLEAR_CALIB)
    
    @staticmethod
    def save_calibration() -> S7Frame:
        """保存校准数据"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_SAVE_CALIB)
    
    @staticmethod
    def start_stream(stream_type: int = 0, interval_ms: int = 10) -> S7Frame:
        """
        开始数据流
        stream_type: 0=all, 1=current, 2=velocity, 3=position
        interval_ms: 流间隔 (ms)
        """
        data = bytes([stream_type, interval_ms & 0xFF])
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_START_STREAM, data)
    
    @staticmethod
    def stop_stream() -> S7Frame:
        """停止数据流"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_STOP_STREAM)
    
    @staticmethod
    def get_params(param_type: int) -> S7Frame:
        """获取参数"""
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_GET_PARAMS, bytes([param_type]))
    
    @staticmethod
    def set_params(param_type: int, param_data: bytes) -> S7Frame:
        """设置参数"""
        data = bytes([param_type]) + param_data
        return S7Frame(MessageType.JOB, FunctionCode.FUNC_SET_PARAMS, data)


class DataDecoder:
    """数据解码器"""
    
    @staticmethod
    def decode_status(data: bytes) -> dict:
        """解码状态数据 (16 bytes)"""
        if len(data) < 16:
            return {}
        
        state, error_code = struct.unpack('<BB', data[:2])
        id_current, iq_current = struct.unpack('<hh', data[2:6])
        velocity = struct.unpack('<h', data[6:8])[0]
        position = struct.unpack('<i', data[8:12])[0]
        voltage = struct.unpack('<H', data[12:14])[0]
        temperature = data[14]
        
        return {
            'state': state,
            'error_code': error_code,
            'id_current': id_current / 1000.0,
            'iq_current': iq_current / 1000.0,
            'velocity': velocity / 1000.0,
            'position': position / 10000.0,
            'voltage': voltage / 100.0,
            'temperature': temperature - 40
        }
    
    @staticmethod
    def decode_version(data: bytes) -> dict:
        """解码版本信息 (6 bytes)"""
        if len(data) < 6:
            return {}
        
        protocol_ver = data[0]
        firmware_ver = f"{data[1]}.{data[2]}.{data[3]}"
        hardware_ver = data[4]
        
        return {
            'protocol_version': protocol_ver,
            'firmware_version': firmware_ver,
            'hardware_version': hardware_ver
        }
    
    @staticmethod
    def decode_calibration_status(data: bytes) -> dict:
        """解码校准状态 (3 bytes)"""
        if len(data) < 3:
            return {}
        
        status = data[0]
        progress = data[1]
        direction = data[2] if len(data) > 2 else 0
        
        return {
            'status': status,
            'progress': progress,
            'direction': direction
        }
    
    @staticmethod
    def decode_stream_data(data: bytes) -> dict:
        """解码流数据 (18 bytes)"""
        if len(data) < 18:
            return {}
        
        timestamp = struct.unpack('<I', data[:4])[0]
        id_current, iq_current = struct.unpack('<hh', data[4:8])
        velocity = struct.unpack('<h', data[8:10])[0]
        position = struct.unpack('<i', data[10:14])[0]
        angle = struct.unpack('<H', data[14:16])[0]
        
        return {
            'timestamp': timestamp,
            'id_current': id_current / 1000.0,
            'iq_current': iq_current / 1000.0,
            'velocity': velocity / 1000.0,
            'position': position / 10000.0,
            'electrical_angle': (angle / 65535.0) * 6.2831853
        }
    
    @staticmethod
    def decode_error(data: bytes) -> dict:
        """解码错误响应 (2 bytes)"""
        if len(data) < 2:
            return {}
        
        error_code = data[0]
        error_class = data[1]
        
        return {
            'error_code': error_code,
            'error_class': error_class
        }
    
    @staticmethod
    def decode_pid_params(data: bytes) -> dict:
        """解码PID参数 (16 bytes)
        @note 使用Q16.16定点数格式: value = int32 / 65536.0
        """
        if len(data) < 16:
            return {}

        kp, ki, kd, limit = struct.unpack('<iiii', data[:16])

        Q16_SCALE = 65536.0

        return {
            'kp': kp / Q16_SCALE,
            'ki': ki / Q16_SCALE,
            'kd': kd / Q16_SCALE,
            'output_limit': limit / Q16_SCALE
        }


def build_response(msg_type: int, function_code: int, data: bytes = b'') -> bytes:
    """构建响应帧"""
    frame = S7Frame(msg_type, function_code, data)
    return frame.to_bytes()


def build_error_response(function_code: int, error_code: int, error_class: int) -> bytes:
    """构建错误响应帧"""
    data = bytes([error_code, error_class])
    return build_response(MessageType.ERROR, function_code, data)


def build_ok_response(function_code: int) -> bytes:
    """构建成功响应帧"""
    return build_response(MessageType.RESPONSE, function_code, bytes([0x00]))
