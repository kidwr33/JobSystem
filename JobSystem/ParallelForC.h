#pragma once
#include "JobSystem.h"
#include <cstdint>
#include <vector>
#include <mutex>

// C-style parallel_for without std::function or lambda
// 专门为 C API 设计，避免 std::function 问题

// C 风格回调类型
typedef void (*ParallelForCCallback)(void* data, uint32_t count, void* userData);

// 分割策略
struct CountSplitter {
    uint32_t threshold;
    CountSplitter(uint32_t t = 256) : threshold(t) {}
    bool ShouldSplit(uint32_t count) const { return count > threshold; }
};

// ParallelFor 数据结构（C 风格）
struct ParallelForDataC {
    JobSystem* jobSystem;
    void* data;
    uint32_t dataSize;
    uint32_t count;
    ParallelForCCallback callback;  // C 函数指针，不是 std::function
    void* userData;  // 用户数据（wrapper）
    CountSplitter splitter;  // ✅ 按值存储，避免悬空指针
    Job* parentJob;
};

// ParallelForDataC 线程局部对象池（完全无锁，零竞争）
class ParallelForDataCPool {
private:
    // 每个线程独立的对象池
    std::vector<ParallelForDataC*> pool;

    // 禁止拷贝
    ParallelForDataCPool(const ParallelForDataCPool&) = delete;
    ParallelForDataCPool& operator=(const ParallelForDataCPool&) = delete;

public:
    ParallelForDataCPool() = default;

    // 从池中分配一个对象（无锁，线程安全）
    ParallelForDataC* Allocate() {
        if (!pool.empty()) {
            ParallelForDataC* data = pool.back();
            pool.pop_back();
            return data;
        }
        // 池为空，创建新对象
        return new ParallelForDataC();
    }

    // 归还对象到池中（无锁，线程安全）
    void Free(ParallelForDataC* data) {
        if (!data) return;
        pool.push_back(data);
    }

    // 清理池中所有对象（析构时调用）
    ~ParallelForDataCPool() {
        for (auto* data : pool) {
            delete data;
        }
        pool.clear();
    }

    // 获取线程局部单例（每个线程独立的池）
    static ParallelForDataCPool& GetInstance() {
        thread_local ParallelForDataCPool instance;
        return instance;
    }
};

// ParallelFor 作业函数（C 风格）
void ParallelForJobC(Job* job, void* jobData);

// parallel_for 主函数（C 风格）
Job* parallel_for_c(
    JobSystem* jobSystem,
    void* data,
    uint32_t count,
    uint32_t elementSize,
    ParallelForCCallback callback,
    void* userData,
    CountSplitter& splitter
);
