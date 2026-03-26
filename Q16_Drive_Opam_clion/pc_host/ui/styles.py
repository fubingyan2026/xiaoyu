"""
Minimalist Stylesheet
简约风格样式表
"""

MAIN_STYLE = """
QMainWindow {
    background-color: #1e1e1e;
    color: #e0e0e0;
}

QWidget {
    background-color: #252526;
    color: #e0e0e0;
    font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;
    font-size: 11px;
}

QGroupBox {
    border: 1px solid #3c3c3c;
    border-radius: 4px;
    margin-top: 10px;
    padding-top: 10px;
    font-weight: bold;
    color: #4ec9b0;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 5px;
}

QPushButton {
    background-color: #0e639c;
    color: white;
    border: none;
    border-radius: 3px;
    padding: 6px 12px;
    min-width: 60px;
}

QPushButton:hover {
    background-color: #1177bb;
}

QPushButton:pressed {
    background-color: #094771;
}

QPushButton:disabled {
    background-color: #3c3c3c;
    color: #808080;
}

QPushButton#danger {
    background-color: #c75450;
}

QPushButton#danger:hover {
    background-color: #d9706c;
}

QPushButton#success {
    background-color: #4ec9b0;
}

QPushButton#success:hover {
    background-color: #73d4c0;
}

QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: #3c3c3c;
    color: #e0e0e0;
    border: 1px solid #5a5a5a;
    border-radius: 3px;
    padding: 4px;
}

QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #0e639c;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 6px solid #e0e0e0;
}

QLabel {
    color: #e0e0e0;
}

QLabel#value {
    color: #4ec9b0;
    font-weight: bold;
    font-size: 12px;
}

QLabel#title {
    color: #569cd6;
    font-size: 14px;
    font-weight: bold;
}

QLabel#status_connected {
    color: #4ec9b0;
}

QLabel#status_disconnected {
    color: #c75450;
}

QProgressBar {
    border: 1px solid #3c3c3c;
    border-radius: 3px;
    text-align: center;
    color: #e0e0e0;
}

QProgressBar::chunk {
    background-color: #0e639c;
    border-radius: 2px;
}

QSlider::groove:horizontal {
    height: 4px;
    background: #3c3c3c;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: #0e639c;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}

QSlider::sub-page:horizontal {
    background: #0e639c;
    border-radius: 2px;
}

QTabWidget::pane {
    border: 1px solid #3c3c3c;
    background-color: #252526;
}

QTabBar::tab {
    background-color: #2d2d30;
    color: #e0e0e0;
    padding: 8px 16px;
    border: none;
    margin-right: 2px;
}

QTabBar::tab:selected {
    background-color: #0e639c;
}

QTabBar::tab:hover:!selected {
    background-color: #3c3c3c;
}

QScrollBar:vertical {
    background-color: #1e1e1e;
    width: 12px;
    border-radius: 6px;
}

QScrollBar::handle:vertical {
    background-color: #5a5a5a;
    min-height: 20px;
    border-radius: 6px;
}

QScrollBar::handle:vertical:hover {
    background-color: #808080;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QMenu {
    background-color: #252526;
    border: 1px solid #3c3c3c;
    padding: 5px;
}

QMenu::item {
    padding: 5px 20px;
    color: #e0e0e0;
}

QMenu::item:selected {
    background-color: #0e639c;
}

QMenu::separator {
    height: 1px;
    background-color: #3c3c3c;
    margin: 5px 0;
}

QDialog {
    background-color: #252526;
}
"""

PLOT_STYLE = {
    'background': '#1e1e1e',
    'foreground': '#e0e0e0',
    'axis_color': '#5a5a5a',
    'grid_color': '#3c3c3c',
    'colors': [
        '#4ec9b0',  # cyan
        '#569cd6',  # blue
        '#ce9178',  # orange
        '#b5cea8',  # green
        '#c586c0',  # purple
        '#dcdcaa',  # yellow
    ]
}
