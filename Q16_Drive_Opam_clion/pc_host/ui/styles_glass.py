"""
Glassmorphism Style - 现代玻璃拟态风格
Dark mode, minimalist, semi-transparent design
"""

MAIN_STYLE = """
/* ===== 主窗口 - 圆角 + 半透明 ===== */
QMainWindow {
    background-color: rgba(18, 18, 18, 230);
    border-radius: 12px;
}

QWidget {
    background-color: transparent;
    color: #ffffff;
    font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;
    font-size: 11px;
}

/* ===== 左侧导航栏 ===== */
QFrame#sidebar {
    background-color: #000000;
    border-top-left-radius: 12px;
    border-bottom-left-radius: 12px;
    border-right: 1px solid #1a1a1a;
}

/* ===== 右侧主内容区 ===== */
QFrame#content {
    background-color: rgba(30, 30, 30, 200);
    border-top-right-radius: 12px;
    border-bottom-right-radius: 12px;
}

/* ===== 分组框 - 玻璃效果 ===== */
QGroupBox {
    background-color: rgba(40, 40, 40, 150);
    border: 1px solid rgba(60, 60, 60, 100);
    border-radius: 10px;
    margin-top: 12px;
    padding-top: 12px;
    font-weight: 500;
    color: #00d4aa;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 14px;
    padding: 0 8px;
    color: #00d4aa;
    font-size: 12px;
}

/* ===== 按钮样式 - 极简扁平 ===== */
QPushButton {
    background-color: transparent;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    padding: 10px 20px;
    font-weight: 500;
    min-width: 90px;
}

QPushButton:hover {
    background-color: rgba(0, 212, 170, 30);
    color: #00d4aa;
}

QPushButton:pressed {
    background-color: rgba(0, 212, 170, 50);
}

QPushButton:disabled {
    color: #4a4a4a;
}

QPushButton#connect {
    background-color: rgba(0, 212, 170, 80);
    color: #000000;
    font-weight: 600;
}

QPushButton#connect:hover {
    background-color: rgba(0, 212, 170, 150);
}

QPushButton#disconnect {
    background-color: rgba(255, 82, 82, 80);
    color: #ffffff;
    font-weight: 600;
}

QPushButton#disconnect:hover {
    background-color: rgba(255, 82, 82, 150);
}

QPushButton#stop {
    background-color: rgba(255, 152, 0, 80);
    color: #000000;
    font-weight: 600;
}

QPushButton#stop:hover {
    background-color: rgba(255, 152, 0, 150);
}

QPushButton#calibrate {
    background-color: rgba(156, 39, 176, 80);
    color: #ffffff;
}

QPushButton#calibrate:hover {
    background-color: rgba(156, 39, 176, 150);
}

/* 导航按钮 */
QPushButton#nav_btn {
    background-color: transparent;
    color: #ffffff;
    border: none;
    border-radius: 0;
    padding: 16px;
    min-width: 0;
    font-size: 18px;
}

QPushButton#nav_btn:hover {
    background-color: rgba(255, 255, 255, 10);
    color: #00d4aa;
}

QPushButton#nav_btn:checked,
QPushButton#nav_btn#active {
    background-color: rgba(0, 212, 170, 20);
    color: #00d4aa;
    border-left: 3px solid #00d4aa;
}

/* ===== 输入控件 - 玻璃效果 ===== */
QLineEdit, QDoubleSpinBox, QSpinBox {
    background-color: rgba(30, 30, 30, 200);
    color: #ffffff;
    border: 1px solid rgba(60, 60, 60, 100);
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 11px;
    font-family: 'Consolas', 'Courier New', monospace;
}

QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
    border: 1px solid #00d4aa;
    background-color: rgba(40, 40, 40, 200);
}

QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {
    width: 20px;
    border: none;
    background-color: transparent;
}

QDoubleSpinBox::up-arrow, QSpinBox::up-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-bottom: 6px solid #00d4aa;
}

QDoubleSpinBox::down-arrow, QSpinBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 6px solid #00d4aa;
}

QComboBox {
    background-color: rgba(30, 30, 30, 200);
    color: #ffffff;
    border: 1px solid rgba(60, 60, 60, 100);
    border-radius: 6px;
    padding: 8px 12px;
    min-width: 140px;
}

QComboBox:focus {
    border: 1px solid #00d4aa;
}

QComboBox::drop-down {
    border: none;
    width: 28px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 7px solid #00d4aa;
}

QComboBox QAbstractItemView {
    background-color: rgba(30, 30, 30, 240);
    border: 1px solid rgba(60, 60, 60, 100);
    border-radius: 6px;
    selection-background-color: rgba(0, 212, 170, 50);
    color: #ffffff;
    outline: none;
    padding: 4px;
}

QComboBox QAbstractItemView::item {
    min-height: 32px;
    padding: 6px 12px;
    border-radius: 4px;
}

QComboBox QAbstractItemView::item:hover {
    background-color: rgba(0, 212, 170, 30);
}

QComboBox QAbstractItemView::item:selected {
    background-color: rgba(0, 212, 170, 50);
}

/* ===== 标签样式 ===== */
QLabel {
    color: rgba(255, 255, 255, 180);
    background-color: transparent;
}

QLabel#title {
    color: #ffffff;
    font-size: 14px;
    font-weight: 600;
    padding: 8px 0;
}

QLabel#value {
    color: #00d4aa;
    font-weight: 600;
    font-size: 13px;
    font-family: 'Consolas', 'Courier New', monospace;
    background-color: rgba(0, 212, 170, 20);
    padding: 6px 12px;
    border-radius: 6px;
}

QLabel#unit {
    color: rgba(255, 255, 255, 100);
    font-size: 9px;
}

QLabel#status_connected {
    color: #00d4aa;
    font-weight: 600;
    background-color: rgba(0, 212, 170, 30);
    padding: 6px 16px;
    border-radius: 6px;
}

QLabel#status_disconnected {
    color: #ff5252;
    font-weight: 600;
    background-color: rgba(255, 82, 82, 30);
    padding: 6px 16px;
    border-radius: 6px;
}

/* Logo 标签 */
QLabel#logo {
    color: #ffffff;
    font-family: 'Brush Script MT', 'Segoe UI', cursive;
    font-size: 24px;
    font-weight: normal;
    padding: 20px 0;
}

/* ===== 波形图表区域 ===== */
QFrame#waveform_container {
    background-color: rgba(10, 10, 10, 200);
    border: 1px solid rgba(40, 40, 40, 100);
    border-radius: 10px;
}

/* ===== 进度条 ===== */
QProgressBar {
    border: none;
    border-radius: 4px;
    text-align: center;
    color: #ffffff;
    font-weight: 500;
    background-color: rgba(40, 40, 40, 150);
    height: 8px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #00d4aa, stop:1 #00a080);
    border-radius: 4px;
}

/* ===== 滑块 ===== */
QSlider::groove:horizontal {
    height: 4px;
    background: rgba(60, 60, 60, 150);
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: #00d4aa;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}

QSlider::handle:horizontal:hover {
    background: #00ffcc;
}

QSlider::sub-page:horizontal {
    background: #00d4aa;
    border-radius: 2px;
}

/* ===== 分割器 ===== */
QSplitter::handle {
    background-color: rgba(40, 40, 40, 100);
}

QSplitter::handle:horizontal {
    width: 1px;
}

QSplitter::handle:vertical {
    height: 1px;
}

/* ===== 选项卡 ===== */
QTabWidget::pane {
    border: none;
    background-color: transparent;
}

QTabBar::tab {
    background-color: transparent;
    color: rgba(255, 255, 255, 150);
    padding: 12px 24px;
    border: none;
    border-bottom: 2px solid transparent;
    font-weight: 500;
}

QTabBar::tab:selected {
    color: #00d4aa;
    border-bottom: 2px solid #00d4aa;
}

QTabBar::tab:hover:!selected {
    color: rgba(255, 255, 255, 220);
}

/* ===== 滚动条 ===== */
QScrollBar:vertical {
    background-color: transparent;
    width: 8px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background-color: rgba(80, 80, 80, 150);
    min-height: 40px;
    border-radius: 4px;
}

QScrollBar::handle:vertical:hover {
    background-color: rgba(0, 212, 170, 100);
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}

QScrollBar:horizontal {
    background-color: transparent;
    height: 8px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background-color: rgba(80, 80, 80, 150);
    min-width: 40px;
    border-radius: 4px;
}

QScrollBar::handle:horizontal:hover {
    background-color: rgba(0, 212, 170, 100);
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}

/* ===== 列表视图 ===== */
QListView, QListWidget {
    background-color: transparent;
    border: none;
    outline: none;
}

QListView::item, QListWidget::item {
    background-color: transparent;
    color: #ffffff;
    padding: 12px 16px;
    border-radius: 6px;
    margin: 2px 4px;
}

QListView::item:hover, QListWidget::item:hover {
    background-color: rgba(50, 50, 50, 150);
}

QListView::item:selected, QListWidget::item:selected {
    background-color: rgba(60, 60, 60, 200);
    color: #00d4aa;
}

/* ===== 菜单 ===== */
QMenuBar {
    background-color: transparent;
    border: none;
    padding: 0;
}

QMenuBar::item {
    padding: 8px 16px;
    color: rgba(255, 255, 255, 180);
    border-radius: 4px;
}

QMenuBar::item:selected {
    background-color: rgba(0, 212, 170, 30);
    color: #00d4aa;
}

QMenu {
    background-color: rgba(30, 30, 30, 240);
    border: 1px solid rgba(60, 60, 60, 100);
    border-radius: 8px;
    padding: 8px;
}

QMenu::item {
    padding: 10px 24px;
    color: #ffffff;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: rgba(0, 212, 170, 30);
    color: #00d4aa;
}

QMenu::separator {
    height: 1px;
    background-color: rgba(60, 60, 60, 100);
    margin: 8px 0;
}

/* ===== 对话框 ===== */
QDialog {
    background-color: rgba(30, 30, 30, 240);
    border-radius: 12px;
}

QMessageBox {
    background-color: rgba(30, 30, 30, 240);
}

QMessageBox QLabel {
    color: #ffffff;
    padding: 12px;
}

/* ===== 工具提示 ===== */
QToolTip {
    background-color: rgba(30, 30, 30, 240);
    color: #ffffff;
    border: 1px solid rgba(0, 212, 170, 100);
    border-radius: 6px;
    padding: 8px 12px;
}

/* ===== 复选框 ===== */
QCheckBox {
    color: rgba(255, 255, 255, 180);
    spacing: 10px;
}

QCheckBox::indicator {
    width: 20px;
    height: 20px;
    border: 2px solid rgba(100, 100, 100, 150);
    border-radius: 4px;
    background-color: transparent;
}

QCheckBox::indicator:checked {
    background-color: #00d4aa;
    border: 2px solid #00d4aa;
}

QCheckBox::indicator:hover {
    border: 2px solid #00d4aa;
}

/* ===== 状态栏 ===== */
QStatusBar {
    background-color: transparent;
    border-top: 1px solid rgba(40, 40, 40, 100);
    color: rgba(255, 255, 255, 120);
    font-size: 10px;
}

QStatusBar::item {
    border: none;
}
"""

PLOT_STYLE = {
    'background': '#0a0a0a',
    'foreground': '#ffffff',
    'axis_color': '#505050',
    'grid_color': '#282828',
    'grid_alpha': 0.3,
    'cursor_color': '#00d4aa',
    'colors': [
        '#00d4aa',  # 青色 - Id
        '#00a0ff',  # 蓝色 - Iq
        '#ff9800',  # 橙色 - 速度
        '#ff5252',  # 红色 - 位置
        '#ab47bc',  # 紫色 - 电角度
        '#ffeb3b',  # 黄色 - 电压
    ],
    'line_width': 2,
    'antialias': True,
}

# 侧边栏图标 (Unicode 字符)
SIDEBAR_ICONS = {
    'logo': 'FOC',
    'folder': '📁',
    'search': '🔍',
    'target': '🎯',
    'play': '▶',
    'document': '📄',
    'list': '☰',
    'settings': '⚙',
}
