"""
Oscilloscope Style - 专业示波器风格
Inspired by professional motor control debug tools
"""

MAIN_STYLE = """
QMainWindow {
    background-color: #0a0a0a;
    color: #ffffff;
}

QWidget {
    background-color: #0f0f0f;
    color: #ffffff;
    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
    font-size: 10px;
}

/* ===== 控制面板 ===== */
QGroupBox {
    border: 1px solid #2a2a2a;
    border-radius: 6px;
    margin-top: 8px;
    padding-top: 10px;
    font-weight: 600;
    color: #00d8ff;
    background-color: #141414;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 8px;
    color: #00d8ff;
}

/* ===== 按钮样式 ===== */
QPushButton {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #1a5f7a, stop:1 #0d3d4d);
    color: white;
    border: 1px solid #2a7f9f;
    border-radius: 4px;
    padding: 8px 16px;
    font-weight: 600;
    min-width: 80px;
}

QPushButton:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #2a7f9f, stop:1 #1a5f7a);
    border: 1px solid #3a9fbf;
}

QPushButton:pressed {
    background: #0d3d4d;
    border: 1px solid #2a7f9f;
}

QPushButton:disabled {
    background: #2a2a2a;
    color: #6a6a6a;
    border: 1px solid #3a3a3a;
}

QPushButton#connect {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #2e7d32, stop:1 #1b5e20);
    border: 1px solid #4caf50;
}

QPushButton#connect:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #4caf50, stop:0.5 #388e3c, stop:1 #2e7d32);
    border: 1px solid #66bb6a;
}

QPushButton#disconnect {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #c62828, stop:1 #8e0000);
    border: 1px solid #ef5350;
}

QPushButton#disconnect:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #ef5350, stop:0.5 #e53935, stop:1 #c62828);
    border: 1px solid #ff6b6b;
}

QPushButton#stop {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #f57c00, stop:1 #e65100);
    border: 1px solid #ff9800;
    font-weight: bold;
    color: #000000;
}

QPushButton#stop:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #ff9800, stop:1 #f57c00);
    border: 1px solid #ffb74d;
}

QPushButton#calibrate {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #7b1fa2, stop:1 #4a148c);
    border: 1px solid #9c27b0;
}

QPushButton#calibrate:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #9c27b0, stop:1 #7b1fa2);
    border: 1px solid #ba68c8;
}

/* ===== 输入控件 ===== */
QLineEdit, QDoubleSpinBox, QSpinBox {
    background-color: #1a1a1a;
    color: #ffffff;
    border: 1px solid #3a3a3a;
    border-radius: 3px;
    padding: 5px 8px;
    font-size: 11px;
    font-family: 'Consolas', 'Courier New', monospace;
}

QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
    border: 1px solid #00d8ff;
    background-color: #202020;
}

QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {
    width: 16px;
    border: 1px solid #3a3a3a;
    background-color: #2a2a2a;
}

QDoubleSpinBox::up-button:hover, QSpinBox::up-button:hover,
QDoubleSpinBox::down-button:hover, QSpinBox::down-button:hover {
    background-color: #3a3a3a;
}

QComboBox {
    background-color: #1a1a1a;
    color: #ffffff;
    border: 1px solid #3a3a3a;
    border-radius: 3px;
    padding: 5px 8px;
    min-width: 120px;
}

QComboBox:focus {
    border: 1px solid #00d8ff;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 7px solid #00d8ff;
    margin-right: 6px;
}

QComboBox QAbstractItemView {
    background-color: #1a1a1a;
    border: 1px solid #3a3a3a;
    selection-background-color: #2a4a5a;
    color: #ffffff;
    outline: none;
}

QComboBox QAbstractItemView::item {
    min-height: 30px;
    padding: 4px 8px;
}

QComboBox QAbstractItemView::item:hover {
    background-color: #2a4a5a;
}

QComboBox QAbstractItemView::item:selected {
    background-color: #0d3d4d;
    color: #ffffff;
}

/* ===== 标签样式 ===== */
QLabel {
    color: #b0b0b0;
}

QLabel#title {
    color: #00d8ff;
    font-size: 13px;
    font-weight: 600;
    background-color: #141414;
    padding: 6px 10px;
    border-radius: 4px;
}

QLabel#value {
    color: #00ff88;
    font-weight: 600;
    font-size: 13px;
    font-family: 'Consolas', 'Courier New', monospace;
    background-color: #0a0a0a;
    padding: 4px 8px;
    border-radius: 3px;
    border: 1px solid #1a3a2a;
}

QLabel#unit {
    color: #8a8a8a;
    font-size: 9px;
}

QLabel#status_connected {
    color: #00ff88;
    font-weight: 600;
    background-color: #0a2a1a;
    padding: 4px 12px;
    border-radius: 4px;
    border: 1px solid #00ff88;
}

QLabel#status_disconnected {
    color: #ff6b6b;
    font-weight: 600;
    background-color: #2a0a0a;
    padding: 4px 12px;
    border-radius: 4px;
    border: 1px solid #ff6b6b;
}

/* ===== 波形图表区域 ===== */
QFrame#waveform_container {
    background-color: #050505;
    border: 2px solid #2a2a2a;
    border-radius: 8px;
}

/* ===== 进度条 ===== */
QProgressBar {
    border: 1px solid #2a2a2a;
    border-radius: 4px;
    text-align: center;
    color: #ffffff;
    font-weight: 600;
    background-color: #1a1a1a;
    height: 20px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #00d8ff, stop:0.5 #00ff88, stop:1 #00d8ff);
    border-radius: 3px;
}

/* ===== 滑块 ===== */
QSlider::groove:horizontal {
    height: 6px;
    background: #2a2a2a;
    border-radius: 3px;
}

QSlider::handle:horizontal {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #00d8ff, stop:1 #0088aa);
    width: 18px;
    height: 18px;
    margin: -6px 0;
    border-radius: 9px;
    border: 2px solid #ffffff;
}

QSlider::handle:horizontal:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 #00ff88, stop:1 #00cc6a);
}

QSlider::sub-page:horizontal {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #00d8ff, stop:1 #0088aa);
    border-radius: 3px;
}

/* ===== 分割线 ===== */
QSplitter::handle {
    background-color: #2a2a2a;
    border: 1px solid #3a3a3a;
}

QSplitter::handle:horizontal {
    width: 4px;
}

QSplitter::handle:vertical {
    height: 4px;
}

/* ===== 选项卡 ===== */
QTabWidget::pane {
    border: 1px solid #2a2a2a;
    background-color: #141414;
    border-radius: 4px;
}

QTabBar::tab {
    background-color: #1a1a1a;
    color: #8a8a8a;
    padding: 10px 20px;
    border: 1px solid #2a2a2a;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
    font-weight: 500;
}

QTabBar::tab:selected {
    background-color: #0d3d4d;
    color: #00d8ff;
    border: 1px solid #00d8ff;
    border-bottom: none;
}

QTabBar::tab:hover:!selected {
    background-color: #2a2a2a;
    color: #b0b0b0;
}

/* ===== 滚动条 ===== */
QScrollBar:vertical {
    background-color: #0a0a0a;
    width: 14px;
    border-radius: 7px;
    border: 1px solid #1a1a1a;
}

QScrollBar::handle:vertical {
    background-color: #3a3a3a;
    min-height: 30px;
    border-radius: 6px;
    margin: 1px;
}

QScrollBar::handle:vertical:hover {
    background-color: #5a5a5a;
}

QScrollBar::handle:vertical:pressed {
    background-color: #00d8ff;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}

QScrollBar:horizontal {
    background-color: #0a0a0a;
    height: 14px;
    border-radius: 7px;
    border: 1px solid #1a1a1a;
}

QScrollBar::handle:horizontal {
    background-color: #3a3a3a;
    min-width: 30px;
    border-radius: 6px;
    margin: 1px;
}

QScrollBar::handle:horizontal:hover {
    background-color: #5a5a5a;
}

QScrollBar::handle:horizontal:pressed {
    background-color: #00d8ff;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}

/* ===== 菜单 ===== */
QMenuBar {
    background-color: #0f0f0f;
    border-bottom: 1px solid #2a2a2a;
    padding: 4px;
}

QMenuBar::item {
    padding: 6px 12px;
    color: #b0b0b0;
    border-radius: 4px;
}

QMenuBar::item:selected {
    background-color: #0d3d4d;
    color: #00d8ff;
}

QMenu {
    background-color: #1a1a1a;
    border: 1px solid #3a3a3a;
    border-radius: 6px;
    padding: 6px;
}

QMenu::item {
    padding: 8px 24px;
    color: #e0e0e0;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: #0d3d4d;
    color: #00d8ff;
}

QMenu::separator {
    height: 1px;
    background-color: #3a3a3a;
    margin: 6px 0;
}

/* ===== 对话框 ===== */
QDialog {
    background-color: #141414;
}

QMessageBox {
    background-color: #141414;
}

QMessageBox QLabel {
    color: #ffffff;
    padding: 10px;
}

QMessageBox QPushButton {
    min-width: 80px;
}

/* ===== 工具提示 ===== */
QToolTip {
    background-color: #0d3d4d;
    color: #ffffff;
    border: 1px solid #00d8ff;
    border-radius: 4px;
    padding: 6px 10px;
    font-size: 10px;
}

/* ===== 复选框和单选框 ===== */
QCheckBox, QRadioButton {
    color: #b0b0b0;
    spacing: 8px;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #3a3a3a;
    border-radius: 4px;
    background-color: #1a1a1a;
}

QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: #00d8ff;
    border: 2px solid #00d8ff;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border: 2px solid #00d8ff;
}

QRadioButton::indicator {
    border-radius: 9px;
}

/* ===== 表格 ===== */
QTableWidget {
    background-color: #141414;
    alternate-background-color: #1a1a1a;
    border: 1px solid #2a2a2a;
    border-radius: 4px;
    gridline-color: #2a2a2a;
    selection-background-color: #0d3d4d;
}

QTableWidget::item {
    padding: 6px;
    color: #e0e0e0;
}

QTableWidget::item:selected {
    background-color: #0d3d4d;
    color: #00d8ff;
}

QHeaderView::section {
    background-color: #1a1a1a;
    color: #00d8ff;
    padding: 8px;
    border: 1px solid #2a2a2a;
    font-weight: 600;
}

/* ===== 状态栏 ===== */
QStatusBar {
    background-color: #0f0f0f;
    border-top: 1px solid #2a2a2a;
    color: #8a8a8a;
    font-size: 10px;
}

QStatusBar::item {
    border: none;
}
"""

PLOT_STYLE = {
    'background': '#050505',
    'foreground': '#ffffff',
    'axis_color': '#4a4a4a',
    'grid_color': '#1a1a1a',
    'grid_alpha': 0.5,
    'cursor_color': '#00d8ff',
    'colors': [
        '#00ff88',  # 亮绿色 - Id
        '#00d8ff',  # 青色 - Iq
        '#ff9800',  # 橙色 - 速度
        '#ff6b6b',  # 红色 - 位置
        '#ba68c8',  # 紫色 - 电角度
        '#ffd700',  # 金色 - 电压
    ],
    'line_width': 2,
    'antialias': True,
}

# 波形通道配置
WAVEFORM_CHANNELS = {
    'current': {
        'title': 'Current (A)',
        'color': '#00ff88',
        'grid_visible': True,
    },
    'velocity': {
        'title': 'Velocity (rad/s)',
        'color': '#ff9800',
        'grid_visible': True,
    },
    'position': {
        'title': 'Position (rad)',
        'color': '#ff6b6b',
        'grid_visible': True,
    },
}
