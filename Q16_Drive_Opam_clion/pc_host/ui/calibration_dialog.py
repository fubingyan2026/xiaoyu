"""
Calibration Dialog
校准对话框 - 编码器校准
"""

from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, 
    QLabel, QPushButton, QProgressBar,
    QTextEdit, QWidget
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal


class CalibrationDialog(QDialog):
    """编码器校准对话框"""
    
    start_calib = pyqtSignal()
    stop_calib = pyqtSignal()
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("编码器校准")
        self.setMinimumSize(500, 400)
        self._setup_ui()
        self._calibrating = False
        
    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setSpacing(15)
        
        # 状态显示
        self.status_label = QLabel("就绪")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet("font-size: 16px; font-weight: bold; color: #4ec9b0;")
        layout.addWidget(self.status_label)
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.progress_bar.setRange(0, 100)
        self.progress_bar.setValue(0)
        self.progress_bar.setTextVisible(True)
        layout.addWidget(self.progress_bar)
        
        # 详细信息
        info_layout = QHBoxLayout()
        info_layout.addWidget(QLabel("方向:"))
        self.direction_label = QLabel("未知")
        self.direction_label.setObjectName("value")
        info_layout.addWidget(self.direction_label)
        info_layout.addStretch()
        info_layout.addWidget(QLabel("步骤:"))
        self.step_label = QLabel("0/44")
        self.step_label.setObjectName("value")
        info_layout.addWidget(self.step_label)
        layout.addLayout(info_layout)
        
        # 日志
        layout.addWidget(QLabel("校准日志:"))
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(150)
        layout.addWidget(self.log_text)
        
        # 按钮
        btn_layout = QHBoxLayout()
        self.start_btn = QPushButton("开始校准")
        self.start_btn.setObjectName("success")
        self.start_btn.clicked.connect(self._on_start)
        btn_layout.addWidget(self.start_btn)
        
        self.stop_btn = QPushButton("停止")
        self.stop_btn.setObjectName("danger")
        self.stop_btn.setEnabled(False)
        self.stop_btn.clicked.connect(self._on_stop)
        btn_layout.addWidget(self.stop_btn)
        
        self.close_btn = QPushButton("关闭")
        self.close_btn.clicked.connect(self.close)
        btn_layout.addWidget(self.close_btn)
        
        layout.addLayout(btn_layout)
    
    def _on_start(self):
        self._calibrating = True
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.progress_bar.setValue(0)
        self.log_text.clear()
        self._log("开始编码器校准...")
        self.start_calib.emit()
    
    def _on_stop(self):
        self._calibrating = False
        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        self._log("校准已停止")
        self.stop_calib.emit()
    
    def _log(self, message: str):
        self.log_text.append(message)
    
    def update_progress(self, status: int, progress: int, direction: int):
        """更新进度"""
        self.progress_bar.setValue(progress)
        
        status_text = {
            0: "就绪",
            1: "正在校准...",
            2: "正向旋转",
            3: "反向旋转",
            4: "校准完成",
            5: "校准失败"
        }
        self.status_label.setText(status_text.get(status, f"未知状态({status})"))
        
        dir_text = {0: "未知", 1: "正向", -1: "反向"}
        self.direction_label.setText(dir_text.get(direction, f"未知({direction})"))
        
        # 计算步骤
        total_steps = 44  # 11极对 * 4
        current_step = int(progress / 100.0 * total_steps)
        self.step_label.setText(f"{current_step}/{total_steps}")
        
        if status == 4:
            self._calibrating = False
            self.start_btn.setEnabled(True)
            self.stop_btn.setEnabled(False)
            self._log("✓ 校准完成!")
        elif status == 5:
            self._calibrating = False
            self.start_btn.setEnabled(True)
            self.stop_btn.setEnabled(False)
            self._log("✗ 校准失败!")
    
    def add_log(self, message: str):
        """添加日志"""
        self._log(message)
    
    def closeEvent(self, event):
        if self._calibrating:
            self.stop_calib.emit()
        event.accept()
