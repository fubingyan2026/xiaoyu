#!/bin/bash
# ============================================
# STM32 J-Link 下载脚本（仅下载，无调试）
# 使用方法: ./flash_jlink.sh 或 bash flash_jlink.sh
# ============================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 打印彩色消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示标题
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}     STM32 J-Link 下载工具（仅下载）    ${NC}"
echo -e "${BLUE}========================================${NC}"

# 1. 设置变量
PROJECT_NAME="Q16_Drive_Opam_clion"
BUILD_DIR="build/RelWithDebInfo"
DOWNLOAD_ADDR="0x08000000"
HEX_FILE="${BUILD_DIR}/${PROJECT_NAME}.hex"
BIN_FILE="${BUILD_DIR}/${PROJECT_NAME}.bin"

# 2. 检查构建目录是否存在
if [ ! -d "$BUILD_DIR" ]; then
    print_error "构建目录不存在: $BUILD_DIR"
    print_info "请先编译项目"
    exit 1
fi

# 3. 检查文件（优先使用 hex，没有则用 bin）
if [ -f "$HEX_FILE" ]; then
    FILE_TO_FLASH="$HEX_FILE"
    print_info "找到 HEX 文件: $HEX_FILE"
elif [ -f "$BIN_FILE" ]; then
    FILE_TO_FLASH="$BIN_FILE"
    print_info "找到 BIN 文件: $BIN_FILE"
else
    print_error "未找到可刷写的文件！"
    print_info "请先编译项目生成 .hex 或 .bin 文件"
    
    # 显示目录内容
    print_info "构建目录内容:"
    ls -la "$BUILD_DIR/" 2>/dev/null || echo "目录为空"
    exit 1
fi

# 4. 检查 J-Link 工具
if ! command -v JLinkExe &> /dev/null; then
    print_error "JLinkExe 未找到！"
    print_info "请安装 SEGGER J-Link Software 并添加到 PATH"
    print_info "下载地址: https://www.segger.com/downloads/jlink/"
    
    # Linux 默认路径
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        print_info "Linux 默认安装路径: /opt/SEGGER/JLink/"
    fi
    exit 1
fi

# 5. 检查 J-Link 连接
print_info "检查 J-Link 连接..."
JLinkExe -device STM32G431C8 -if SWD -speed 4000 -autoconnect 1 -ExitOnError 1 <<EOF
exit
EOF

if [ $? -ne 0 ]; then
    print_error "J-Link 连接失败！"
    print_info "请检查："
    print_info "1. J-Link 是否插入 USB"
    print_info "2. 接线是否正确（SWDIO, SWCLK, GND）"
    print_info "3. 目标板是否供电"
    print_info "4. 驱动是否安装"
    exit 1
else
    print_success "J-Link 连接正常"
fi

# 6. 开始下载
print_info "开始下载程序..."
echo "========================================"

# 创建临时命令文件
TEMP_CMD_FILE=$(mktemp)
cat > "$TEMP_CMD_FILE" <<EOF
connect
r
h
loadbin $FILE_TO_FLASH $DOWNLOAD_ADDR
r
g
exit
EOF

# 使用 J-Link 下载
JLinkExe -device STM32G431C8 -if SWD -speed 4000 -autoconnect 1 -CommandFile "$TEMP_CMD_FILE"

DOWNLOAD_RESULT=$?

# 删除临时文件
rm -f "$TEMP_CMD_FILE"

# 7. 检查结果
if [ $DOWNLOAD_RESULT -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}          下载成功！          ${NC}"
    echo -e "${GREEN}========================================${NC}"
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}          下载失败！          ${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
