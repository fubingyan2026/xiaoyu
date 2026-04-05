"""
Communication Controller
通信控制器 - 整合协议和串口管理
"""

from PyQt6.QtCore import QObject, pyqtSignal, QTimer
from typing import Optional, Callable
from datetime import datetime
from core.protocol import (
    S7Frame, ProtocolParser, CommandBuilder,
    DataDecoder, MessageType, FunctionCode
)
from core.serial_manager import SerialManager


def get_timestamp() -> str:
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]


class FOCController(QObject):
    """FOC控制器 - 提供高级接口"""

    status_updated = pyqtSignal(dict)
    calibration_progress = pyqtSignal(dict)
    stream_data = pyqtSignal(dict)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error = pyqtSignal(str)
    ping_timeout = pyqtSignal()

    PING_TIMEOUT_MS = 3000
    MAX_TIMEOUT_COUNT = 3

    def __init__(self, parent=None):
        super().__init__(parent)
        self.serial = SerialManager(self)
        self.parser = ProtocolParser(self._on_frame_received)

        self.serial.data_received.connect(self._on_data_received)
        self.serial.connection_changed.connect(self._on_connection_changed)
        self.serial.error_occurred.connect(self.error.emit)

        self._response_callbacks: dict[int, Callable] = {}
        self._stream_enabled = False

        self._ping_timer = QTimer(self)
        self._ping_timer.timeout.connect(self._send_ping)

        self._ping_timeout_timer = QTimer(self)
        self._ping_timeout_timer.timeout.connect(self._on_ping_timeout)

        self._ping_pending = False
        self._timeout_count = 0

    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """连接设备"""
        return self.serial.connect(port, baudrate)

    def disconnect(self):
        """断开连接"""
        self._ping_timer.stop()
        if self._stream_enabled:
            self.stop_stream()
        self.serial.disconnect()

    def is_connected(self) -> bool:
        return self.serial.is_connected()

    def _on_data_received(self, data: bytes):
        """处理接收到的原始数据"""
        self.parser.feed(data)

    def _on_frame_received(self, frame: S7Frame):
        """处理解析后的帧"""
        msg_type_name = MessageType(frame.msg_type).name if isinstance(frame.msg_type, int) else frame.msg_type.name
        func_name = self._get_func_name(frame.function_code)
        ts = get_timestamp()
        print(f"[{ts}] [RX] {msg_type_name:8s} FUNC=0x{frame.function_code:02X} ({func_name:20s}) LEN={len(frame.data):3d} DATA={frame.data.hex()}")
        msg_type = frame.msg_type
        func_code = frame.function_code

        if func_code == FunctionCode.FUNC_PING and msg_type == MessageType.RESPONSE:
            self._on_ping_response()

        if msg_type == MessageType.RESPONSE:
            if func_code == FunctionCode.FUNC_GET_STATUS:
                status = DataDecoder.decode_status(frame.data)
                if status:
                    self.status_updated.emit(status)
            elif func_code == FunctionCode.FUNC_GET_CALIB_STATUS:
                calib_status = DataDecoder.decode_calibration_status(frame.data)
                if calib_status:
                    self.calibration_progress.emit(calib_status)
            elif func_code == FunctionCode.FUNC_GET_VERSION:
                version = DataDecoder.decode_version(frame.data)
                if version:
                    pass
            else:
                data = DataDecoder.decode_stream_data(frame.data)
                if data:
                    self.stream_data.emit(data)
        elif msg_type == MessageType.ACK:
            pass
        elif msg_type == MessageType.ERROR:
            error_info = DataDecoder.decode_error(frame.data)
            self.error.emit(f"Device error: code={error_info.get('error_code', 0):02X}, class={error_info.get('error_class', 0):02X}")

        callback_key = func_code | 0x80
        if callback_key in self._response_callbacks:
            callback = self._response_callbacks.pop(callback_key)
            callback(frame)

    def _on_ping_response(self):
        """收到 PING 响应"""
        self._ping_timeout_timer.stop()
        self._ping_pending = False
        self._timeout_count = 0

    def _on_connection_changed(self, connected: bool):
        if connected:
            self.connected.emit()
            self._timeout_count = 0
            self._ping_pending = False
            self._ping_timer.start(1000)
        else:
            self.disconnected.emit()
            self._ping_timer.stop()
            self._ping_timeout_timer.stop()

    def _send_ping(self):
        """发送心跳"""
        if self._ping_pending:
            return
        self._ping_pending = True
        self._ping_timeout_timer.start(self.PING_TIMEOUT_MS)
        self._send_frame(CommandBuilder.ping())

    def _on_ping_timeout(self):
        """PING 超时处理"""
        self._ping_timeout_timer.stop()
        self._ping_pending = False
        self._timeout_count += 1
        print(f"[{get_timestamp()}] [WARN] PING timeout ({self._timeout_count}/{self.MAX_TIMEOUT_COUNT})")
        if self._timeout_count >= self.MAX_TIMEOUT_COUNT:
            self.error.emit(f"设备无响应: 连续 {self.MAX_TIMEOUT_COUNT} 次 PING 超时")
            self.ping_timeout.emit()

    def _send_frame(self, frame: S7Frame, callback: Optional[Callable] = None):
        """发送帧"""
        if callback:
            self._response_callbacks[frame.function_code | 0x80] = callback
        data = frame.to_bytes()
        msg_type_name = MessageType(frame.msg_type).name if isinstance(frame.msg_type, int) else frame.msg_type.name
        func_name = self._get_func_name(frame.function_code)
        ts = get_timestamp()
        print(f"[{ts}] [TX] {msg_type_name:8s} FUNC=0x{frame.function_code:02X} ({func_name:20s}) LEN={len(data):3d} DATA={data.hex()}")
        self.serial.send(data)

    def _get_func_name(self, func_code: int) -> str:
        """获取功能码名称"""
        func_names = {
            0x01: "PING",
            0x02: "GET_VERSION",
            0x03: "RESET",
            0x04: "GET_DEVICE_INFO",
            0x10: "GET_STATUS",
            0x11: "GET_STATE",
            0x12: "GET_ERROR",
            0x20: "SET_CURRENT",
            0x21: "SET_VELOCITY",
            0x22: "SET_POSITION",
            0x23: "STOP",
            0x24: "START",
            0x25: "BRAKE",
            0x30: "START_CALIB",
            0x31: "GET_CALIB_STATUS",
            0x32: "GET_CALIB_DATA",
            0x33: "SET_CALIB_DATA",
            0x34: "CLEAR_CALIB",
            0x35: "SAVE_CALIB",
            0x40: "GET_PARAMS",
            0x41: "SET_PARAMS",
            0x42: "SAVE_PARAMS",
            0x43: "LOAD_PARAMS",
            0x50: "START_STREAM",
            0x51: "STOP_STREAM",
            0x52: "SET_STREAM_CFG",
        }
        return func_names.get(func_code, "UNKNOWN")

    # ========== 公共控制接口 ==========

    def get_status(self):
        """获取当前状态"""
        self._send_frame(CommandBuilder.get_status())

    def set_current(self, id_a: float, iq_a: float):
        """设置电流 (A)"""
        self._send_frame(CommandBuilder.set_current(id_a, iq_a))

    def set_velocity(self, velocity_rad_s: float):
        """设置速度 (rad/s)"""
        self._send_frame(CommandBuilder.set_velocity(velocity_rad_s))

    def set_position(self, position_rad: float):
        """设置位置 (rad)"""
        self._send_frame(CommandBuilder.set_position(position_rad))

    def stop(self):
        """停止电机"""
        self._send_frame(CommandBuilder.stop())

    def start_calibration(self):
        """开始编码器校准"""
        self._send_frame(CommandBuilder.start_calibration())

    def get_calibration_status(self):
        """获取校准状态"""
        self._send_frame(CommandBuilder.get_calibration_status())

    def start_stream(self, stream_type: int = 0):
        """开始数据流"""
        self._send_frame(CommandBuilder.start_stream(stream_type))
        self._stream_enabled = True

    def stop_stream(self):
        """停止数据流"""
        self._send_frame(CommandBuilder.stop_stream())
        self._stream_enabled = False

    def reset_device(self):
        """复位设备"""
        self._send_frame(S7Frame(MessageType.JOB, FunctionCode.FUNC_RESET))
