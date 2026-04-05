"""
Main Window - Glassmorphism Style
主窗口 - 玻璃拟态风格
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QFrame, QLabel, QPushButton, QStackedWidget,
    QSizePolicy, QMessageBox, QSpacerItem,
    QComboBox
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal
from PyQt6.QtGui import QFont

from core.controller import FOCController
from core.models import MotorDataModel
from ui.styles_glass import MAIN_STYLE, PLOT_STYLE, SIDEBAR_ICONS
from ui.control_panel import ControlPanel
from ui.status_panel import StatusPanel
from ui.waveform_widget import WaveformWidget
from ui.calibration_dialog import CalibrationDialog


class NavButton(QPushButton):
    """导航按钮"""
    def __init__(self, icon: str, tooltip: str = "", parent=None):
        super().__init__(parent)
        self.setObjectName("nav_btn")
        self.setText(icon)
        self.setToolTip(tooltip)
        self.setFixedSize(50, 50)
        self.setCheckable(True)
        self.setCursor(Qt.CursorShape.PointingHandCursor)


class TopBar(QFrame):
    """顶部连接栏 - 固定不变"""

    connect_clicked = pyqtSignal(str, int)
    disconnect_clicked = pyqtSignal()
    refresh_clicked = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("topbar")
        self.setFixedHeight(50)
        self._setup_ui()

    def _setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 8, 16, 8)
        layout.setSpacing(12)

        # Logo
        logo = QLabel("FOC")
        logo.setStyleSheet("font-size: 18px; font-weight: bold; color: #00d4aa;")
        layout.addWidget(logo)

        layout.addSpacing(20)

        # 分隔线
        sep = QFrame()
        sep.setFixedWidth(1)
        sep.setStyleSheet("background-color: rgba(255,255,255,30);")
        layout.addWidget(sep)

        layout.addSpacing(10)

        # 端口选择
        layout.addWidget(QLabel("端口:"))
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(180)
        self.port_combo.setMaximumWidth(200)
        layout.addWidget(self.port_combo)

        # 刷新按钮
        self.refresh_btn = QPushButton("⟳")
        self.refresh_btn.setFixedSize(32, 28)
        self.refresh_btn.setToolTip("刷新端口")
        self.refresh_btn.clicked.connect(self._on_refresh)
        layout.addWidget(self.refresh_btn)

        # 波特率
        layout.addWidget(QLabel("波特率:"))
        self.baud_combo = QComboBox()
        self.baud_combo.setFixedWidth(100)
        self.baud_combo.addItems(["9600", "19200", "38400", "57600", "115200"])
        self.baud_combo.setCurrentText("115200")
        layout.addWidget(self.baud_combo)

        layout.addStretch()

        # 状态指示
        self.status_indicator = QLabel("●")
        self.status_indicator.setStyleSheet("color: #ff5252; font-size: 14px;")
        layout.addWidget(self.status_indicator)

        self.status_label = QLabel("未连接")
        self.status_label.setStyleSheet("color: rgba(255,255,255,150);")
        layout.addWidget(self.status_label)

        layout.addSpacing(20)

        # 连接按钮
        self.connect_btn = QPushButton("连接")
        self.connect_btn.setObjectName("connect")
        self.connect_btn.setFixedSize(80, 32)
        self.connect_btn.clicked.connect(self._on_connect)
        layout.addWidget(self.connect_btn)

        self.disconnect_btn = QPushButton("断开")
        self.disconnect_btn.setObjectName("disconnect")
        self.disconnect_btn.setFixedSize(80, 32)
        self.disconnect_btn.clicked.connect(self._on_disconnect)
        self.disconnect_btn.setVisible(False)
        layout.addWidget(self.disconnect_btn)

    def _on_refresh(self):
        self.refresh_clicked.emit()

    def _on_connect(self):
        port = self.port_combo.currentText()
        baudrate = int(self.baud_combo.currentText())
        self.connect_clicked.emit(port, baudrate)

    def _on_disconnect(self):
        self.disconnect_clicked.emit()

    def update_ports(self, ports: list):
        self.port_combo.clear()
        for port in ports:
                self.port_combo.addItem(port)
        if ports:
            self.port_combo.setCurrentIndex(0)

    def set_connected(self, connected: bool):
        if connected:
            self.connect_btn.setVisible(False)
            self.disconnect_btn.setVisible(True)
            self.port_combo.setEnabled(False)
            self.baud_combo.setEnabled(False)
            self.status_indicator.setStyleSheet("color: #00d4aa; font-size: 14px;")
            self.status_label.setText("已连接")
            self.status_label.setStyleSheet("color: #00d4aa;")
        else:
            self.connect_btn.setVisible(True)
            self.disconnect_btn.setVisible(False)
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
            self.status_indicator.setStyleSheet("color: #ff5252; font-size: 14px;")
            self.status_label.setText("未连接")
            self.status_label.setStyleSheet("color: rgba(255,255,255,150);")


class SidebarWidget(QFrame):
    """左侧导航栏"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("sidebar")
        self.setFixedWidth(60)
        self._setup_ui()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # 导航按钮
        self.btn_monitor = NavButton(SIDEBAR_ICONS['target'], "监控")
        self.btn_monitor.setChecked(True)
        layout.addWidget(self.btn_monitor)

        self.btn_waveform = NavButton(SIDEBAR_ICONS['play'], "波形")
        layout.addWidget(self.btn_waveform)

        self.btn_config = NavButton(SIDEBAR_ICONS['settings'], "配置")
        layout.addWidget(self.btn_config)

        layout.addStretch()

        # 底部设置按钮
        self.btn_settings = NavButton(SIDEBAR_ICONS['settings'], "设置")
        layout.addWidget(self.btn_settings)


