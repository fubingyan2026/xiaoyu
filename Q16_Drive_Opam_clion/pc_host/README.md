# FOC Motor Driver Host PC

基于PyQt6的FOC电机驱动上位机，用于STM32G431 FOC控制系统。

## 功能特性

- **串口通信**: A5协议，支持115200波特率
- **电机控制**: 电流/速度/位置三模式控制
- **编码器校准**: 可视化校准流程，进度显示
- **波形显示**: 实时显示电流、速度、位置波形
- **简约界面**: 暗色主题，专业风格

## 安装依赖

```bash
pip install -r requirements.txt
```

## 运行

```bash
python main.py
```

## 通信协议

### A5帧格式

```
帧头:  0xA5 0x5A (2 bytes)
命令:  1 byte
长度:  1 byte (payload长度)
数据:  0-255 bytes
校验:  XOR (1 byte)
帧尾:  0x0D 0x0A (2 bytes)
```

### 命令列表

| 命令 | 代码 | 说明 |
|------|------|------|
| CMD_PING | 0x01 | 心跳/连接测试 |
| CMD_GET_STATUS | 0x10 | 获取当前状态 |
| CMD_SET_CURRENT | 0x20 | 设置电流 (Id, Iq) |
| CMD_SET_VELOCITY | 0x21 | 设置速度 |
| CMD_SET_POSITION | 0x22 | 设置位置 |
| CMD_STOP | 0x23 | 停止电机 |
| CMD_START_CALIB | 0x30 | 开始编码器校准 |
| CMD_START_STREAM | 0x50 | 开始数据流 |

## 项目结构

```
pc_host/
├── core/               # 核心模块
│   ├── protocol.py     # A5协议实现
│   ├── serial_manager.py  # 串口管理
│   ├── controller.py   # FOC控制器
│   └── models.py       # 数据模型
├── ui/                 # UI模块
│   ├── main_window.py  # 主窗口
│   ├── connection_panel.py  # 连接面板
│   ├── control_panel.py     # 控制面板
│   ├── status_panel.py      # 状态面板
│   ├── waveform_widget.py   # 波形显示
│   ├── calibration_dialog.py # 校准对话框
│   └── styles.py       # 样式定义
├── main.py            # 入口文件
├── requirements.txt   # 依赖列表
└── README.md          # 说明文档
```

## 界面预览

- 左侧: 连接设置、电机控制、实时状态
- 右侧: 波形显示（电流、速度、位置）

## 注意事项

1. 确保下位机固件已烧录并运行
2. 选择正确的串口号（通常是USB转串口）
3. 默认波特率115200
4. 波形显示需要点击"开始采集"按钮
