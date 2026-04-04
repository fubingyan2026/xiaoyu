#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import time
import threading
import serial
import serial.tools.list_ports
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QTabWidget, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QMessageBox, QFileDialog, QGroupBox,
    QGridLayout, QLineEdit, QTextEdit, QProgressBar, QFrame, QSpacerItem, QSizePolicy
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt6.QtGui import QFont, QIcon, QColor, QPalette, QTextCursor, QCloseEvent

class SerialThread(QThread):
    """串口通信线程"""
    sent_data = pyqtSignal(str)
    received_data = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    connection_status = pyqtSignal(bool)
    
    def __init__(self, port, baudrate):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = False
        self.stop_event = threading.Event()
        # 添加命令队列和相关控制变量
        self.command_queue = []
        self.is_waiting_response = False
        self.command_lock = threading.Lock()
        # 添加接收缓冲区
        self.receive_buffer = bytearray()
        
    def run(self):
        self.running = True
        self.ser = None
        try:
            # 添加端口有效性检查
            if not self.port:
                error_msg = "端口未指定"
                self.error_occurred.emit(error_msg)
                print(error_msg)
                return
                
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=0.1,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            self.connection_status.emit(True)
            
            # 清空接收缓冲区
            self.receive_buffer = bytearray()
            last_receive_time = time.time()
            
            while self.running and not self.stop_event.is_set():
                try:
                    # 安全检查串口状态
                    if self.ser and self.ser.is_open:
                        # 避免直接访问in_waiting可能的异常
                        try:
                            waiting_bytes = self.ser.in_waiting
                        except:
                            waiting_bytes = 0
                            
                        if waiting_bytes > 0:
                            # 读取数据到缓冲区
                            data = self.ser.read(waiting_bytes)
                            self.receive_buffer.extend(data)
                            last_receive_time = time.time()
                            
                            # 检查是否是EF开头的完整帧（指纹模块的标准响应）
                            if self.receive_buffer and self.receive_buffer[0] == 0xEF:
                                # 等待更多数据或超时
                                continue
                        
                        # 检查是否超过100ms没有新数据到达，认为一帧数据接收完成
                        current_time = time.time()
                        if self.receive_buffer and (current_time - last_receive_time > 0.1):
                            # 安全转换为十六进制字符串
                            try:
                                hex_data = ' '.join([f'{b:02X}' for b in self.receive_buffer])
                                # 确保信号发送在主线程中
                                self.received_data.emit(hex_data)
                                # 收到完整响应后，处理下一个命令
                                with self.command_lock:
                                    self.is_waiting_response = False
                                    self._process_next_command()
                                # 清空缓冲区
                                self.receive_buffer = bytearray()
                            except Exception as hex_e:
                                error_msg = f"数据转换错误: {str(hex_e)}"
                                self.error_occurred.emit(error_msg)
                                print(error_msg)
                                # 清空缓冲区
                                self.receive_buffer = bytearray()
                    
                    # 如果没有等待响应且有命令在队列中，尝试处理下一个命令
                    with self.command_lock:
                        if not self.is_waiting_response and self.command_queue:
                            self._process_next_command()
                    
                    time.sleep(0.01)  # 提高轮询频率
                except Exception as e:
                    error_msg = f"读取数据错误: {str(e)}"
                    self.error_occurred.emit(error_msg)
                    print(error_msg)  # 打印错误信息以便调试
                    # 不要立即break，尝试继续运行
                    time.sleep(0.1)
                    continue
        except Exception as e:
            error_msg = f"串口连接错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            print(error_msg)  # 打印错误信息以便调试
            self.connection_status.emit(False)
        finally:
            if self.ser and self.ser.is_open:
                self.ser.close()
            self.connection_status.emit(False)
    
    def stop(self):
        try:
            self.stop_event.set()
            self.running = False
            # 清空缓冲区和队列
            if hasattr(self, 'receive_buffer'):
                self.receive_buffer = bytearray()
            with self.command_lock:
                self.command_queue = []
                self.is_waiting_response = False
            
            # 等待线程终止，但设置超时以避免死锁
            if self.isRunning():
                self.wait(1000)  # 1秒超时
            
            # 确保串口被关闭
            if hasattr(self, 'ser') and self.ser and self.ser.is_open:
                try:
                    # 关闭前清空串口缓冲区
                    if hasattr(self.ser, 'reset_input_buffer'):
                        self.ser.reset_input_buffer()
                    if hasattr(self.ser, 'reset_output_buffer'):
                        self.ser.reset_output_buffer()
                    self.ser.close()
                except:
                    pass
        except Exception as e:
            print(f"停止线程错误: {str(e)}")
    
    def _process_next_command(self):
        """处理队列中的下一个命令"""
        if not self.command_queue or self.is_waiting_response or not self.ser or not self.ser.is_open:
            return
        
        try:
            # 获取队列中的第一个命令
            command_str = self.command_queue.pop(0)
            
            # 将字符串转换为字节
            try:
                hex_list = command_str.split()
                byte_values = []
                for h in hex_list:
                    try:
                        byte_values.append(int(h, 16))
                    except ValueError:
                        error_msg = f"无效的十六进制值: {h}"
                        self.error_occurred.emit(error_msg)
                        print(error_msg)
                        # 继续处理下一个命令
                        self.is_waiting_response = False
                        return
                data = bytes(byte_values)
                
                # 发送数据
                self.ser.write(data)
                # 发送信号记录发送的数据
                self.sent_data.emit(command_str)
                # 设置等待响应标志
                self.is_waiting_response = True
            except Exception as parse_e:
                error_msg = f"命令解析错误: {str(parse_e)}"
                self.error_occurred.emit(error_msg)
                print(error_msg)
                # 继续处理下一个命令
                self.is_waiting_response = False
        except Exception as e:
            error_msg = f"处理命令错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            print(error_msg)
            self.is_waiting_response = False
    
    def send_command(self, command_str):
        """发送命令，命令格式为带空格的十六进制字符串"""
        try:
            # 基本参数检查
            if not command_str or not isinstance(command_str, str):
                error_msg = "无效的命令格式"
                self.error_occurred.emit(error_msg)
                print(error_msg)
                return False
                
            if not self.ser or not self.ser.is_open:
                error_msg = "串口未连接"
                self.error_occurred.emit(error_msg)
                print(error_msg)
                return False
            
            # 将命令添加到队列
            with self.command_lock:
                self.command_queue.append(command_str)
            
            return True
        except Exception as e:
            error_msg = f"发送命令错误: {str(e)}"
            self.error_occurred.emit(error_msg)
            print(error_msg)
            return False

