"""
Control Panel
控制面板 - 电机控制
"""

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QPushButton, QGroupBox, 
    QDoubleSpinBox, QComboBox
)
from PyQt6.QtCore import pyqtSignal


class ControlPanel(QGroupBox):
    """电机控制面板"""
    
    command_clicked = pyqtSignal(str, float, float)
    stop_clicked = pyqtSignal()
    calibrate_clicked = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__("电机控制", parent)
        self._setup_ui()
        
    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(10)
        
        # 控制模式
        mode_layout = QHBoxLayout()
        mode_layout.addWidget(QLabel("模式:"))
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["电流控制", "速度控制", "位置控制"])
        self.mode_combo.currentIndexChanged.connect(self._on_mode_changed)
        mode_layout.addWidget(self.mode_combo)
        layout.addLayout(mode_layout)
        
        # 参数输入
        self.param1_layout = QHBoxLayout()
        self.param1_label = QLabel("Id (A):")
        self.param1_spin = QDoubleSpinBox()
        self.param1_spin.setRange(-10, 10)
        self.param1_spin.setDecimals(3)
        self.param1_spin.setSingleStep(0.1)
        self.param1_layout.addWidget(self.param1_label)
        self.param1_layout.addWidget(self.param1_spin)
        layout.addLayout(self.param1_layout)
        
        self.param2_layout = QHBoxLayout()
        self.param2_label = QLabel("Iq (A):")
        self.param2_spin = QDoubleSpinBox()
        self.param2_spin.setRange(-10, 10)
        self.param2_spin.setDecimals(3)
        self.param2_spin.setSingleStep(0.1)
        self.param2_layout.addWidget(self.param2_label)
        self.param2_layout.addWidget(self.param2_spin)
        layout.addLayout(self.param2_layout)
        
        # 发送按钮
        self.send_btn = QPushButton("发送命令")
        self.send_btn.clicked.connect(self._on_send_clicked)
        layout.addWidget(self.send_btn)
        
        # 停止按钮
        self.stop_btn = QPushButton("紧急停止")
        self.stop_btn.setObjectName("danger")
        self.stop_btn.clicked.connect(self.stop_clicked.emit)
        layout.addWidget(self.stop_btn)
        
        # 分隔线
        layout.addSpacing(10)
        
        # 校准按钮
        self.calib_btn = QPushButton("编码器校准")
        self.calib_btn.setObjectName("success")
        self.calib_btn.clicked.connect(self.calibrate_clicked.emit)
        layout.addWidget(self.calib_btn)
        
        layout.addStretch()
    
    def _on_mode_changed(self, index: int):
        if index == 0:  # 电流
            self.param1_label.setText("Id (A):")
            self.param2_label.setText("Iq (A):")
            self.param1_spin.setRange(-10, 10)
            self.param2_spin.setRange(-10, 10)
        elif index == 1:  # 速度
            self.param1_label.setText("速度 (rad/s):")
            self.param2_label.setVisible(False)
            self.param2_spin.setVisible(False)
        else:  # 位置
            self.param1_label.setText("位置 (rad):")
            self.param2_label.setVisible(False)
            self.param2_spin.setVisible(False)
        
        if index > 0:
            self.param2_label.setVisible(True)
            self.param2_spin.setVisible(True)
    
    def _on_send_clicked(self):
        mode = self.mode_combo.currentIndex()
        val1 = self.param1_spin.value()
        val2 = self.param2_spin.value()
        
        if mode == 0:
            cmd = "current"
        elif mode == 1:
            cmd = "velocity"
            val2 = 0
        else:
            cmd = "position"
            val2 = 0
        
        self.command_clicked.emit(cmd, val1, val2)
    
    def set_enabled(self, enabled: bool):
        self.send_btn.setEnabled(enabled)
        self.stop_btn.setEnabled(enabled)
        self.calib_btn.setEnabled(enabled)
        self.mode_combo.setEnabled(enabled)
        self.param1_spin.setEnabled(enabled)
        self.param2_spin.setEnabled(enabled)
