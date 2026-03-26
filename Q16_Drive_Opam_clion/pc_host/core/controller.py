"""
Communication Controller
通信控制器 - 整合协议和串口管理
"""

from PyQt6.QtCore import QObject, pyqtSignal, QTimer
from typing import Optional, Callable
from core.protocol import (
    S7Frame, ProtocolParser, CommandBuilder,
    DataDecoder, MessageType, FunctionCode
)
from core.serial_manager import SerialManager


class FOCController(QObject):
    """FOC控制器 - 提供高级接口"""
    
    status_updated = pyqtSignal(dict)
    calibration_progress = pyqtSignal(dict)
    stream_data = pyqtSignal(dict)
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.serial = SerialManager(self)
        self.parser = ProtocolParser(self._on_frame_received)
        
        self.serial.data_received.connect(self._on_data_received)
        self.serial.connection_changed.connect(self._on_connection_changed)
        self.serial.error_occurred.connect(self.error.emit)
        
        self._response_callbacks: dict[int, Callable] = {}
        self._stream_enabled = False
        
        # 心跳定时器
        self._ping_timer = QTimer(self)
        self._ping_timer.timeout.connect(self._send_ping)
        
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
        cmd = frame.function_code
        
        if cmd == FunctionCode.FUNC_GET_STATUS:
            self._handle_response_data(frame.data)
        elif cmd == FunctionCode.FUNC_GET_STATE:
            pass
        elif cmd == MessageType.RESPONSE:
            data = DataDecoder.decode_stream_data(frame.data)
            if data:
                self.stream_data.emit(data)
        elif cmd == MessageType.ACK:
            pass
        elif cmd == MessageType.ERROR:
            self.error.emit(f"Device error: {frame.data.hex()}")
        
        # 调用回调
        if cmd in self._response_callbacks:
            callback = self._response_callbacks.pop(cmd)
            callback(frame)
    
    def _handle_response_data(self, data: bytes):
        """处理数据响应"""
        if len(data) < 1:
            return
        
        sub_cmd = data[0]
        payload = data[1:]
        
        if sub_cmd == FunctionCode.FUNC_GET_STATUS:
            status = DataDecoder.decode_status(payload)
            if status:
                self.status_updated.emit(status)
        elif sub_cmd == FunctionCode.FUNC_GET_CALIB_STATUS:
            calib_status = DataDecoder.decode_calibration_status(payload)
            if calib_status:
                self.calibration_progress.emit(calib_status)
    
    def _on_connection_changed(self, connected: bool):
        if connected:
            self.connected.emit()
            self._ping_timer.start(1000)
        else:
            self.disconnected.emit()
            self._ping_timer.stop()
    
    def _send_ping(self):
        """发送心跳"""
        self._send_frame(CommandBuilder.ping())
    
    def _send_frame(self, frame: S7Frame, callback: Optional[Callable] = None):
        """发送帧"""
        if callback:
            self._response_callbacks[frame.function_code | 0x80] = callback
        self.serial.send(frame.to_bytes())
    
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
