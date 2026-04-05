#!/usr/bin/env python3
"""
FOC Motor Driver Host PC Software
基于PyQt6的FOC电机驱动上位机 - 玻璃拟态风格

使用方法:
    python main.py

依赖:
    pip install -r requirements.txt
"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt

from ui.main_window_glass import MainWindow
from ui.styles_glass import MAIN_STYLE


def main():
    # 启用高DPI支持
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    app = QApplication(sys.argv)
    app.setApplicationName("FOC Motor Controller")
    app.setApplicationVersion("1.0.0")

    # 应用玻璃拟态风格样式
    app.setStyleSheet(MAIN_STYLE)

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
