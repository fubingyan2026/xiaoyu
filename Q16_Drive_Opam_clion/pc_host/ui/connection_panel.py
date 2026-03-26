"""
Connection Panel
连接面板 - 串口连接控制
"""

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QComboBox, QPushButton, QGroupBox
)
from PyQt6.QtCore import Qt, pyqtSignal


class ConnectionPanel(QGroupBox):
    """连接控制面板"""
    
    connect_clicked = pyqtSignal(str, int)
    disconnect_clicked = pyqtSignal()
    refresh_clicked = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__("连接设置", parent)
        self._setup_ui()
        self._connected = False
        
    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(8)
        
        # 端口选择
        port_layout = QHBoxLayout()
        port_layout.addWidget(QLabel("端口:"))
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        port_layout.addWidget(self.port_combo)
        
        self.refresh_btn = QPushButton("刷新")
        self.refresh_btn.setMaximumWidth(50)
        self.refresh_btn.clicked.connect(self.refresh_clicked.emit)
        port_layout.addWidget(self.refresh_btn)
        layout.addLayout(port_layout)
        
        # 波特率
        baud_layout = QHBoxLayout()
        baud_layout.addWidget(QLabel("波特率:"))
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.baud_combo.setCurrentText("115200")
        baud_layout.addWidget(self.baud_combo)
        layout.addLayout(baud_layout)
        
        # 连接按钮
        self.connect_btn = QPushButton("连接")
        self.connect_btn.setObjectName("success")
        self.connect_btn.clicked.connect(self._on_connect_clicked)
        layout.addWidget(self.connect_btn)
        
        # 状态标签
        self.status_label = QLabel("未连接")
        self.status_label.setObjectName("status_disconnected")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.status_label)
        
    def _on_connect_clicked(self):
        if not self._connected:
            port = self.port_combo.currentText()
            baud = int(self.baud_combo.currentText())
            self.connect_clicked.emit(port, baud)
        else:
            self.disconnect_clicked.emit()
    
    def set_connected(self, connected: bool):
        """设置连接状态"""
        self._connected = connected
        if connected:
            self.connect_btn.setText("断开")
            self.connect_btn.setObjectName("danger")
            self.status_label.setText("已连接")
            self.status_label.setObjectName("status_connected")
            self.port_combo.setEnabled(False)
            self.baud_combo.setEnabled(False)
        else:
            self.connect_btn.setText("连接")
            self.connect_btn.setObjectName("success")
            self.status_label.setText("未连接")
            self.status_label.setObjectName("status_disconnected")
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
        
        # 刷新样式
        self.style().unpolish(self.connect_btn)
        self.style().polish(self.connect_btn)
        self.style().unpolish(self.status_label)
        self.style().polish(self.status_label)
    
    def update_ports(self, ports: list):
        """更新端口列表"""
        current = self.port_combo.currentText()
        self.port_combo.clear()
        self.port_combo.addItems(ports)
        if current in ports:
            self.port_combo.setCurrentText(current)
