#include "JobSystemCAPI.h"
#include "JobSystem.h"
#include "Profiler.h"
#include "ParallelForC.h"  // 使用 C 风格版本，避免 std::function/lambda 问题
#include <new>

// 包装结构，用于存储回调和用户数据
struct JobCallbackWrapper {
    JobCallback callback;
    void* userData;
};

// ParallelFor 包装结构，用于存储回调
struct ParallelForWrapper {
    ParallelForCallback callback;  // C# 的2参数回调
    CountSplitter* splitter;  // 堆上分配的splitter
    JobSystem* jobSystem;  // 用于日志输出

    // 显式构造函数，确保正确初始化
    ParallelForWrapper(ParallelForCallback cb, CountSplitter* spl, JobSystem* js)
        : callback(cb), splitter(spl), jobSystem(js) {}
};

// C 适配器函数：将3参数回调适配到2参数的 C# delegate
static void ParallelForCAdapterFunc(void* data, uint32_t count, void* userData) {
    ParallelForWrapper* wrapper = static_cast<ParallelForWrapper*>(userData);

    char logBuf[512];

    // ❌ 先尝试写日志（即使 wrapper 可能为 NULL）
    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForCAdapterFunc] ENTRY: data=%p, count=%u, userData=%p, wrapper=%p\n",
             data, count, userData, (void*)wrapper);

    // 如果 wrapper 有效，用 jobSystem->Log，否则用 printf
    if (wrapper && wrapper->jobSystem) {
        wrapper->jobSystem->Log(logBuf);
    } else {
        printf("%s", logBuf);
        snprintf(logBuf, sizeof(logBuf),
                 "[ParallelForCAdapterFunc] ERROR: Invalid wrapper or jobSystem=NULL\n");
        printf("%s", logBuf);
        return;  // 避免崩溃
    }

    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForCAdapterFunc] About to call C# delegate=%p\n",
             (void*)wrapper->callback);
    wrapper->jobSystem->Log(logBuf);

    // 调用2参数的 C# delegate
    wrapper->callback(data, count);

    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForCAdapterFunc] C# callback returned successfully\n");
    wrapper->jobSystem->Log(logBuf);
}

// 内部适配函数：将 C++ JobFunction 转换为 C 回调
static void JobFunctionAdapter(Job* job, void* data) {
    if (data) {
        JobCallbackWrapper* wrapper = static_cast<JobCallbackWrapper*>(data);
        if (wrapper->callback) {
            wrapper->callback(job, wrapper->userData);
        }
        delete wrapper;
        job->data = nullptr; // 清理
    }
}

// ====== JobSystem 生命周期管理 ======

JOBSYSTEM_C_API JobSystem* JobSystem_Create() {
    JobSystem* system = new (std::nothrow) JobSystem();
    if (system) {
        system->Initialize();
    }
    return system;
}

JOBSYSTEM_C_API void JobSystem_Destroy(JobSystem* system) {
    if (system) {
        system->ShutDown();
        delete system;
    }
}

JOBSYSTEM_C_API void JobSystem_FrameStart(JobSystem* system) {
    if (system) {
        system->FrameStart();
    }
}

JOBSYSTEM_C_API void JobSystem_FrameEnd(JobSystem* system) {
    if (system) {
        system->FrameEnd();
    }
}

// ====== Job 操作 ======

JOBSYSTEM_C_API Job* JobSystem_CreateJob(JobSystem* system, JobCallback callback, void* userData) {
    if (!system) return nullptr;

    // 创建包装器
    JobCallbackWrapper* wrapper = new (std::nothrow) JobCallbackWrapper();
    if (!wrapper) return nullptr;

    wrapper->callback = callback;
    wrapper->userData = userData;

    Job* job = system->CreateJob(JobFunctionAdapter);
    if (job) {
        job->data = wrapper;
    } else {
        delete wrapper;
    }

    return job;
}

JOBSYSTEM_C_API Job* JobSystem_CreateChildJob(JobSystem* system, Job* parent, JobCallback callback, void* userData) {
    if (!system || !parent) return nullptr;

    // 创建包装器
    JobCallbackWrapper* wrapper = new (std::nothrow) JobCallbackWrapper();
    if (!wrapper) return nullptr;

    wrapper->callback = callback;
    wrapper->userData = userData;

    Job* job = system->CreateJob(parent, JobFunctionAdapter);
    if (job) {
        job->data = wrapper;
    } else {
        delete wrapper;
    }

    return job;
}

JOBSYSTEM_C_API void JobSystem_RunJob(JobSystem* system, Job* job) {
    if (system && job) {
        system->RunJob(job);
    }
}

