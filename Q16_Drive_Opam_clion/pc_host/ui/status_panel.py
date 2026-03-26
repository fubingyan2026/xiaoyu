"""
Status Panel
状态面板 - 显示电机状态
"""

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QGroupBox, QGridLayout
)
from PyQt6.QtCore import Qt


class StatusPanel(QGroupBox):
    """状态显示面板"""
    
    def __init__(self, parent=None):
        super().__init__("实时状态", parent)
        self._setup_ui()
        
    def _setup_ui(self):
        layout = QGridLayout(self)
        layout.setSpacing(8)
        
        # 电流
        layout.addWidget(QLabel("Id:"), 0, 0)
        self.id_label = QLabel("0.000 A")
        self.id_label.setObjectName("value")
        layout.addWidget(self.id_label, 0, 1)
        
        layout.addWidget(QLabel("Iq:"), 0, 2)
        self.iq_label = QLabel("0.000 A")
        self.iq_label.setObjectName("value")
        layout.addWidget(self.iq_label, 0, 3)
        
        # 速度位置
        layout.addWidget(QLabel("速度:"), 1, 0)
        self.vel_label = QLabel("0.00 rad/s")
        self.vel_label.setObjectName("value")
        layout.addWidget(self.vel_label, 1, 1)
        
        layout.addWidget(QLabel("位置:"), 1, 2)
        self.pos_label = QLabel("0.00 rad")
        self.pos_label.setObjectName("value")
        layout.addWidget(self.pos_label, 1, 3)
        
        # 电压温度
        layout.addWidget(QLabel("电压:"), 2, 0)
        self.volt_label = QLabel("0.0 V")
        self.volt_label.setObjectName("value")
        layout.addWidget(self.volt_label, 2, 1)
        
        layout.addWidget(QLabel("温度:"), 2, 2)
        self.temp_label = QLabel("0 °C")
        self.temp_label.setObjectName("value")
        layout.addWidget(self.temp_label, 2, 3)
        
        # 状态
        layout.addWidget(QLabel("状态:"), 3, 0)
        self.state_label = QLabel("IDLE")
        self.state_label.setObjectName("value")
        layout.addWidget(self.state_label, 3, 1)
        
        layout.addWidget(QLabel("错误:"), 3, 2)
        self.error_label = QLabel("0")
        self.error_label.setObjectName("value")
        layout.addWidget(self.error_label, 3, 3)
        
        # 电角度
        layout.addWidget(QLabel("电角度:"), 4, 0)
        self.angle_label = QLabel("0.00 rad")
        self.angle_label.setObjectName("value")
        layout.addWidget(self.angle_label, 4, 1, 1, 3)
    
    def update_status(self, data: dict):
        """更新状态显示"""
        self.id_label.setText(f"{data.get('id_current', 0):.3f} A")
        self.iq_label.setText(f"{data.get('iq_current', 0):.3f} A")
        self.vel_label.setText(f"{data.get('velocity', 0):.2f} rad/s")
        self.pos_label.setText(f"{data.get('position', 0):.2f} rad")
        self.volt_label.setText(f"{data.get('voltage', 0):.1f} V")
        self.temp_label.setText(f"{data.get('temperature', 0):.0f} °C")
        self.angle_label.setText(f"{data.get('electrical_angle', 0):.3f} rad")
        
        state = data.get('state', 0)
        state_names = {0: 'IDLE', 1: 'SELF_CHECK', 2: 'ALIGN', 
                      3: 'ALIGNMENT', 4: 'RUN', 5: 'HALL', 6: 'STOP'}
        self.state_label.setText(state_names.get(state, f"UNKNOWN({state})"))
        self.error_label.setText(str(data.get('error_code', 0)))
