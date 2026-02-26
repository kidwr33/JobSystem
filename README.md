# JobSystem 构建指南

## 前置要求

### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 CMake (使用 Homebrew)
brew install cmake

# 或者从官网下载: https://cmake.org/download/
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake

# CentOS/RHEL
sudo yum install gcc-c++ make cmake
```

### Windows
1. 安装 Visual Studio 2017 或更新版本
2. 下载并安装 CMake: https://cmake.org/download/
3. 确保将 CMake 添加到系统 PATH

## 构建步骤

### 方法 1: 使用构建脚本（推荐）

#### macOS / Linux
```bash
cd /Users/wepie/JobSystem
chmod +x build.sh
./build.sh
```

#### Windows
```cmd
cd C:\path\to\JobSystem
build.bat
```

### 方法 2: 手动构建

#### macOS / Linux
```bash
# 1. 进入项目目录
cd /Users/wepie/JobSystem

# 2. 创建 build 目录
mkdir -p build
cd build

# 3. 运行 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. 编译
cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

# 5. 查看生成的库
ls -la lib/
```

#### Windows
```cmd
REM 1. 进入项目目录
cd C:\path\to\JobSystem

REM 2. 创建 build 目录
mkdir build
cd build

REM 3. 运行 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

REM 4. 编译
cmake --build . --config Release

REM 5. 查看生成的库
dir lib\Release\
```

## 输出文件

构建成功后，动态库位于：

- **macOS**: `build/lib/libJobSystem.dylib`
- **Linux**: `build/lib/libJobSystem.so`
- **Windows**: `build\lib\Release\JobSystem.dll`

## 验证构建

### macOS
```bash
# 查看动态库依赖
otool -L build/lib/libJobSystem.dylib

# 查看导出符号
nm -gU build/lib/libJobSystem.dylib | grep JobSystem
```

### Linux
```bash
# 查看动态库依赖
ldd build/lib/libJobSystem.so

# 查看导出符号
nm -D build/lib/libJobSystem.so | grep JobSystem
```

### Windows
```cmd
REM 使用 dumpbin（需要 Visual Studio）
dumpbin /EXPORTS build\lib\Release\JobSystem.dll
```

## 复制到 Unity 项目

### 自动复制脚本（macOS/Linux）

创建 `copy_to_unity.sh`:
```bash
#!/bin/bash

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
fi

echo "复制 $PLATFORM 动态库到 Unity..."

# 创建目标目录
mkdir -p "$TARGET_DIR"

# 复制库文件
cp "$LIB_FILE" "$TARGET_DIR/"

echo "复制完成: $TARGET_DIR"
```

运行：
```bash
chmod +x copy_to_unity.sh
./copy_to_unity.sh
```

### 手动复制

#### macOS
```bash
mkdir -p "/Users/wepie/TestProject/MyProject(1)/Assets/Plugins/macOS"
cp build/lib/libJobSystem.dylib "/Users/wepie/TestProject/MyProject(1)/Assets/Plugins/macOS/"
```

#### Linux
```bash
mkdir -p "/path/to/unity/Assets/Plugins/x86_64"
cp build/lib/libJobSystem.so "/path/to/unity/Assets/Plugins/x86_64/"
```

#### Windows
```cmd
mkdir "C:\path\to\unity\Assets\Plugins\x86_64"
copy build\lib\Release\JobSystem.dll "C:\path\to\unity\Assets\Plugins\x86_64\"
```

## 故障排除

### 问题 1: CMake 未找到
```bash
# macOS
brew install cmake

# Linux
sudo apt-get install cmake  # 或 sudo yum install cmake
```

### 问题 2: 编译错误 - 找不到头文件
确保项目结构正确：
```
JobSystem/
├── CMakeLists.txt
└── JobSystem/
    ├── JobSystem.h
    ├── JobSystem.cpp
    ├── JobSystemCAPI.h
    ├── JobSystemCAPI.cpp
    └── ...
```

### 问题 3: 链接错误
- 确保所有 `.cpp` 文件都在 `CMakeLists.txt` 的 `JOBSYSTEM_SOURCES` 中
- 检查是否有循环依赖

### 问题 4: macOS 动态库加载失败
```bash
# 检查库的架构
lipo -info build/lib/libJobSystem.dylib

# 如需支持 Apple Silicon，使用 universal binary
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

## 清理构建

```bash
# 删除 build 目录
rm -rf build

# 或使用 CMake
cd build
cmake --build . --target clean
```

## 构建选项

### 禁用测试可执行文件
```bash
cmake .. -DBUILD_TESTS=OFF
```

### 指定 C++ 标准
```bash
cmake .. -DCMAKE_CXX_STANDARD=17
```

### Debug 构建
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

## 下一步

构建完成后，请参考 [Unity/README.md](Unity/README.md) 了解如何在 Unity 中使用。