class FingerprintModule:
    """指纹模组控制类"""
    
    # 基本指令
    CMD_WAKEUP = "EF 02 FF FF FF FF FF FF FF FF 00 00 23 03 01 01 00 00 3F 3B"
    CMD_SLEEP = "EF 02 FF FF FF FF FF FF FF FF 00 00 90 03 01 02 00 00 00 12 09"
    CMD_DELETE_ALL = "EF 02 FF FF FF FF FF FF FF FF 00 00 2A 03 01 00 00 9D 49"
    # 添加专门的指纹状态查询命令
    CMD_CHECK_FINGER_STATUS = "EF 02 FF FF FF FF FF FF FF FF 00 00 23 03 01 01 00 00 3F 3B"
    
    # 指纹状态
    STATUS_NO_FINGER = "00 04 00"
    STATUS_FINGER_PRESENT = "00 00 00"
    STATUS_FINGER_REMOVING = "00 20 00"
    
    def __init__(self, serial_thread):
        self.serial_thread = serial_thread
        self.current_finger_id = None
        self.is_recording = False
        self.record_step = 0
        
    def wakeup(self):
        """唤醒模块"""
        return self.serial_thread.send_command(self.CMD_WAKEUP)
    
    def sleep(self):
        """休眠模块"""
        return self.serial_thread.send_command(self.CMD_SLEEP)
    
    def delete_all_fingerprints(self):
        """删除所有指纹"""
        return self.serial_thread.send_command(self.CMD_DELETE_ALL)
    
    def check_finger_status(self):
        """检查指纹状态"""
        return self.serial_thread.send_command(self.CMD_CHECK_FINGER_STATUS)
    
    def start_record(self, finger_id):
        """开始录制指纹"""
        self.is_recording = True
        self.record_step = 0
        self.current_finger_id = finger_id
        # 开始第一次采集
        cmd = f"EF 02 FF FF FF FF FF FF FF FF 00 00 25 03 01 04 00 01 00 00 00 C2 4F"
        return self.serial_thread.send_command(cmd)
    
    def continue_record(self):
        """继续录制指纹（第二次/第三次）"""
        if not self.is_recording:
            return False
        
        self.record_step += 1
        if self.record_step == 1:
            # 第二次采集
            cmd = f"EF 02 FF FF FF FF FF FF FF FF 00 00 25 03 01 04 00 02 00 00 00 1E D4"
        elif self.record_step == 2:
            # 第三次采集
            cmd = f"EF 02 FF FF FF FF FF FF FF FF 00 00 25 03 01 04 00 03 00 00 00 AA A2"
        else:
            # 结束采集并保存
            self.save_fingerprint()
            return True
        
        return self.serial_thread.send_command(cmd)
    
    def save_fingerprint(self):
        """保存指纹"""
        if not self.current_finger_id:
            return False
        
        # 结束录入命令
        cmd_end = "EF 02 FF FF FF FF FF FF FF FF 00 00 36 03 01 00 00 EC C6"
        self.serial_thread.send_command(cmd_end)
        
        # 生成注册命令 - 通过命令队列自动等待前一条命令响应后再发送
        cmd = self.generate_register_command(self.current_finger_id)
        result = self.serial_thread.send_command(cmd)
        
        self.is_recording = False
        self.record_step = 0
        return result
    
    def generate_register_command(self, finger_id):
        """生成指纹注册命令"""
        # 计算CRC校验
        def crc16_ccitt(data):
            crc = 0x0000
            polynomial = 0x1021
            for byte in data:
                crc ^= (byte << 8)
                for _ in range(8):
                    if crc & 0x8000:
                        crc = (crc << 1) ^ polynomial
                    else:
                        crc <<= 1
                    crc &= 0xFFFF
            return crc
        
        # 构建命令前19字节
        pre_check_bytes = [
            0xEF, 0x02,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x00, 0x27, 0x03, 0x01, 0x02,
            0x00,
            finger_id & 0xFF,
            0x00
        ]
        
        # 计算CRC
        crc = crc16_ccitt(pre_check_bytes)
        crc_low = crc & 0xFF
        crc_high = (crc >> 8) & 0xFF
        
        # 组合完整命令 尾部加上0a 0d
        full_command = pre_check_bytes + [crc_low, crc_high, 0x0A, 0x0D] 
        
        # 转换为带空格的十六进制字符串
        return ' '.join(f'{b:02X}' for b in full_command)
    
    def verify_fingerprint(self):
        """验证指纹 - 按照正确流程执行"""
        # 先缓存指纹
        cmd_cache = "EF 02 FF FF FF FF FF FF FF FF 00 00 25 03 01 04 00 01 00 00 00 C2 4F"
        self.serial_thread.send_command(cmd_cache)
        
        # 然后验证 - 通过命令队列自动等待前一条命令响应后再发送
        cmd_verify = "EF 02 FF FF FF FF FF FF FF FF 00 00 28 03 01 04 00 01 00 00 00 F1 11"
        return self.serial_thread.send_command(cmd_verify)
    
    @staticmethod
    def parse_verify_result(response_str):
        """解析验证结果"""
        try:
            # 正确的验证结果格式：... 00 00 00 XX 00 XX XX XX XX
            # 错误的验证结果格式：... 00 07 00 00 00 00 00 XX XX
            parts = response_str.split()
            if len(parts) < 10:
                return None
            
            # 检查是否是验证响应
            if "28" in parts and "03" in parts and "02" in parts and "06" in parts:
                # 查找特征码位置
                for i in range(len(parts) - 5):
                    if parts[i] == "00" and parts[i+1] == "00" and parts[i+2] == "00":
                        # 找到正确的指纹
                        finger_id = int(parts[i+3], 16)
                        return finger_id
                    elif parts[i] == "00" and parts[i+1] == "07" and parts[i+2] == "00":
                        # 指纹不匹配
                        return -1
            return None
        except Exception:
            return None

