# Unity JobSystem 问题分析和修复报告

## 问题1: 线程冻结 (已修复)

### 现象
Unity主线程卡死，表现为无响应状态。

### 根本原因分析

通过 `sample` 工具采样Unity进程，发现主线程调用栈：
```
JobSystem_WaitJob (JobSystemCAPI.cpp:102)
  → JobSystem::WaitJob (JobSystem.cpp:153)
    → JobSystem::GetJob (JobSystem.cpp:108)
      → JobSystem::Yield()
        → std::this_thread::yield() (busy-wait循环)
```

发现两个并发缺陷：

#### 缺陷1: 竞态条件导致工作线程退出
**位置**: [JobSystem.cpp:8-31](JobSystem/JobSystem.cpp#L8-L31)

```cpp
void JobSystem::Initialize() {
    // ...创建队列和资源...

    // 启动工作线程
    for (int i = 0; i < numThreads - 1; i++) {
        workerThreads.emplace_back([this, i]() {
            WorkerThreadFunction(i + 1);
        });
    }
    isRunning = true;  // ⚠️ 问题：在线程创建后才设置
}
```

工作线程函数：
```cpp
void JobSystem::WorkerThreadFunction(int threadIndex) {
    // ...初始化...
    while (isRunning) {  // ⚠️ 竞态条件！
        Job* job = GetJob();
        if (job) {
            ExecuteJob(job);
        }
    }
}
```

**问题**：如果工作线程在 `isRunning = true` 执行前就读取了该变量（读到false），线程会立即退出。
**后果**：没有工作线程来执行任务，所有Job永远无法完成。

#### 缺陷2: WaitJob实现导致主线程busy-wait
**位置**: [JobSystem.cpp:151-158](JobSystem/JobSystem.cpp#L151-L158)

```cpp
void JobSystem::WaitJob(Job* job) {
    while (!HasJobCompleted(job)) {
        Job* nextJob = GetJob();
        if (nextJob) {
            ExecuteJob(nextJob);
        }
        // ⚠️ 问题：GetJob()返回nullptr时立即重试，CPU空转
    }
}
```

**问题**：当工作线程都退出后，队列为空，`GetJob()` 持续返回nullptr，主线程陷入无限busy-wait。
**后果**：CPU 100%使用率，线程冻结。

### 修复方案

#### 修复1: 竞态条件
```cpp
void JobSystem::Initialize() {
    // ...创建队列和资源...

    // ✅ 在创建线程前设置标志
    isRunning = true;

    // 启动工作线程
    for (int i = 0; i < numThreads - 1; i++) {
        workerThreads.emplace_back([this, i]() {
            WorkerThreadFunction(i + 1);
        });
    }
}
```

#### 修复2: Busy-wait
```cpp
void JobSystem::WaitJob(Job* job) {
    while (!HasJobCompleted(job)) {
        Job* nextJob = GetJob();
        if (nextJob) {
            ExecuteJob(nextJob);
        } else {
            // ✅ 没有任务时让出CPU
            Yield();
        }
    }
}
```

---

## 问题2: Unity闪退 - 代码签名无效 (已修复)

### 现象
Unity启动后立即崩溃，进程被系统终止。

### 崩溃日志分析
```json
{
  "exception": {
    "type": "EXC_BAD_ACCESS",
    "signal": "SIGKILL (Code Signature Invalid)",
    "subtype": "UNKNOWN_0x32 at 0x000000014fea0000"
  },
  "termination": {
    "namespace": "CODESIGNING",
    "indicator": "Invalid Page"
  }
}
```

### 根本原因
macOS Gatekeeper检测到 `libJobSystem.dylib` 没有有效的代码签名，认为是不可信的代码，将Unity进程终止。

这在macOS 10.15+（尤其是Apple Silicon Mac）上特别严格。

### 修复方案

#### 1. 立即修复（手动）
```bash
# 对库进行ad-hoc签名
codesign -s - -f build/lib/libJobSystem.dylib
codesign -s - -f "/Users/wepie/TestProject/My project (1)/Assets/Plugins/macOS/libJobSystem.dylib"
```

#### 2. 自动化修复（脚本）
更新 [build.sh](build.sh) 和 [copy_to_unity.sh](copy_to_unity.sh)，在构建和复制后自动签名：

```bash
# 在build.sh中添加
if [[ "$PLATFORM" == "macOS" ]]; then
    codesign -s - -f "$LIB_PATH"
fi

# 在copy_to_unity.sh中添加
if [[ "$PLATFORM" == "macOS" ]]; then
    codesign -s - -f "$TARGET_DIR/$(basename $LIB_FILE)"
fi
```

---

## 验证清单

- [x] 修复竞态条件（isRunning标志）
- [x] 修复busy-wait（WaitJob添加Yield）
- [x] 添加代码签名
- [x] 更新构建脚本自动签名
- [x] 更新复制脚本自动签名
- [x] 重新编译并部署到Unity

---

## 使用建议

### 每次修改代码后
```bash
# 1. 构建（自动签名）
./build.sh

# 2. 复制到Unity（自动签名）
./copy_to_unity.sh

# 3. 重启Unity Editor（确保加载新库）
```

### 如果遇到崩溃
1. 检查崩溃报告：`~/Library/Logs/DiagnosticReports/Unity-*.ips`
2. 检查Unity日志：`~/Library/Logs/Unity/Editor.log`
3. 验证库签名：`codesign -vv path/to/libJobSystem.dylib`

---

## 技术细节

### 为什么需要代码签名？
- macOS安全机制要求所有可执行代码都要签名
- Unity在加载动态库时会触发系统验证
- 未签名或签名无效的库会被拒绝加载
- Ad-hoc签名（`codesign -s -`）足够用于本地开发

### 为什么工作线程会退出？
- `std::atomic<bool>` 保证原子性，但不保证线程间的happens-before关系
- 线程创建和 `isRunning` 赋值之间存在时间窗口
- 某些线程可能在 `isRunning = true` 前就开始执行
- C++ memory model允许这种重排序

### 为什么需要Yield？
- 空循环会占用CPU 100%
- `Yield()` 告诉操作系统让出时间片
- 其他线程有机会执行
- 避免无谓的CPU消耗

---

生成时间: 2026-02-15
分析工具: lldb, sample, codesign
