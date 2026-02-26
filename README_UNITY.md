# JobSystem for Unity

将 JobSystem 编译成动态库并在 Unity 中使用的完整指南。

## 📋 目录

1. [快速开始](#快速开始)
2. [详细步骤](#详细步骤)
3. [文件说明](#文件说明)
4. [Unity 集成](#unity-集成)
5. [常见问题](#常见问题)

## 🚀 快速开始

### 1. 安装依赖

**macOS:**
```bash
# 安装 CMake (如果还没安装)
brew install cmake
```

**Linux:**
```bash
sudo apt-get install build-essential cmake  # Ubuntu/Debian
# 或
sudo yum install gcc-c++ make cmake         # CentOS/RHEL
```

**Windows:**
- 安装 Visual Studio 2017+
- 安装 CMake: https://cmake.org/download/

### 2. 编译动态库

```bash
# macOS / Linux
cd /Users/wepie/JobSystem
./build.sh
```

```cmd
REM Windows
cd C:\path\to\JobSystem
build.bat
```

### 3. 复制到 Unity

```bash
# macOS / Linux - 自动复制（推荐）
./copy_to_unity.sh
```

或者手动复制：

**macOS:**
```bash
cp build/lib/libJobSystem.dylib "/Users/wepie/TestProject/MyProject(1)/Assets/Plugins/macOS/"
```

**Linux:**
```bash
cp build/lib/libJobSystem.so "/path/to/unity/Assets/Plugins/x86_64/"
```

**Windows:**
```cmd
copy build\lib\Release\JobSystem.dll "C:\path\to\unity\Assets\Plugins\x86_64\"
```

### 4. 在 Unity 中使用

1. 复制 C# 文件到 Unity 项目：
   - `Unity/JobSystemWrapper.cs`
   - `Unity/JobSystemExample.cs` (可选，示例代码)

2. 在 Unity Editor 中配置动态库导入设置（详见下文）

3. 将 `JobSystemExample.cs` 挂载到场景中的 GameObject 上测试

## 📖 详细步骤

### 步骤 1: 构建动态库

项目已配置为构建动态库：

```cmake
# CMakeLists.txt 配置要点:
add_library(JobSystem SHARED ${JOBSYSTEM_SOURCES})  # 构建动态库
target_compile_definitions(JobSystem PRIVATE JOBSYSTEM_BUILD_DLL)  # 导出符号
```

运行构建脚本：

```bash
./build.sh  # macOS/Linux
build.bat   # Windows
```

构建产物：
- **macOS**: `build/lib/libJobSystem.dylib`
- **Linux**: `build/lib/libJobSystem.so`
- **Windows**: `build\lib\Release\JobSystem.dll`

### 步骤 2: 理解项目结构

```
JobSystem/
├── CMakeLists.txt                 # CMake 配置文件
├── build.sh / build.bat          # 构建脚本
├── copy_to_unity.sh              # Unity 复制脚本
├── BUILD.md                       # 详细构建文档
├── JobSystem/                     # 核心代码
│   ├── JobSystem.h/cpp           # JobSystem 主类
│   ├── JobSystemCAPI.h/cpp       # C API 接口（Unity 使用）
│   ├── JobSystemExport.h         # 导出宏定义
│   ├── Job.h                     # Job 结构
│   ├── JobAllocator.h/cpp        # Job 内存分配器
│   ├── WorkThreadStealQueue.h/cpp # 工作窃取队列
│   └── ParallelFor.h             # parallel_for 实现
└── Unity/                         # Unity 集成文件
    ├── README.md                 # Unity 详细文档
    ├── JobSystemWrapper.cs       # C# 包装类
    └── JobSystemExample.cs       # 使用示例
```

### 步骤 3: Unity 项目结构

```
TestProject/MyProject(1)/
└── Assets/
    ├── Plugins/                   # 动态库目录
    │   ├── macOS/
    │   │   └── libJobSystem.dylib
    │   ├── x86_64/
    │   │   ├── libJobSystem.so    (Linux)
    │   │   └── JobSystem.dll      (Windows 64-bit)
    │   └── x86/
    │       └── JobSystem.dll      (Windows 32-bit)
    └── Scripts/
        └── JobSystem/
            ├── JobSystemWrapper.cs
            └── JobSystemExample.cs
```

## 🎮 Unity 集成

### 1. 配置动态库导入设置

在 Unity Editor 中选中动态库文件，在 Inspector 中配置：

#### macOS (.dylib)
- ✅ Select platforms for plugin: Editor, Standalone
- Platform settings:
  - CPU: x86_64
  - OS: macOS

#### Linux (.so)
- ✅ Select platforms for plugin: Standalone
- Platform settings:
  - CPU: x86_64
  - OS: Linux

#### Windows (.dll)
- ✅ Select platforms for plugin: Editor, Standalone
- Platform settings:
  - CPU: x86_64 (或 x86)
  - OS: Windows

### 2. 基本使用示例

```csharp
using UnityEngine;
using JobSystemNative;

public class MyJobTest : MonoBehaviour
{
    private JobSystemWrapper jobSystem;

    void Start()
    {
        // 创建 JobSystem
        jobSystem = new JobSystemWrapper();
        Debug.Log("JobSystem 初始化完成");
    }

    void Update()
    {
        if (jobSystem == null) return;

        // 帧开始
        jobSystem.FrameStart();

        // 创建简单 Job
        JobCallback callback = (job, userData) =>
        {
            // 这里运行在工作线程中
            // 执行耗时计算...
            int sum = 0;
            for (int i = 0; i < 1000000; i++)
            {
                sum += i;
            }
        };

        IntPtr job = jobSystem.CreateJob(callback);
        jobSystem.RunJob(job);
        jobSystem.WaitJob(job);

        // 帧结束
        jobSystem.FrameEnd();
    }

    void OnDestroy()
    {
        // 释放资源
        jobSystem?.Dispose();
    }
}
```

### 3. 高级功能

详见 [Unity/README.md](Unity/README.md) 和 `Unity/JobSystemExample.cs`，包括：
- 父子 Job（嵌套任务）
- Continuation（依赖关系）
- 用户数据传递
- 性能优化技巧

## 📝 文件说明

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | CMake 构建配置 |
| `build.sh` | macOS/Linux 构建脚本 |
| `build.bat` | Windows 构建脚本 |
| `copy_to_unity.sh` | 自动复制到 Unity 的脚本 |
| `BUILD.md` | 详细构建文档 |
| `JobSystemExport.h` | 跨平台导出宏定义 |
| `JobSystemCAPI.h/cpp` | C API 接口实现 |
| `Unity/README.md` | Unity 集成详细文档 |
| `Unity/JobSystemWrapper.cs` | Unity C# 包装类 |
| `Unity/JobSystemExample.cs` | 完整使用示例 |

## ❓ 常见问题

### Q1: 构建失败 - CMake not found

**A:** 安装 CMake:
```bash
# macOS
brew install cmake

# Linux
sudo apt-get install cmake
```

### Q2: Unity 中报错 DllNotFoundException

**A:** 检查以下几点:
1. 动态库是否在正确的 `Assets/Plugins/` 子目录中
2. Unity Inspector 中是否正确配置了平台设置
3. 库文件名是否正确（区分大小写）

### Q3: Unity 崩溃或函数调用失败

**A:** 可能的原因:
1. 未调用 `FrameStart/FrameEnd`
2. 在工作线程中访问了 Unity API
3. GCHandle 未正确管理
4. JobSystem 未正确初始化

### Q4: macOS 提示无法验证开发者

**A:** 运行以下命令:
```bash
sudo xattr -r -d com.apple.quarantine build/lib/libJobSystem.dylib
```

### Q5: 如何构建 Universal Binary（支持 Intel + Apple Silicon）

**A:** 在 CMake 配置时指定:
```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

### Q6: 性能不如预期

**A:** 检查以下几点:
1. Job 粒度是否太小（开销大于收益）
2. 是否在每帧创建过多 Job
3. 是否有不必要的 `WaitJob` 调用
4. 考虑使用父子 Job 来批量处理

## 🔗 更多资源

- [详细构建文档](BUILD.md)
- [Unity 集成文档](Unity/README.md)
- [使用示例代码](Unity/JobSystemExample.cs)

## 📄 许可

根据你的项目许可协议使用。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

**提示:** 首次使用请先阅读 [BUILD.md](BUILD.md) 和 [Unity/README.md](Unity/README.md) 获取完整信息。