class CommandHistoryWidget(QWidget):
    """通信记录窗口"""
    def __init__(self, standalone=False):
        super().__init__()
        self.standalone = standalone  # 是否作为独立窗口使用
        self.init_ui()
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        title_label = QLabel("通信记录")
        title_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        
        self.history_text = QTextEdit()
        self.history_text.setReadOnly(True)
        
        # 如果不是独立窗口，则设置最大高度限制
        if not self.standalone:
            self.history_text.setMaximumHeight(150)
        
        self.history_text.setStyleSheet("font-family: Consolas, Monaco, monospace; font-size: 12px;")
        
        clear_btn = QPushButton("清空记录")
        clear_btn.clicked.connect(self.clear_history)
        
        # 添加保存记录按钮
        save_btn = QPushButton("保存记录")
        save_btn.clicked.connect(self.save_history)
        
        button_layout = QHBoxLayout()
        button_layout.addWidget(clear_btn)
        button_layout.addWidget(save_btn)
        button_layout.addStretch()
        
        layout.addWidget(title_label)
        layout.addWidget(self.history_text)
        layout.addLayout(button_layout)
        
    def save_history(self):
        """保存通信记录到文件"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "保存通信记录", "", "文本文件 (*.txt);;所有文件 (*)"
        )
        
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    # 移除HTML标签，只保存纯文本内容
                    plain_text = self.history_text.toPlainText()
                    f.write(plain_text)
                QMessageBox.information(self, "成功", "通信记录已保存！")
            except Exception as e:
                QMessageBox.critical(self, "错误", f"保存失败: {str(e)}")
    
    def add_record(self, record_type, content):
        """添加记录"""
        timestamp = time.strftime("%H:%M:%S")
        if record_type == "发送":
            color = "#0066CC"
        elif record_type == "接收":
            color = "#009900"
        else:
            color = "#666666"
        
        self.history_text.append(f'<span style="color:{color};">[{timestamp}] {record_type}: {content}</span>')
        # 自动滚动到底部
        self.history_text.moveCursor(QTextCursor.MoveOperation.End)
    
    def clear_history(self):
        """清空记录"""
        self.history_text.clear()

class ExternalProgramConfig(QWidget):
    """外部程序配置窗口"""
    def __init__(self):
        super().__init__()
        self.init_ui()
        self.load_config()
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        title_label = QLabel("指纹绑定外部程序配置")
        title_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        
        # 创建配置表格
        grid_layout = QGridLayout()
        grid_layout.setSpacing(10)
        
        self.file_paths = []
        self.args_inputs = []
        
        for i in range(1, 7):  # 支持6个指纹
            finger_label = QLabel(f"指纹 {i}:")
            file_path = QLineEdit()
            file_path.setPlaceholderText("选择可执行文件路径")
            browse_btn = QPushButton("浏览...")
            browse_btn.clicked.connect(lambda checked, idx=i-1: self.browse_file(idx))
            
            args_label = QLabel("参数:")
            args_input = QLineEdit()
            args_input.setPlaceholderText("可选参数")
            
            grid_layout.addWidget(finger_label, i-1, 0)
            grid_layout.addWidget(file_path, i-1, 1)
            grid_layout.addWidget(browse_btn, i-1, 2)
            grid_layout.addWidget(args_label, i-1, 3)
            grid_layout.addWidget(args_input, i-1, 4)
            
            self.file_paths.append(file_path)
            self.args_inputs.append(args_input)
        
        # 保存按钮
        save_btn = QPushButton("保存配置")
        save_btn.clicked.connect(self.save_config)
        save_btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 8px;")
        
        layout.addWidget(title_label)
        layout.addLayout(grid_layout)
        layout.addWidget(save_btn)
    
    def browse_file(self, index):
        """浏览文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "选择可执行文件", "", "可执行文件 (*.exe);;所有文件 (*)")
        if file_path:
            self.file_paths[index].setText(file_path)
    
    def save_config(self):
        """保存配置"""
        config = {}
        for i in range(len(self.file_paths)):
            if self.file_paths[i].text():
                config[f"finger_{i+1}"] = {
                    "path": self.file_paths[i].text(),
                    "args": self.args_inputs[i].text()
                }
        
        # 保存到文件
        import json
        try:
            with open("fingerprint_config.json", "w", encoding="utf-8") as f:
                json.dump(config, f, ensure_ascii=False, indent=2)
            QMessageBox.information(self, "成功", "配置已保存！")
        except Exception as e:
            QMessageBox.critical(self, "错误", f"保存配置失败: {str(e)}")
    
    def load_config(self):
        """加载配置"""
        import json
        try:
            if os.path.exists("fingerprint_config.json"):
                with open("fingerprint_config.json", "r", encoding="utf-8") as f:
                    config = json.load(f)
                    for i in range(len(self.file_paths)):
                        key = f"finger_{i+1}"
                        if key in config:
                            self.file_paths[i].setText(config[key].get("path", ""))
                            self.args_inputs[i].setText(config[key].get("args", ""))
        except Exception:
            pass
    
    def get_program_info(self, finger_id):
        """获取指定指纹的程序信息"""
        import json
        try:
            if os.path.exists("fingerprint_config.json"):
                with open("fingerprint_config.json", "r", encoding="utf-8") as f:
                    config = json.load(f)
                    key = f"finger_{finger_id}"
                    if key in config:
                        return config[key]
        except Exception:
            pass
        return None

