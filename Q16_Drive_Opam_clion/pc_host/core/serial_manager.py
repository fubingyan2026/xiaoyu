"""
Serial Communication Manager
串口通信管理器 - 处理串口连接和数据收发
"""

from PyQt6.QtCore import QObject, QThread, pyqtSignal, QMutex, QWaitCondition
import serial
import serial.tools.list_ports
from typing import Optional, Callable
import time


class SerialWorker(QThread):
    """串口工作线程"""
    data_received = pyqtSignal(bytes)
    connection_error = pyqtSignal(str)
    connection_closed = pyqtSignal()
    
    def __init__(self, port: str, baudrate: int = 115200):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.serial: Optional[serial.Serial] = None
        self.running = False
        self._mutex = QMutex()
        
    def run(self):
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.01
            )
            self.running = True
            
            while self.running and self.serial.is_open:
                try:
                    if self.serial.in_waiting > 0:
                        data = self.serial.read(self.serial.in_waiting)
                        if data:
                            self.data_received.emit(data)
                    time.sleep(0.001)
                except serial.SerialException as e:
                    if self.running:
                        self.connection_error.emit(str(e))
                    break
                    
        except serial.SerialException as e:
            self.connection_error.emit(str(e))
        finally:
            if self.serial and self.serial.is_open:
                self.serial.close()
            self.connection_closed.emit()
    
    def send(self, data: bytes) -> bool:
        if self.serial and self.serial.is_open:
            try:
                self.serial.write(data)
                return True
            except serial.SerialException:
                return False
        return False
    
    def stop(self):
        self.running = False
        if self.serial and self.serial.is_open:
            self.serial.close()


class SerialManager(QObject):
    """串口管理器 - 管理串口连接和通信"""
    
    data_received = pyqtSignal(bytes)
    connection_changed = pyqtSignal(bool)
    error_occurred = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.worker: Optional[SerialWorker] = None
        self._connected = False
        
    def get_available_ports(self) -> list:
        """获取可用串口列表"""
        ports = serial.tools.list_ports.comports()
        return [f"{p.device} - {p.description}" for p in ports]
    
    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """连接串口"""
        if self._connected:
            self.disconnect()
            
        port_name = port.split(' - ')[0] if ' - ' in port else port
        
        self.worker = SerialWorker(port_name, baudrate)
        self.worker.data_received.connect(self._on_data_received)
        self.worker.connection_error.connect(self._on_error)
        self.worker.connection_closed.connect(self._on_closed)
        self.worker.start()
        
        time.sleep(0.1)
        self._connected = True
        self.connection_changed.emit(True)
        return True
    
    def disconnect(self):
        """断开连接"""
        if self.worker:
            self.worker.stop()
            self.worker.wait(1000)
            self.worker = None
        self._connected = False
        self.connection_changed.emit(False)
    
    def send(self, data: bytes) -> bool:
        """发送数据"""
        if self.worker and self._connected:
            return self.worker.send(data)
        return False
    
    def is_connected(self) -> bool:
        return self._connected
    
    def _on_data_received(self, data: bytes):
        self.data_received.emit(data)
    
    def _on_error(self, error_msg: str):
        self.error_occurred.emit(error_msg)
        self._connected = False
        self.connection_changed.emit(False)
    
    def _on_closed(self):
        self._connected = False
        self.connection_changed.emit(False)
