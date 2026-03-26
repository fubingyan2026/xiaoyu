"""
Main Window
主窗口
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, 
    QVBoxLayout, QMessageBox, QSplitter
)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont

from core.controller import FOCController
from core.models import MotorDataModel
from ui.styles import MAIN_STYLE
from ui.connection_panel import ConnectionPanel
from ui.control_panel import ControlPanel
from ui.status_panel import StatusPanel
from ui.waveform_widget import WaveformWidget
from ui.calibration_dialog import CalibrationDialog


class MainWindow(QMainWindow):
    """主窗口"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("FOC Motor Controller v1.0")
        self.setMinimumSize(1200, 800)
        
        # 设置全局字体
        font = QFont("Microsoft YaHei", 10)
        self.setFont(font)
        
        # 应用样式
        self.setStyleSheet(MAIN_STYLE)
        
        # 初始化核心组件
        self.controller = FOCController(self)
        self.data_model = MotorDataModel(self)
        self.calib_dialog: CalibrationDialog = None
        
        # 设置UI
        self._setup_ui()
        self._connect_signals()
        
        # 状态更新定时器
        self.status_timer = QTimer(self)
        self.status_timer.timeout.connect(self._request_status)
        self.status_timer.start(100)  # 10Hz
    
    def _setup_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        
        main_layout = QHBoxLayout(central)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # 左侧控制面板
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(10)
        
        self.conn_panel = ConnectionPanel()
        left_layout.addWidget(self.conn_panel)
        
        self.control_panel = ControlPanel()
        self.control_panel.set_enabled(False)
        left_layout.addWidget(self.control_panel)
        
        self.status_panel = StatusPanel()
        left_layout.addWidget(self.status_panel)
        
        left_layout.addStretch()
        
        main_layout.addWidget(left_panel, 1)
        
        # 右侧波形显示
        self.waveform_widget = WaveformWidget(self.data_model)
        main_layout.addWidget(self.waveform_widget, 4)
        
        # 刷新端口列表
        self._refresh_ports()
    
    def _connect_signals(self):
        # 连接面板信号
        self.conn_panel.connect_clicked.connect(self._on_connect)
        self.conn_panel.disconnect_clicked.connect(self._on_disconnect)
        self.conn_panel.refresh_clicked.connect(self._refresh_ports)
        
        # 控制面板信号
        self.control_panel.command_clicked.connect(self._on_command)
        self.control_panel.stop_clicked.connect(self._on_stop)
        self.control_panel.calibrate_clicked.connect(self._on_calibrate)
        
        # 控制器信号
        self.controller.connected.connect(self._on_controller_connected)
        self.controller.disconnected.connect(self._on_controller_disconnected)
        self.controller.error.connect(self._on_controller_error)
        self.controller.status_updated.connect(self._on_status_updated)
        self.controller.calibration_progress.connect(self._on_calib_progress)
        self.controller.stream_data.connect(self._on_stream_data)
    
    def _refresh_ports(self):
        ports = self.controller.serial.get_available_ports()
        self.conn_panel.update_ports(ports)
    
    def _on_connect(self, port: str, baudrate: int):
        if self.controller.connect(port, baudrate):
            self.conn_panel.set_connected(True)
    
    def _on_disconnect(self):
        self.controller.disconnect()
        self.conn_panel.set_connected(False)
    
    def _on_controller_connected(self):
        self.control_panel.set_enabled(True)
        self.waveform_widget.stream_btn.setEnabled(True)
    
    def _on_controller_disconnected(self):
        self.control_panel.set_enabled(False)
        self.waveform_widget.stream_btn.setEnabled(False)
        self.waveform_widget.stream_btn.setChecked(False)
        if self.calib_dialog:
            self.calib_dialog.close()
    
    def _on_controller_error(self, error: str):
        QMessageBox.critical(self, "错误", f"通信错误: {error}")
        self._on_disconnect()
    
    def _request_status(self):
        if self.controller.is_connected():
            self.controller.get_status()
    
    def _on_status_updated(self, status: dict):
        self.data_model.update_from_status(status)
        self.status_panel.update_status(self.data_model.__dict__)
    
    def _on_stream_data(self, data: dict):
        self.data_model.update_from_stream(data)
        self.status_panel.update_status(self.data_model.__dict__)
    
    def _on_command(self, cmd: str, val1: float, val2: float):
        if cmd == "current":
            self.controller.set_current(val1, val2)
        elif cmd == "velocity":
            self.controller.set_velocity(val1)
        elif cmd == "position":
            self.controller.set_position(val1)
    
    def _on_stop(self):
        self.controller.stop()
    
    def _on_calibrate(self):
        if not self.calib_dialog:
            self.calib_dialog = CalibrationDialog(self)
            self.calib_dialog.start_calib.connect(self.controller.start_calibration)
            self.calib_dialog.stop_calib.connect(self._on_stop)
        self.calib_dialog.show()
        self.calib_dialog.raise_()
    
    def _on_calib_progress(self, status: dict):
        if self.calib_dialog:
            self.calib_dialog.update_progress(
                status.get('status', 0),
                status.get('progress', 0),
                status.get('direction', 0)
            )
    
    def closeEvent(self, event):
        self.controller.disconnect()
        event.accept()