class VerificationTab(QWidget):
    """验证指纹标签页"""
    def __init__(self, fingerprint_module, history_widget, program_config):
        super().__init__()
        self.fingerprint_module = fingerprint_module
        self.history_widget = history_widget
        self.program_config = program_config
        self.init_ui()
        self.setup_signals()
        self.module_verified = False  # 添加模块验证状态标志
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        # 状态显示区域
        status_group = QGroupBox("验证状态")
        status_layout = QVBoxLayout()
        
        self.status_label = QLabel("等待连接指纹模组...")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet(
            "font-size: 16px; padding: 20px; border: 2px dashed #CCCCCC; border-radius: 8px;"
        )
        
        self.finger_id_label = QLabel("")
        self.finger_id_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.finger_id_label.setStyleSheet(
            "font-size: 24px; font-weight: bold; color: #4CAF50; margin-top: 10px;"
        )
        
        status_layout.addWidget(self.status_label)
        status_layout.addWidget(self.finger_id_label)
        status_group.setLayout(status_layout)
        
        # 操作按钮
        button_layout = QHBoxLayout()
        
        self.verify_btn = QPushButton("开始验证")
        self.verify_btn.setStyleSheet(
            "background-color: #2196F3; color: white; padding: 10px 20px; font-size: 14px;"
        )
        self.verify_btn.setEnabled(False)
        
        self.stop_btn = QPushButton("停止验证")
        self.stop_btn.setStyleSheet(
            "background-color: #FF9800; color: white; padding: 10px 20px; font-size: 14px;"
        )
        self.stop_btn.setEnabled(False)
        
        button_layout.addWidget(self.verify_btn)
        button_layout.addWidget(self.stop_btn)
        button_layout.addStretch()
        
        layout.addWidget(status_group)
        layout.addLayout(button_layout)
        layout.addWidget(self.history_widget)
        
        # 定时器用于循环检测
        self.timer = QTimer()
        self.is_verifying = False
    
    def setup_signals(self):
        self.verify_btn.clicked.connect(self.start_verification)
        self.stop_btn.clicked.connect(self.stop_verification)
        self.timer.timeout.connect(self.check_fingerprint)
    
    def start_verification(self):
        """开始验证"""
        self.is_verifying = True
        self.verify_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.status_label.setText("请按压指纹...")
        self.finger_id_label.setText("")
        # 启动定时器，每300ms检查一次
        self.timer.start(300)
    
    def stop_verification(self):
        """停止验证"""
        # 立即设置标志位为False，阻止所有验证相关操作
        self.is_verifying = False
        
        # 停止定时器
        if self.timer.isActive():
            self.timer.stop()
        
        # 更新UI状态
        self.verify_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        self.status_label.setText("验证已停止")
        self.finger_id_label.setText("")
        
        # 处理UI事件队列，确保UI立即响应更新
        QApplication.processEvents()
    
    def check_fingerprint(self):
        """检查指纹"""
        self.fingerprint_module.check_finger_status()
    
    def update_connection_status(self, connected, module_verified=False):
        """更新连接状态"""
        if connected and module_verified:
            self.module_verified = True
            self.status_label.setText("已连接，请点击开始验证")
            self.verify_btn.setEnabled(True)
        elif connected and not module_verified:
            self.module_verified = False
            self.status_label.setText("正在验证模块响应...")
            self.verify_btn.setEnabled(False)
        else:
            self.stop_verification()
            self.module_verified = False
            self.status_label.setText("未连接指纹模组")
            self.verify_btn.setEnabled(False)
    
    def handle_response(self, response_str):
        """处理响应 - 使用精确的状态响应匹配"""
        if not self.is_verifying:
            return
            
        # 检查精确的指纹状态响应
        # 有手指且接触良好: EF 02 FF FF FF FF FF FF FF FF 00 00 23 03 02 02 00 00 00 36 CD
        if "23 03 02 02 00 00 00 36 CD" in response_str and self.is_verifying:
            self.status_label.setText("正在验证指纹...")
            # 执行验证
            threading.Timer(1, self.perform_verification).start()
        # 无手指或接触不好: EF 02 FF FF FF FF FF FF FF FF 00 00 23 03 02 02 00 04 00 F2 01
        elif "23 03 02 02 00 04 00 F2 01" in response_str and self.is_verifying:
            self.status_label.setText("请按压指纹...")
            self.finger_id_label.setText("")
    
    def perform_verification(self):
        """执行验证"""
        if self.is_verifying:
            self.fingerprint_module.verify_fingerprint()
    
    def handle_verify_result(self, finger_id):
        """处理验证结果"""
        if finger_id is None:
            return
        
        if finger_id == -1:
            self.status_label.setText("指纹不匹配")
            self.finger_id_label.setText("")
        else:
            self.status_label.setText("验证成功！")
            self.finger_id_label.setText(f"指纹 {finger_id}")
            # 检查是否需要执行外部程序
            program_info = self.program_config.get_program_info(finger_id)
            if program_info and "path" in program_info:
                self.execute_external_program(program_info)
    
    def execute_external_program(self, program_info):
        """执行外部程序"""
        try:
            import subprocess
            path = program_info["path"]
            args = program_info.get("args", "").split()
            
            if os.path.exists(path):
                subprocess.Popen([path] + args, shell=True)
                self.history_widget.add_record("系统", f"已执行外部程序: {os.path.basename(path)}")
        except Exception as e:
            self.history_widget.add_record("错误", f"执行外部程序失败: {str(e)}")