JOBSYSTEM_C_API void JobSystem_WaitJob(JobSystem* system, Job* job) {
    if (system && job) {
        system->WaitJob(job);
    }
}

JOBSYSTEM_C_API void JobSystem_AddContinuation(JobSystem* system, Job* job, Job* continuation) {
    if (system && job && continuation) {
        system->AddContinuation(job, continuation);
    }
}

JOBSYSTEM_C_API void* Job_GetUserData(Job* job) {
    if (!job || !job->data) return nullptr;

    JobCallbackWrapper* wrapper = static_cast<JobCallbackWrapper*>(job->data);
    return wrapper->userData;
}

JOBSYSTEM_C_API void Job_SetUserData(Job* job, void* userData) {
    if (job && job->data) {
        JobCallbackWrapper* wrapper = static_cast<JobCallbackWrapper*>(job->data);
        wrapper->userData = userData;
    }
}

// Profiler API
JOBSYSTEM_C_API void Profiler_BeginSession(const char* filepath) {
    Profiler::Instance().BeginSession(filepath);
}

JOBSYSTEM_C_API void Profiler_EndSession() {
    Profiler::Instance().EndSession();
}

// ====== Parallel For 实现 ======

JOBSYSTEM_C_API Job* JobSystem_ParallelFor(
    JobSystem* system,
    void* data,
    uint32_t count,
    size_t elementSize,
    ParallelForCallback callback,
    uint32_t threshold
) {
    if (!system || !data || !callback || count == 0 || elementSize == 0) {
        return nullptr;
    }

    // 创建堆上的splitter（修复悬空指针问题）
    CountSplitter* splitter = new (std::nothrow) CountSplitter(threshold);
    if (!splitter) {
        return nullptr;
    }

    // 创建包装器存储用户回调和splitter（使用显式构造函数）
    ParallelForWrapper* wrapper = new (std::nothrow) ParallelForWrapper(callback, splitter, system);
    if (!wrapper) {
        delete splitter;
        return nullptr;
    }

    char* byteData = (char*)data;

    // ✅ 不使用 lambda，直接使用 C 风格的 parallel_for_c
    // 调用 C 风格 parallel_for，避免 std::function/lambda 问题
    Job* rootJob = parallel_for_c(
        system,
        byteData,
        count,
        static_cast<uint32_t>(elementSize),
        ParallelForCAdapterFunc,  // 使用适配器函数
        wrapper,                   // 将 wrapper 作为 userData 传递
        *splitter
    );

    // 创建清理 Job，使用 continuation 确保在 rootJob 完成后执行
    if (rootJob) {
        Job* cleanupJob = system->CreateJob([](Job* job, void* data) {
            ParallelForWrapper* w = static_cast<ParallelForWrapper*>(data);
            delete w->splitter;  // 删除splitter
            delete w;            // 删除wrapper
        });
        cleanupJob->data = wrapper;

        // 将 cleanupJob 作为 rootJob 的 continuation
        system->AddContinuation(rootJob, cleanupJob);   
    } else {
        delete splitter;
        delete wrapper;
    }

    return rootJob;
}

// ✅ 纯C++函数指针版本 - 零跨界开销
JOBSYSTEM_C_API Job* JobSystem_ParallelForNative(
    JobSystem* system,
    void* data,
    uint32_t count,
    size_t elementSize,
    NativeCallback nativeFuncPtr,
    void* userData,
    uint32_t threshold
) {
    if (!system || !data || !nativeFuncPtr || count == 0 || elementSize == 0) {
        return nullptr;
    }

    // 创建堆上的splitter
    CountSplitter* splitter = new (std::nothrow) CountSplitter(threshold);
    if (!splitter) {
        return nullptr;
    }

    char* byteData = (char*)data;

    // ✅ 直接使用纯C++函数指针，零delegate开销
    Job* rootJob = parallel_for_c(
        system,
        byteData,
        count,
        static_cast<uint32_t>(elementSize),
        nativeFuncPtr,  // ✅ 纯C++函数指针，不是C# delegate
        userData,       // 用户数据（如PhysicsParams）
        *splitter
    );

    // 创建清理 Job
    if (rootJob) {
        Job* cleanupJob = system->CreateJob([](Job* job, void* data) {
            CountSplitter* spl = static_cast<CountSplitter*>(data);
            delete spl;
        });
        cleanupJob->data = splitter;
        system->AddContinuation(rootJob, cleanupJob);
    } else {
        delete splitter;
    }

    return rootJob;
}
