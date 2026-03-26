#!/usr/bin/env python3
"""Test script to verify all modules can be imported correctly"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def test_imports():
    print("Testing imports...")
    
    try:
        from core.protocol import S7Frame, MessageType, FunctionCode, ProtocolParser
        from core.serial_manager import SerialManager
        from core.controller import FOCController
        from core.models import MotorDataModel
        print("✓ Core modules imported successfully")
    except Exception as e:
        print(f"✗ Core import error: {e}")
        return False
    
    try:
        from ui.styles import MAIN_STYLE, PLOT_STYLE
        from ui.connection_panel import ConnectionPanel
        from ui.control_panel import ControlPanel
        from ui.status_panel import StatusPanel
        from ui.waveform_widget import WaveformWidget
        from ui.calibration_dialog import CalibrationDialog
        from ui.main_window import MainWindow
        print("✓ UI modules imported successfully")
    except Exception as e:
        print(f"✗ UI import error: {e}")
        return False
    
    print("\nAll modules imported successfully!")
    print("You can now run: python main.py")
    return True

if __name__ == "__main__":
    success = test_imports()
    sys.exit(0 if success else 1)