class EnrollmentTab(QWidget):
    """注册指纹标签页"""
    def __init__(self, fingerprint_module, history_widget):
        super().__init__()
        self.fingerprint_module = fingerprint_module
        self.history_widget = history_widget
        self.init_ui()
        self.setup_signals()
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        # 指纹ID选择
        id_group = QGroupBox("选择指纹ID")
        id_layout = QHBoxLayout()
        
        self.finger_id_combo = QComboBox()
        for i in range(1, 101):  # 假设有100个指纹槽位
            self.finger_id_combo.addItem(f"指纹 {i}", i)
        
        id_layout.addWidget(self.finger_id_combo)
        id_group.setLayout(id_layout)
        
        # 注册状态
        status_group = QGroupBox("注册状态")
        status_layout = QVBoxLayout()
        
        self.status_label = QLabel("请选择指纹ID并点击开始注册")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_label.setStyleSheet(
            "font-size: 16px; padding: 20px; border: 2px dashed #CCCCCC; border-radius: 8px;"
        )
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setValue(0)
        self.progress_bar.setRange(0, 3)
        
        status_layout.addWidget(self.status_label)
        status_layout.addWidget(self.progress_bar)
        status_group.setLayout(status_layout)
        
        # 操作按钮
        button_layout = QHBoxLayout()
        
        self.start_btn = QPushButton("开始注册")
        self.start_btn.setStyleSheet(
            "background-color: #4CAF50; color: white; padding: 10px 20px; font-size: 14px;"
        )
        self.start_btn.setEnabled(False)
        
        self.cancel_btn = QPushButton("取消注册")
        self.cancel_btn.setStyleSheet(
            "background-color: #FF5722; color: white; padding: 10px 20px; font-size: 14px;"
        )
        self.cancel_btn.setEnabled(False)
        
        button_layout.addWidget(self.start_btn)
        button_layout.addWidget(self.cancel_btn)
        button_layout.addStretch()
        
        layout.addWidget(id_group)
        layout.addWidget(status_group)
        layout.addLayout(button_layout)
        layout.addWidget(self.history_widget)
        
        # 状态变量
        self.is_enrolling = False
        self.enroll_step = 0
        self.current_finger_id = None
        # 定时器用于查询指纹状态
        self.timer = QTimer()
    
    def setup_signals(self):
        self.start_btn.clicked.connect(self.start_enrollment)
        self.cancel_btn.clicked.connect(self.cancel_enrollment)
        self.timer.timeout.connect(self.check_finger_status)
    
    def start_enrollment(self):
        """开始注册"""
        self.current_finger_id = self.finger_id_combo.currentData()
        self.is_enrolling = True
        self.enroll_step = 0
        
        self.start_btn.setEnabled(False)
        self.cancel_btn.setEnabled(True)
        self.status_label.setText(f"请按压指纹（第1次）")
        self.progress_bar.setValue(0)
        
        # 启动定时器，每300ms检查一次指纹状态
        self.timer.start(300)
    
    def cancel_enrollment(self):
        """取消注册"""
        self.is_enrolling = False
        self.start_btn.setEnabled(True)
        self.cancel_btn.setEnabled(False)
        self.status_label.setText("注册已取消")
        self.progress_bar.setValue(0)
        self.current_finger_id = None
        # 停止定时器
        self.timer.stop()
    
    def update_connection_status(self, connected, module_verified=False):
        """更新连接状态"""
        if connected and module_verified:
            self.start_btn.setEnabled(True)
        elif connected and not module_verified:
            # 模块连接但未验证时也保持按钮禁用
            self.start_btn.setEnabled(False)
        else:
            self.cancel_enrollment()
            self.start_btn.setEnabled(False)
    
    def check_finger_status(self):
        """定时检查指纹状态"""
        if self.is_enrolling:
            self.fingerprint_module.check_finger_status()
    
    def handle_response(self, response_str):
        """处理响应 - 使用精确的状态响应匹配"""
        if not self.is_enrolling:
            return
        
        # 检查指纹状态和注册响应
        # 使用精确的状态响应匹配
        if "23 03 02 02 00 00 00 36 CD" in response_str:
            if self.enroll_step == 0:
                # 第一次按压，开始第一次采集
                self.status_label.setText(f"请抬起手指")
                self.fingerprint_module.start_record(self.current_finger_id)
                self.enroll_step = 1
            elif self.enroll_step == 1:
                # 第二次按压，开始第二次采集
                self.status_label.setText(f"请抬起手指")
                self.progress_bar.setValue(1)
                self.fingerprint_module.continue_record()
                self.enroll_step = 2
            elif self.enroll_step == 2:
                # 第三次按压，开始第三次采集
                self.status_label.setText(f"请抬起手指")
                self.progress_bar.setValue(2)
                self.fingerprint_module.continue_record()
                self.enroll_step = 3
        elif "23 03 02 02 00 04 00 F2 01" in response_str:
            if self.enroll_step == 1:
                self.status_label.setText(f"请再次按压指纹（第2次）")
            elif self.enroll_step == 3:
                # 完成注册过程，保存指纹
                self.fingerprint_module.save_fingerprint()
                self.status_label.setText(f"注册成功！")
                self.progress_bar.setValue(3)
                self.is_enrolling = False
                self.timer.stop()
                self.start_btn.setEnabled(True)
                self.cancel_btn.setEnabled(False)
                
                QMessageBox.information(self, "成功", f"指纹 {self.current_finger_id} 注册成功！")

