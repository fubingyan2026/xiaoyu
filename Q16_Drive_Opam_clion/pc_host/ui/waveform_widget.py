"""
Waveform Plot Widget
波形显示控件 - 使用pyqtgraph
"""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton
from PyQt6.QtCore import Qt, QTimer
import pyqtgraph as pg
import numpy as np
from core.models import MotorDataModel
from ui.styles import PLOT_STYLE


class WaveformWidget(QWidget):
    """波形显示控件"""
    
    def __init__(self, data_model: MotorDataModel, parent=None):
        super().__init__(parent)
        self.data_model = data_model
        self._setup_ui()
        
        # 更新定时器
        self.update_timer = QTimer(self)
        self.update_timer.timeout.connect(self._update_plot)
        self.update_timer.start(50)  # 20Hz
        
    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        
        # 工具栏
        toolbar = QHBoxLayout()
        self.stream_btn = QPushButton("开始采集")
        self.stream_btn.setCheckable(True)
        self.stream_btn.toggled.connect(self._on_stream_toggled)
        toolbar.addWidget(self.stream_btn)
        
        toolbar.addWidget(QLabel("显示:"))
        self.current_cb = QPushButton("电流")
        self.current_cb.setCheckable(True)
        self.current_cb.setChecked(True)
        toolbar.addWidget(self.current_cb)
        
        self.velocity_cb = QPushButton("速度")
        self.velocity_cb.setCheckable(True)
        self.velocity_cb.setChecked(True)
        toolbar.addWidget(self.velocity_cb)
        
        self.position_cb = QPushButton("位置")
        self.position_cb.setCheckable(True)
        toolbar.addWidget(self.position_cb)
        
        toolbar.addStretch()
        layout.addLayout(toolbar)
        
        # 创建绘图区域
        self.plot_widget = pg.GraphicsLayoutWidget()
        layout.addWidget(self.plot_widget)
        
        # 配置pyqtgraph样式
        pg.setConfigOptions(antialias=True)
        self.plot_widget.setBackground(PLOT_STYLE['background'])
        
        # 电流图
        self.current_plot = self.plot_widget.addPlot(row=0, col=0)
        self.current_plot.setTitle('电流', color=PLOT_STYLE['foreground'])
        self.current_plot.setLabel('left', 'A', color=PLOT_STYLE['foreground'])
        self.current_plot.showGrid(x=True, y=True, alpha=0.3)
        self.current_plot.getAxis('left').setPen(PLOT_STYLE['axis_color'])
        self.current_plot.getAxis('bottom').setPen(PLOT_STYLE['axis_color'])
        
        self.id_curve = self.current_plot.plot(pen=pg.mkPen(PLOT_STYLE['colors'][0], width=2), name='Id')
        self.iq_curve = self.current_plot.plot(pen=pg.mkPen(PLOT_STYLE['colors'][1], width=2), name='Iq')
        
        # 速度图
        self.velocity_plot = self.plot_widget.addPlot(row=1, col=0)
        self.velocity_plot.setTitle('速度', color=PLOT_STYLE['foreground'])
        self.velocity_plot.setLabel('left', 'rad/s', color=PLOT_STYLE['foreground'])
        self.velocity_plot.showGrid(x=True, y=True, alpha=0.3)
        self.velocity_plot.getAxis('left').setPen(PLOT_STYLE['axis_color'])
        self.velocity_plot.getAxis('bottom').setPen(PLOT_STYLE['axis_color'])
        
        self.velocity_curve = self.velocity_plot.plot(pen=pg.mkPen(PLOT_STYLE['colors'][2], width=2))
        
        # 位置图
        self.position_plot = self.plot_widget.addPlot(row=2, col=0)
        self.position_plot.setTitle('位置/角度', color=PLOT_STYLE['foreground'])
        self.position_plot.setLabel('left', 'rad', color=PLOT_STYLE['foreground'])
        self.position_plot.setLabel('bottom', 'Time', color=PLOT_STYLE['foreground'])
        self.position_plot.showGrid(x=True, y=True, alpha=0.3)
        self.position_plot.getAxis('left').setPen(PLOT_STYLE['axis_color'])
        self.position_plot.getAxis('bottom').setPen(PLOT_STYLE['axis_color'])
        
        self.position_curve = self.position_plot.plot(pen=pg.mkPen(PLOT_STYLE['colors'][3], width=2), name='Position')
        self.angle_curve = self.position_plot.plot(pen=pg.mkPen(PLOT_STYLE['colors'][4], width=2), name='Elec Angle')
        
        # 链接X轴
        self.velocity_plot.setXLink(self.current_plot)
        self.position_plot.setXLink(self.current_plot)
        
        self.stream_active = False
        
    def _update_plot(self):
        if not self.stream_active:
            return
        
        # 更新电流
        if self.current_cb.isChecked() and len(self.data_model.id_buffer) > 0:
            times = self.data_model.id_buffer.get_time()
            if len(times) > 0:
                times = times - times[-1]
            self.id_curve.setData(times, self.data_model.id_buffer.get_data())
            self.iq_curve.setData(times, self.data_model.iq_buffer.get_data())
        
        # 更新速度
        if self.velocity_cb.isChecked() and len(self.data_model.velocity_buffer) > 0:
            times = self.data_model.velocity_buffer.get_time()
            if len(times) > 0:
                times = times - times[-1]
            self.velocity_curve.setData(times, self.data_model.velocity_buffer.get_data())
        
        # 更新位置
        if self.position_cb.isChecked():
            if len(self.data_model.position_buffer) > 0:
                times = self.data_model.position_buffer.get_time()
                if len(times) > 0:
                    times = times - times[-1]
                self.position_curve.setData(times, self.data_model.position_buffer.get_data())
            if len(self.data_model.angle_buffer) > 0:
                times = self.data_model.angle_buffer.get_time()
                if len(times) > 0:
                    times = times - times[-1]
                self.angle_curve.setData(times, self.data_model.angle_buffer.get_data())
    
    def _on_stream_toggled(self, checked: bool):
        self.stream_active = checked
        self.stream_btn.setText("停止采集" if checked else "开始采集")
        
    def clear(self):
        """清空波形"""
        self.id_curve.clear()
        self.iq_curve.clear()
        self.velocity_curve.clear()
        self.position_curve.clear()
        self.angle_curve.clear()
