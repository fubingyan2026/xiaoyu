
# ============================================
# STM32 自动下载脚本
# 使用方法: ./flash.sh 或 bash flash.sh
# ============================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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
echo -e "${BLUE}     STM32 ST-LINK 下载工具    ${NC}"
echo -e "${BLUE}========================================${NC}"

# 1. 设置变量
PROJECT_NAME="IMU_CTRL"  # 改为你的项目名
BUILD_DIR="build/RelWithDebInfo"
DOWNLOAD_ADDR="0x08005000"  # 根据需要修改下载地址
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

# 4. 检查 STM32_Programmer_CLI
if ! command -v STM32_Programmer_CLI &> /dev/null; then
    print_error "STM32_Programmer_CLI 未找到！"
    print_info "请安装 STM32CubeProgrammer 并添加到 PATH"
    print_info "下载地址: https://www.st.com/en/development-tools/stm32cubeprog.html"
    
    # Windows 用户提示
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        print_info "Windows 默认安装路径:"
        print_info "C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\\"
    fi
    exit 1
fi

# 5. 检查 ST-LINK 连接
print_info "检查 ST-LINK 连接..."
if STM32_Programmer_CLI -c port=SWD 2>&1 | grep -q "Error"; then
    print_error "ST-LINK 连接失败！"
    print_info "请检查："
    print_info "1. ST-LINK 是否插入 USB"
    print_info "2. 接线是否正确（SWDIO, SWCLK, GND）"
    print_info "3. 目标板是否供电"
    print_info "4. 驱动是否安装"
    exit 1
else
    print_success "ST-LINK 连接正常"
fi

# 6. 询问用户是否继续
# read -p "是否开始下载程序？(y/N): " -n 1 -r
# echo
# if [[ ! $REPLY =~ ^[Yy]$ ]]; then
#     print_info "已取消"
#     exit 0
# fi

# 7. 开始下载
print_info "开始下载程序..."
echo "========================================"

if [[ "$FILE_TO_FLASH" == *.hex ]]; then
    # 下载 HEX 文件
    STM32_Programmer_CLI -c port=SWD freq=4000 -w "$FILE_TO_FLASH" -v -s
else
    # 下载 BIN 文件，需要指定地址
    STM32_Programmer_CLI -c port=SWD freq=4000 -w "$FILE_TO_FLASH" "$DOWNLOAD_ADDR" -v -s
fi

# 8. 检查结果
if [ $? -eq 0 ]; then
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