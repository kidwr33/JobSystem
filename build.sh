#!/bin/bash

# JobSystem 动态库构建脚本（macOS/Linux）

echo "====== JobSystem 动态库构建脚本 ======"

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 解析构建模式参数
BUILD_TYPE="Release"
if [ $# -gt 0 ]; then
    case "$1" in
        debug|Debug|DEBUG)
            BUILD_TYPE="Debug"
            ;;
        release|Release|RELEASE)
            BUILD_TYPE="Release"
            ;;
        *)
            echo -e "${RED}错误: 未知的构建模式 '$1'${NC}"
            echo "用法: $0 [debug|release]"
            echo "  debug   - 构建 Debug 模式（带调试信息）"
            echo "  release - 构建 Release 模式（优化，默认）"
            exit 1
            ;;
    esac
fi

# 检测平台
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    LIB_EXT="dylib"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    LIB_EXT="so"
else
    echo -e "${RED}不支持的平台: $OSTYPE${NC}"
    exit 1
fi

echo "平台: $PLATFORM"
echo "构建模式: $BUILD_TYPE"
echo ""

# 创建 build 目录
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}清理旧的 build 目录...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 运行 CMake
echo -e "${GREEN}运行 CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake 配置失败${NC}"
    exit 1
fi

# 编译
echo -e "${GREEN}编译中...${NC}"
cmake --build . --config $BUILD_TYPE -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
if [ $? -ne 0 ]; then
    echo -e "${RED}编译失败${NC}"
    exit 1
fi

cd ..

# 检查输出
LIB_PATH="build/lib/libJobSystem.$LIB_EXT"
if [ -f "$LIB_PATH" ]; then
    echo ""
    echo -e "${GREEN}====== 构建成功！ ======${NC}"
    echo "构建模式: $BUILD_TYPE"
    echo "动态库位置: $LIB_PATH"

    # macOS代码签名
    if [[ "$PLATFORM" == "macOS" ]]; then
        echo ""
        echo -e "${GREEN}对动态库进行代码签名...${NC}"
        codesign -s - -f "$LIB_PATH"
        if [ $? -eq 0 ]; then
            echo "✅ 代码签名成功"
        else
            echo -e "${RED}⚠️  代码签名失败${NC}"
        fi
    fi

    # 显示库信息
    if [[ "$PLATFORM" == "macOS" ]]; then
        echo ""
        echo "库信息:"
        otool -L "$LIB_PATH"
    else
        echo ""
        echo "库信息:"
        ldd "$LIB_PATH"
    fi

    echo ""
    echo -e "${YELLOW}提示：${NC}"
    echo "1. 将 $LIB_PATH 复制到 Unity 项目的 Assets/Plugins/ 目录"
    echo "2. macOS 需要放在 Assets/Plugins/macOS/"
    echo "3. Linux 需要放在 Assets/Plugins/x86_64/"

    if [[ "$BUILD_TYPE" == "Debug" ]]; then
        echo ""
        echo -e "${YELLOW}Debug 模式特性：${NC}"
        echo "- 包含调试符号（可用 lldb/gdb 调试）"
        echo "- 未优化，性能较慢"
        echo "- 文件体积较大"
    fi

else
    echo -e "${RED}未找到生成的动态库${NC}"
    exit 1
fi
