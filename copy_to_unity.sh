#!/bin/bash

# 自动复制动态库到 Unity 项目

# Unity 项目路径（根据你的路径修改）
UNITY_PROJECT="Unity根目录"
PLUGINS_DIR="$UNITY_PROJECT/Assets/Plugins"

# 检测平台
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    LIB_FILE="build/lib/libJobSystem.dylib"
    TARGET_DIR="$PLUGINS_DIR/macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    LIB_FILE="build/lib/libJobSystem.so"
    TARGET_DIR="$PLUGINS_DIR/x86_64"
else
    echo "不支持的平台: $OSTYPE"
    exit 1
fi

echo "====== 复制 JobSystem 到 Unity ======"
echo "平台: $PLATFORM"
echo "源文件: $LIB_FILE"
echo "目标目录: $TARGET_DIR"
echo ""

# 检查库文件是否存在
if [ ! -f "$LIB_FILE" ]; then
    echo "错误: 找不到动态库文件 $LIB_FILE"
    echo "请先运行 ./build.sh 编译项目"
    exit 1
fi

# 检查 Unity 项目目录
if [ ! -d "$UNITY_PROJECT" ]; then
    echo "错误: Unity 项目目录不存在: $UNITY_PROJECT"
    echo "请在脚本中修改 UNITY_PROJECT 变量为正确的路径"
    exit 1
fi

# 创建目标目录
mkdir -p "$TARGET_DIR"

# 复制库文件
echo "正在复制..."
cp "$LIB_FILE" "$TARGET_DIR/"

if [ $? -eq 0 ]; then
    echo "✅ 复制成功!"

    # macOS代码签名
    if [[ "$PLATFORM" == "macOS" ]]; then
        echo ""
        echo "对动态库进行代码签名..."
        codesign -s - -f "$TARGET_DIR/$(basename $LIB_FILE)"
        if [ $? -eq 0 ]; then
            echo "✅ 代码签名成功"
        else
            echo "⚠️  代码签名失败，Unity可能无法加载此库"
        fi
    fi

    echo ""
    echo "动态库已复制到: $TARGET_DIR/$(basename $LIB_FILE)"

else
    echo "❌ 复制失败"
    exit 1
fi

echo ""
echo "完成！"