class MainWindow(QMainWindow):
    """主窗口 - 玻璃拟态风格"""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("FOC Motor Controller")
        self.setMinimumSize(1280, 800)
        self.resize(1400, 900)

        # 设置全局字体
        font = QFont("Segoe UI", 10)
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
        self.status_timer.start(1000)

    def _setup_ui(self):
        # 主容器
        central = QWidget()
        self.setCentralWidget(central)

        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        # 顶部连接栏 - 固定不变
        self.top_bar = TopBar()
        self.top_bar.setStyleSheet("""
            QFrame#topbar {
                background-color: rgba(20, 20, 20, 230);
                border-bottom: 1px solid rgba(50, 50, 50, 100);
            }
        """)
        main_layout.addWidget(self.top_bar)

        # 下部区域（侧边栏 + 内容）
        content_area = QWidget()
        content_layout = QHBoxLayout(content_area)
        content_layout.setContentsMargins(0, 0, 0, 0)
        content_layout.setSpacing(0)

        # 左侧导航栏
        self.sidebar = SidebarWidget()
        content_layout.addWidget(self.sidebar)

        # 右侧主内容区
        self.content = QFrame()
        self.content.setObjectName("content")
        content_inner_layout = QVBoxLayout(self.content)
        content_inner_layout.setContentsMargins(16, 16, 16, 16)
        content_inner_layout.setSpacing(16)

        # 堆栈窗口
        self.stack = QStackedWidget()
        content_inner_layout.addWidget(self.stack)

        content_layout.addWidget(self.content, 1)
        main_layout.addWidget(content_area, 1)

        # 创建监控页面
        monitor_page = self._create_monitor_page()
        self.stack.addWidget(monitor_page)

        # 创建波形页面
        self.waveform_widget = WaveformWidget(self.data_model)
        self.stack.addWidget(self.waveform_widget)

        # 连接导航按钮
        self.sidebar.btn_monitor.clicked.connect(lambda: self.stack.setCurrentIndex(0))
        self.sidebar.btn_waveform.clicked.connect(lambda: self.stack.setCurrentIndex(1))

        # 刷新端口列表
        self._refresh_ports()

    def _create_monitor_page(self) -> QWidget:
        page = QWidget()
        layout = QHBoxLayout(page)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(16)

        # 左侧控制面板
        left_panel = QFrame()
        left_panel.setStyleSheet("""
            QFrame {
                background-color: rgba(30, 30, 30, 150);
                border-radius: 12px;
                border: 1px solid rgba(50, 50, 50, 100);
            }
        """)
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(12, 12, 12, 12)
        left_layout.setSpacing(12)

        # 控制面板
        self.control_panel = ControlPanel()
        self.control_panel.set_enabled(False)
        left_layout.addWidget(self.control_panel)

        # 状态面板
        self.status_panel = StatusPanel()
        left_layout.addWidget(self.status_panel)

        left_layout.addStretch()

        layout.addWidget(left_panel, 1)

        # 右侧信息区
        right_panel = QFrame()
        right_panel.setStyleSheet("""
            QFrame {
                background-color: rgba(30, 30, 30, 150);
                border-radius: 12px;
                border: 1px solid rgba(50, 50, 50, 100);
            }
        """)
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(12, 12, 12, 12)

        # 快速状态显示
        quick_status = self._create_quick_status()
        right_layout.addWidget(quick_status)

        layout.addWidget(right_panel, 2)

        return page

    def _create_quick_status(self) -> QFrame:
        frame = QFrame()
        layout = QVBoxLayout(frame)
        layout.setSpacing(8)

        # 标题
        title = QLabel("实时状态")
        title.setObjectName("title")
        title.setStyleSheet("font-size: 14px;")
        layout.addWidget(title)

        # 状态项
        self.quick_labels = {}
        items = [
            ("state", "状态机", ""),
            ("id_current", "Id", "A"),
            ("iq_current", "Iq", "A"),
            ("velocity", "速度", "rad/s"),
            ("position", "位置", "rad"),
            ("voltage", "电压", "V"),
            ("temperature", "温度", "°C"),
        ]

        for key, name, unit in items:
            row = QFrame()
            row.setStyleSheet("background: transparent;")
            row_layout = QHBoxLayout(row)
            row_layout.setContentsMargins(0, 0, 0, 0)

            label = QLabel(name)
            label.setStyleSheet("color: rgba(255,255,255,150); font-size: 11px;")
            row_layout.addWidget(label)

            row_layout.addStretch()

            value_label = QLabel("--")
            value_label.setObjectName("value")
            value_label.setStyleSheet("font-size: 12px;")
            self.quick_labels[key] = value_label
            row_layout.addWidget(value_label)

            if unit:
                unit_label = QLabel(unit)
                unit_label.setObjectName("unit")
                row_layout.addWidget(unit_label)

            layout.addWidget(row)

        return frame

    def _connect_signals(self):
        # 顶部连接栏信号
        self.top_bar.connect_clicked.connect(self._on_connect)
        self.top_bar.disconnect_clicked.connect(self._on_disconnect)
        self.top_bar.refresh_clicked.connect(self._refresh_ports)

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
        self.controller.ping_timeout.connect(self._on_ping_timeout)

    def _refresh_ports(self):
        ports = self.controller.serial.get_available_ports()
        self.top_bar.update_ports(ports)

    def _on_connect(self, port: str, baudrate: int):
        if self.controller.connect(port, baudrate):
            self.top_bar.set_connected(True)

    def _on_disconnect(self):
        self.controller.disconnect()
        self.top_bar.set_connected(False)

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

    def _on_ping_timeout(self):
        QMessageBox.warning(self, "警告", "设备无响应: 连续 3 次 PING 超时\n请检查设备连接状态")
        self._on_disconnect()

    def _request_status(self):
        if self.controller.is_connected():
            self.controller.get_status()

    def _on_status_updated(self, status: dict):
        self.data_model.update_from_status(status)
        self.status_panel.update_status(self.data_model.__dict__)
        self._update_quick_status(status)

    def _update_quick_status(self, status: dict):
        """更新快速状态显示"""
        state_names = {0: "IDLE", 1: "ALIGN", 2: "CALIB", 3: "RUN", 4: "HALL"}

        if "state" in self.quick_labels:
            state = status.get("state", 0)
            self.quick_labels["state"].setText(state_names.get(state, str(state)))

        if "id_current" in self.quick_labels:
            self.quick_labels["id_current"].setText(f"{status.get('id_current', 0):.2f}")

        if "iq_current" in self.quick_labels:
            self.quick_labels["iq_current"].setText(f"{status.get('iq_current', 0):.2f}")

        if "velocity" in self.quick_labels:
            self.quick_labels["velocity"].setText(f"{status.get('velocity', 0):.2f}")

        if "position" in self.quick_labels:
            self.quick_labels["position"].setText(f"{status.get('position', 0):.2f}")

        if "voltage" in self.quick_labels:
            self.quick_labels["voltage"].setText(f"{status.get('voltage', 0):.1f}")

        if "temperature" in self.quick_labels:
            self.quick_labels["temperature"].setText(f"{status.get('temperature', 0)}")

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
