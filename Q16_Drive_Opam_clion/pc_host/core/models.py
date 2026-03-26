"""
Data Models
数据模型 - 存储和管理运行时数据
"""

from PyQt6.QtCore import QObject, pyqtSignal
from collections import deque
from typing import Optional
import numpy as np


class CircularBuffer:
    """循环缓冲区 - 用于存储波形数据"""
    
    def __init__(self, size: int = 1000):
        self.size = size
        self.buffer = deque(maxlen=size)
        self.time_buffer = deque(maxlen=size)
        
    def append(self, value: float, timestamp: float = 0):
        self.buffer.append(value)
        self.time_buffer.append(timestamp)
        
    def get_data(self) -> np.ndarray:
        return np.array(self.buffer)
    
    def get_time(self) -> np.ndarray:
        return np.array(self.time_buffer)
    
    def clear(self):
        self.buffer.clear()
        self.time_buffer.clear()
    
    def __len__(self):
        return len(self.buffer)


class MotorDataModel(QObject):
    """电机数据模型"""
    
    data_changed = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # 实时值
        self.id_current = 0.0
        self.iq_current = 0.0
        self.velocity = 0.0
        self.position = 0.0
        self.electrical_angle = 0.0
        self.voltage = 0.0
        self.temperature = 0.0
        self.state = 0
        self.error_code = 0
        
        # 波形缓冲区
        self.buffer_size = 1000
        self.id_buffer = CircularBuffer(self.buffer_size)
        self.iq_buffer = CircularBuffer(self.buffer_size)
        self.velocity_buffer = CircularBuffer(self.buffer_size)
        self.position_buffer = CircularBuffer(self.buffer_size)
        self.angle_buffer = CircularBuffer(self.buffer_size)
        
    def update_from_status(self, status: dict):
        """从状态数据更新"""
        self.id_current = status.get('id_current', 0)
        self.iq_current = status.get('iq_current', 0)
        self.velocity = status.get('velocity', 0)
        self.position = status.get('position', 0)
        self.voltage = status.get('voltage', 0)
        self.temperature = status.get('temperature', 0)
        self.state = status.get('state', 0)
        self.error_code = status.get('error_code', 0)
        self.data_changed.emit()
    
    def update_from_stream(self, data: dict):
        """从流数据更新"""
        timestamp = data.get('timestamp', 0)
        
        self.id_current = data.get('id_current', 0)
        self.iq_current = data.get('iq_current', 0)
        self.velocity = data.get('velocity', 0)
        self.position = data.get('position', 0)
        self.electrical_angle = data.get('electrical_angle', 0)
        
        # 添加到缓冲区
        self.id_buffer.append(self.id_current, timestamp)
        self.iq_buffer.append(self.iq_current, timestamp)
        self.velocity_buffer.append(self.velocity, timestamp)
        self.position_buffer.append(self.position, timestamp)
        self.angle_buffer.append(self.electrical_angle, timestamp)
        
        self.data_changed.emit()
    
    def clear_buffers(self):
        """清空缓冲区"""
        self.id_buffer.clear()
        self.iq_buffer.clear()
        self.velocity_buffer.clear()
        self.position_buffer.clear()
        self.angle_buffer.clear()


class CalibrationModel(QObject):
    """校准数据模型"""
    
    progress_changed = pyqtSignal(int, int, int)
    calib_data_received = pyqtSignal(dict)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.status = 0
        self.progress = 0
        self.direction = 0
        self.angle_map = []
        self.is_valid = False
        
    def update_status(self, status: dict):
        """更新校准状态"""
        self.status = status.get('status', 0)
        self.progress = status.get('progress', 0)
        self.direction = status.get('direction', 0)
        self.progress_changed.emit(self.status, self.progress, self.direction)
        
    def update_data(self, angle_map: list, direction: int):
        """更新校准数据"""
        self.angle_map = angle_map
        self.direction = direction
        self.is_valid = True
        self.calib_data_received.emit({
            'angle_map': angle_map,
            'direction': direction
        })


STATE_NAMES = {
    0: 'IDLE',
    1: 'SELF_CHECK',
    2: 'ALIGN',
    3: 'ALIGNMENT',
    4: 'RUN',
    5: 'HALL',
    6: 'STOP'
}

CALIB_STATE_NAMES = {
    0: 'IDLE',
    1: 'RUNNING',
    2: 'FORWARD',
    3: 'REVERSE',
    4: 'COMPLETE',
    5: 'FAILED'
}
