# JobSystem

高性能的 C++ Job 系统，支持工作窃取、并行计算和任务依赖管理。

## ✨ 特性

- ⚡ **工作窃取（Work-Stealing）**：高效的负载均衡算法
- 🔄 **并行 For 循环**：简化数据并行处理
- 🔗 **任务依赖管理**：支持 Continuation 和依赖链
- 🎯 **C API 接口**：可与 Unity 等引擎集成
- 🧵 **高效线程池**：自动管理工作线程
- 💾 **对象池优化**：减少内存分配开销

## 📋 前置要求

### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 CMake (使用 Homebrew)
brew install cmake
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake

# CentOS/RHEL
sudo yum install gcc-c++ make cmake
```

## 🚀 快速开始

### 使用构建脚本（推荐）

```bash
# Release 模式（默认，优化构建）
./build.sh

# Debug 模式（包含调试符号，用于调试）
./build.sh debug
```

### 手动构建

```bash
# 创建并进入 build 目录
mkdir -p build && cd build

# 配置 CMake（Release 或 Debug）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译（并行构建）
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

# 返回项目根目录
cd ..
```

## 📦 输出文件

构建成功后，动态库位于：

| 平台 | 路径 | Release 大小 | Debug 大小 |
|------|------|-------------|-----------|
| macOS | `build/lib/libJobSystem.dylib` | ~96KB | ~234KB |
| Linux | `build/lib/libJobSystem.so` | ~100KB | ~250KB |

## ✅ 验证构建

### macOS
```bash
# 查看动态库依赖
otool -L build/lib/libJobSystem.dylib

# 查看导出符号
nm -gU build/lib/libJobSystem.dylib | grep JobSystem

# 查看架构信息
lipo -info build/lib/libJobSystem.dylib
```

### Linux
```bash
# 查看动态库依赖
ldd build/lib/libJobSystem.so

# 查看导出符号
nm -D build/lib/libJobSystem.so | grep JobSystem
```

## 🎮 Unity 集成

### 方法 1：使用自动复制脚本

编辑 `copy_to_unity.sh`，设置你的 Unity 项目路径，然后运行：

```bash
./copy_to_unity.sh
```

### 方法 2：手动复制

#### macOS
```bash
# 复制到 Unity 项目
cp build/lib/libJobSystem.dylib "$UNITY_PROJECT/Assets/Plugins/macOS/"
```

#### Linux
```bash
# 复制到 Unity 项目
cp build/lib/libJobSystem.so "$UNITY_PROJECT/Assets/Plugins/x86_64/"
```

详细的 Unity 集成指南请参考 [README_UNITY.md](README_UNITY.md)

## 🔧 高级选项

### 构建配置

```bash
# 禁用测试可执行文件
cmake .. -DBUILD_TESTS=OFF

# 指定 C++ 标准
cmake .. -DCMAKE_CXX_STANDARD=17

# 支持 Apple Silicon 和 Intel（macOS Universal Binary）
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

### 清理构建

```bash
# 完全清理
rm -rf build

# 或使用 CMake clean
cd build && cmake --build . --target clean
```

## 📁 项目结构

```
JobSystem/
├── CMakeLists.txt          # CMake 构建配置
├── build.sh                # 构建脚本
├── copy_to_unity.sh        # Unity 复制脚本
├── README.md               # 本文件
├── README_UNITY.md         # Unity 集成指南
├── BUILD.md                # 详细构建文档
└── JobSystem/              # 源代码目录
    ├── JobSystem.h/cpp           # 核心 Job 系统
    ├── JobSystemCAPI.h/cpp       # C API 接口
    ├── ParallelFor.h             # 并行 For 实现
    ├── ParallelForC.h/cpp        # C API 并行 For
    ├── ParticleUpdateNative.h/cpp # 粒子系统示例
    ├── WorkThreadStealQueue.cpp  # 工作窃取队列
    ├── JobAllocator.cpp          # 对象池分配器
    └── main.cpp                  # 测试程序
```

## 🐛 故障排除

### CMake 未找到
```bash
# macOS
brew install cmake

# Linux
sudo apt-get install cmake
```

### 编译错误 - 链接失败
检查 `CMakeLists.txt` 中的 `JOBSYSTEM_SOURCES` 是否包含所有 `.cpp` 文件：
```cmake
set(JOBSYSTEM_SOURCES
    JobSystem/JobSystem.cpp
    JobSystem/JobAllocator.cpp
    JobSystem/WorkThreadStealQueue.cpp
    JobSystem/JobSystemCAPI.cpp
    JobSystem/ParallelForC.cpp
    JobSystem/ParticleUpdateNative.cpp
)
```

### macOS 动态库加载失败
```bash
# 检查代码签名
codesign -dv build/lib/libJobSystem.dylib

# 重新签名
codesign -s - -f build/lib/libJobSystem.dylib
```

### 性能问题
- 确保使用 **Release** 模式构建（默认）
- Debug 模式性能较慢（约 2-3 倍），仅用于调试

## 📚 相关文档

- [Unity 集成指南](README_UNITY.md) - 如何在 Unity 中使用
- [构建详细文档](BUILD.md) - 更详细的构建说明
- [调试指南](DEBUG_ANALYSIS.md) - 调试和性能分析