class ManagementTab(QWidget):
    """指纹管理标签页"""
    def __init__(self, fingerprint_module, history_widget):
        super().__init__()
        self.fingerprint_module = fingerprint_module
        self.history_widget = history_widget
        self.init_ui()
        self.setup_signals()
    
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        # 操作按钮组
        button_group = QGroupBox("模块操作")
        button_layout = QGridLayout()
        
        self.wakeup_btn = QPushButton("唤醒模块")
        self.wakeup_btn.setStyleSheet(
            "background-color: #2196F3; color: white; padding: 10px; font-size: 14px;"
        )
        self.wakeup_btn.setEnabled(False)
        
        self.sleep_btn = QPushButton("休眠模块")
        self.sleep_btn.setStyleSheet(
            "background-color: #9C27B0; color: white; padding: 10px; font-size: 14px;"
        )
        self.sleep_btn.setEnabled(False)
        
        self.delete_all_btn = QPushButton("删除所有指纹")
        self.delete_all_btn.setStyleSheet(
            "background-color: #F44336; color: white; padding: 10px; font-size: 14px;"
        )
        self.delete_all_btn.setEnabled(False)
        
        button_layout.addWidget(self.wakeup_btn, 0, 0)
        button_layout.addWidget(self.sleep_btn, 0, 1)
        button_layout.addWidget(self.delete_all_btn, 1, 0, 1, 2)
        button_group.setLayout(button_layout)
        
        # 状态显示
        status_group = QGroupBox("模块状态")
        status_layout = QVBoxLayout()
        
        self.module_status_label = QLabel("未连接")
        self.module_status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.module_status_label.setStyleSheet(
            "font-size: 16px; padding: 20px; border: 2px solid #FF5722; border-radius: 8px; color: #FF5722;"
        )
        
        status_layout.addWidget(self.module_status_label)
        status_group.setLayout(status_layout)
        
        layout.addWidget(button_group)
        layout.addWidget(status_group)
        layout.addWidget(self.history_widget)
    
    def setup_signals(self):
        self.wakeup_btn.clicked.connect(self.wakeup_module)
        self.sleep_btn.clicked.connect(self.sleep_module)
        self.delete_all_btn.clicked.connect(self.delete_all_fingerprints)
    
    def wakeup_module(self):
        """唤醒模块"""
        result = self.fingerprint_module.wakeup()
        if result:
            self.module_status_label.setText("已唤醒")
            self.module_status_label.setStyleSheet(
                "font-size: 16px; padding: 20px; border: 2px solid #4CAF50; border-radius: 8px; color: #4CAF50;"
            )
    
    def sleep_module(self):
        """休眠模块"""
        result = self.fingerprint_module.sleep()
        if result:
            self.module_status_label.setText("已休眠")
            self.module_status_label.setStyleSheet(
                "font-size: 16px; padding: 20px; border: 2px solid #9E9E9E; border-radius: 8px; color: #9E9E9E;"
            )
    
    def delete_all_fingerprints(self):
        """删除所有指纹"""
        reply = QMessageBox.question(
            self, "确认删除", "确定要删除所有指纹吗？此操作不可恢复！",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No, QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            result = self.fingerprint_module.delete_all_fingerprints()
            if result:
                QMessageBox.information(self, "成功", "所有指纹已删除！")
    
    def update_connection_status(self, connected, module_verified=False):
        """更新连接状态"""
        if connected and module_verified:
            self.wakeup_btn.setEnabled(True)
            self.sleep_btn.setEnabled(True)
            self.delete_all_btn.setEnabled(True)
            self.module_status_label.setText("已连接")
            self.module_status_label.setStyleSheet(
                "font-size: 16px; padding: 20px; border: 2px solid #4CAF50; border-radius: 8px; color: #4CAF50;"
            )
        elif connected and not module_verified:
            # 模块连接但未验证时禁用按钮
            self.wakeup_btn.setEnabled(False)
            self.sleep_btn.setEnabled(False)
            self.delete_all_btn.setEnabled(False)
            self.module_status_label.setText("正在验证模块")
            self.module_status_label.setStyleSheet(
                "font-size: 16px; padding: 20px; border: 2px solid #FFC107; border-radius: 8px; color: #FFC107;"
            )
        else:
            self.wakeup_btn.setEnabled(False)
            self.sleep_btn.setEnabled(False)
            self.delete_all_btn.setEnabled(False)
            self.module_status_label.setText("未连接")
            self.module_status_label.setStyleSheet(
                "font-size: 16px; padding: 20px; border: 2px solid #FF5722; border-radius: 8px; color: #FF5722;"
            )

class MainWindow(QMainWindow):
    """主窗口"""
    def __init__(self):
        super().__init__()
        self.is_connected = False
        self.module_response_received = False
        self.fingerprint_module = None
        self.response_timer = QTimer(self)
        self.response_timer.timeout.connect(self.check_module_response)
        # 添加最后提示时间记录，避免频繁弹窗
        self.last_prompt_time = 0
        self.prompt_interval = 5000  # 5秒内不重复提示
        self.init_ui()
        self.setup_serial()
    
    def init_ui(self):
        # 设置窗口标题和大小
        self.setWindowTitle("专用指纹模组测试上位机 作者微信liuwen2357 维修测试用 请勿传播")
        self.setMinimumSize(800, 600)
        
        # 创建中心部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 主布局
        main_layout = QVBoxLayout(central_widget)
        
        # 串口配置区域
        serial_group = QGroupBox("串口配置")
        serial_layout = QHBoxLayout()
        
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        
        self.refresh_btn = QPushButton("刷新端口")
        self.refresh_btn.clicked.connect(self.refresh_ports)
        
        self.connect_btn = QPushButton("连接")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 5px 20px;")
        self.connect_btn.clicked.connect(self.toggle_connection)
        
        serial_layout.addWidget(QLabel("串口:"))
        serial_layout.addWidget(self.port_combo)
        serial_layout.addWidget(self.refresh_btn)
        serial_layout.addWidget(self.connect_btn)
        serial_layout.addStretch()
        serial_group.setLayout(serial_layout)
        
        # 创建标签页
        self.tab_widget = QTabWidget()
        
        # 创建公共组件
        self.history_widget = CommandHistoryWidget(standalone=False)  # 用于嵌入各个标签页
        self.standalone_history = CommandHistoryWidget(standalone=True)  # 独立标签页的通信记录
        self.program_config = ExternalProgramConfig()
        
        # 创建指纹模块实例（先使用None，连接时再初始化）
        self.serial_thread = None
        self.fingerprint_module = None
        
        # 创建各个标签页
        self.verification_tab = VerificationTab(None, self.history_widget, self.program_config)
        self.enrollment_tab = EnrollmentTab(None, self.history_widget)
        self.management_tab = ManagementTab(None, self.history_widget)
        
        # 添加标签页
        self.tab_widget.addTab(self.verification_tab, "验证指纹")
        self.tab_widget.addTab(self.enrollment_tab, "注册指纹")
        self.tab_widget.addTab(self.management_tab, "模块管理")
        self.tab_widget.addTab(self.program_config, "外部程序配置")
        
        # 添加独立通信记录窗口按钮
        self.history_window = None  # 保存独立窗口引用
        self.show_history_btn = QPushButton("显示通信记录")
        self.show_history_btn.clicked.connect(self.toggle_history_window)
        serial_layout.addWidget(self.show_history_btn)
        
        # 将串口配置和标签页添加到主布局
        main_layout.addWidget(serial_group)
        main_layout.addWidget(self.tab_widget)
        
        # 刷新端口列表
        self.refresh_ports()
    
    def setup_serial(self):
        """设置串口相关功能"""
        self.is_connected = False
        # 用于检测模块响应的标志和计时器
        self.module_response_received = False
        self.response_timer = QTimer(self)
        self.response_timer.timeout.connect(self.check_module_response)
    
    def refresh_ports(self):
        """刷新端口列表"""
        self.port_combo.clear()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.port_combo.addItem(f"{port.device} - {port.description}", port.device)
        
        # 如果没有端口，显示提示
        if self.port_combo.count() == 0:
            self.port_combo.addItem("无可用串口")
    
    def toggle_connection(self):
        """切换连接状态"""
        if self.is_connected:
            self.disconnect_serial()
        else:
            self.connect_serial()
    
    def connect_serial(self):
        """连接串口"""
        if self.port_combo.count() == 0 or self.port_combo.currentIndex() < 0:
            QMessageBox.warning(self, "警告", "请选择有效的串口！")
            return
        
        port = self.port_combo.currentData()
        if not port:
            QMessageBox.warning(self, "警告", "请选择有效的串口！")
            return
        
        try:
            # 创建串口线程
            self.serial_thread = SerialThread(port=port, baudrate=9600)
            self.serial_thread.received_data.connect(self.handle_received_data)
            self.serial_thread.sent_data.connect(self.handle_sent_data)
            self.serial_thread.error_occurred.connect(self.handle_serial_error)
            self.serial_thread.connection_status.connect(self.handle_connection_status)
            
            # 创建指纹模块实例
            self.fingerprint_module = FingerprintModule(self.serial_thread)
            
            # 更新标签页中的指纹模块引用
            self.verification_tab.fingerprint_module = self.fingerprint_module
            self.enrollment_tab.fingerprint_module = self.fingerprint_module
            self.management_tab.fingerprint_module = self.fingerprint_module
            
            # 启动串口线程
            self.serial_thread.start()
            
            # 更新UI
            self.connect_btn.setText("断开")
            self.connect_btn.setStyleSheet("background-color: #F44336; color: white; padding: 5px 20px;")
            self.port_combo.setEnabled(False)
            self.refresh_btn.setEnabled(False)
            
            self.history_widget.add_record("系统", f"正在连接串口: {port}")
        except Exception as e:
            QMessageBox.critical(self, "错误", f"连接串口失败: {str(e)}")
    
    def disconnect_serial(self):
        """断开串口连接"""
        if self.serial_thread:
            self.serial_thread.stop()
            self.serial_thread = None
        
        self.fingerprint_module = None
        
        # 更新UI
        self.connect_btn.setText("连接")
        self.connect_btn.setStyleSheet("background-color: #4CAF50; color: white; padding: 5px 20px;")
        self.port_combo.setEnabled(True)
        self.refresh_btn.setEnabled(True)
        self.is_connected = False
        
        # 更新所有标签页的连接状态
        self.verification_tab.update_connection_status(False)
        self.enrollment_tab.update_connection_status(False)
        self.management_tab.update_connection_status(False)
        
        self.history_widget.add_record("系统", "已断开串口连接")
    
    def handle_sent_data(self, data):
        """处理发送的数据"""
        try:
            # 确保在主线程中更新UI
            if self.history_widget and hasattr(self.history_widget, 'add_record'):
                self.history_widget.add_record("发送", data)
                # 同时更新独立的通信记录窗口
                if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
                    self.standalone_history.add_record("发送", data)
        except Exception as e:
            print(f"处理发送数据错误: {str(e)}")
    
    def handle_received_data(self, data):
        """处理接收到的数据"""
        try:
            # 确保在主线程中更新UI
            if self.history_widget and hasattr(self.history_widget, 'add_record'):
                self.history_widget.add_record("接收", data)
                # 同时更新独立的通信记录窗口
                if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
                    self.standalone_history.add_record("接收", data)
            
            # 标记已收到模块响应
            if self.is_connected and not self.module_response_received:
                self.module_response_received = True
                self.response_timer.stop()
                self.history_widget.add_record("系统", "模块响应验证成功")
                # 同时更新独立的通信记录窗口
                if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
                    self.standalone_history.add_record("系统", "模块响应验证成功")
                
                # 更新所有标签页的验证状态为已验证
                self.verification_tab.update_connection_status(True, True)
                self.enrollment_tab.update_connection_status(True, True)
                self.management_tab.update_connection_status(True, True)
            
            # 解析验证结果 - 添加额外检查
            try:
                finger_id = self.fingerprint_module.parse_verify_result(data)
                if finger_id is not None and hasattr(self.verification_tab, 'handle_verify_result'):
                    self.verification_tab.handle_verify_result(finger_id)
            except Exception as parse_e:
                print(f"解析验证结果错误: {str(parse_e)}")
            
            # 处理各标签页的响应 - 添加额外检查
            try:
                if hasattr(self.verification_tab, 'handle_response'):
                    self.verification_tab.handle_response(data)
                if hasattr(self.enrollment_tab, 'handle_response'):
                    self.enrollment_tab.handle_response(data)
            except Exception as resp_e:
                print(f"处理响应错误: {str(resp_e)}")
        except Exception as e:
            print(f"处理接收数据总体错误: {str(e)}")
    
    def handle_serial_error(self, error_msg):
        """处理串口错误"""
        self.history_widget.add_record("错误", error_msg)
        # 同时更新独立的通信记录窗口
        if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
            self.standalone_history.add_record("错误", error_msg)
        QMessageBox.warning(self, "警告", error_msg)
    
    def handle_connection_status(self, connected):
        """处理连接状态变化"""
        self.is_connected = connected
        if connected:
            self.history_widget.add_record("系统", "串口连接成功")
            # 同时更新独立的通信记录窗口
            if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
                self.standalone_history.add_record("系统", "串口连接成功")
            
            # 重置响应检测标志
            self.module_response_received = False
            
            # 延迟发送查询指令，确保串口完全就绪
            QTimer.singleShot(500, self.send_initial_query)
            
            # 设置响应超时检测（3秒后如果没有收到响应则提示）
            self.response_timer.start(3000)
            
            # 更新标签页状态为正在验证（未完全连接）
            self.verification_tab.update_connection_status(True, False)
            self.enrollment_tab.update_connection_status(True, False)
            self.management_tab.update_connection_status(True, False)
        else:
            self.disconnect_serial()
            # 停止响应检测计时器
            self.response_timer.stop()
            
            # 更新标签页状态为断开连接
            self.verification_tab.update_connection_status(False)
            self.enrollment_tab.update_connection_status(False)
            self.management_tab.update_connection_status(False)
    
    def send_initial_query(self):
        """发送初始查询指令"""
        if self.is_connected and self.fingerprint_module:
            self.history_widget.add_record("系统", "发送初始查询指令...")
            # 发送指纹状态查询指令
            self.fingerprint_module.check_finger_status()
    
    def check_module_response(self):
        """检查模块是否响应"""
        if self.is_connected and not self.module_response_received:
            # 添加时间控制，避免频繁弹窗
            current_time = time.time() * 1000  # 转换为毫秒
            if current_time - self.last_prompt_time > self.prompt_interval:
                QMessageBox.information(self, "提示", "模块可能未连接或已休眠，请用手指按压指纹传感器尝试唤醒！")
                self.last_prompt_time = current_time
            
            self.history_widget.add_record("系统", "未收到模块响应，可能需要唤醒模块")
            # 同时更新独立的通信记录窗口
            if hasattr(self, 'standalone_history') and hasattr(self.standalone_history, 'add_record'):
                self.standalone_history.add_record("系统", "未收到模块响应，可能需要唤醒模块")
    
    def toggle_history_window(self):
        """切换独立通信记录窗口的显示状态"""
        if not self.history_window:
            # 创建独立的通信记录窗口
            self.history_window = QMainWindow()
            self.history_window.setWindowTitle("独立通信记录")
            self.history_window.setMinimumSize(600, 400)
            
            # 每次创建新窗口时都创建一个新的CommandHistoryWidget实例
            self.standalone_history = CommandHistoryWidget(standalone=True)
            
            # 设置独立窗口的中心部件为通信记录部件
            self.history_window.setCentralWidget(self.standalone_history)
            
            # 显示窗口
            self.history_window.show()
            self.show_history_btn.setText("关闭通信记录")
        else:
            # 关闭窗口
            self.history_window.close()
            self.history_window = None
            self.show_history_btn.setText("显示通信记录")
    
    def closeEvent(self, a0: QCloseEvent | None):
        """关闭窗口时断开串口连接"""
        self.response_timer.stop()
        self.disconnect_serial()
        # 关闭独立通信记录窗口
        if self.history_window:
            self.history_window.close()
        if a0:
            a0.accept()

if __name__ == "__main__":
    # 确保中文正常显示
    import matplotlib
    matplotlib.use('Agg')  # 使用非交互式后端，避免可能的冲突
    
    app = QApplication(sys.argv)
    
    # 设置应用程序样式
    app.setStyle("Fusion")
    
    # 设置全局字体
    font = QFont("SimHei", 9)
    app.setFont(font)
    
    # 设置主题色
    palette = QPalette()
    palette.setColor(QPalette.ColorRole.Window, QColor(240, 240, 240))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(0, 0, 0))
    palette.setColor(QPalette.ColorRole.Base, QColor(255, 255, 255))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor(240, 240, 240))
    palette.setColor(QPalette.ColorRole.ToolTipBase, QColor(255, 255, 220))
    palette.setColor(QPalette.ColorRole.ToolTipText, QColor(0, 0, 0))
    palette.setColor(QPalette.ColorRole.Text, QColor(0, 0, 0))
    palette.setColor(QPalette.ColorRole.Button, QColor(240, 240, 240))
    palette.setColor(QPalette.ColorRole.ButtonText, QColor(0, 0, 0))
    palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 0, 0))
    palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(42, 130, 218))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor(255, 255, 255))
    app.setPalette(palette)
    
    window = MainWindow()
    window.show()
    sys.exit(app.exec())