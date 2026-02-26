#pragma once
#include "JobSystem.h"
#include <cstdint>
#include <functional>
#include <cstdio>

// 分割策略：基于元素数量
struct CountSplitter {
    uint32_t threshold;

    CountSplitter(uint32_t t = 256) : threshold(t) {}

    bool ShouldSplit(uint32_t count) const {
        return count > threshold;
    }
};

// 分割策略：基于数据大小（考虑缓存）
struct DataSizeSplitter {
    size_t elementSize;
    size_t cacheSizeThreshold;

    DataSizeSplitter(size_t elemSize, size_t cacheSize = 32 * 1024) // 默认 32KB (L1 cache)
        : elementSize(elemSize), cacheSizeThreshold(cacheSize) {}

    bool ShouldSplit(uint32_t count) const {
        return (count * elementSize) > cacheSizeThreshold;
    }
};

// parallel_for 的数据结构
template<typename T, typename Func, typename Splitter>
struct ParallelForData {
    JobSystem* jobSystem;
    T* data;
    uint32_t dataSize;
    uint32_t count;
    Func func;
    Splitter* splitter;
    Job* parentJob;
    void* userData;  // 用于存储 wrapper 指针
};

// parallel_for 作业函数
template<typename T, typename Func, typename Splitter>
void ParallelForJob(Job* job, void* jobData) {
    char logBuf[512];
    std::thread::id threadId = std::this_thread::get_id();

    auto* pfData = static_cast<ParallelForData<T, Func, Splitter>*>(jobData);

    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] ENTRY - job=%p, jobData=%p, data=%p, count=%u, dataSize=%u\n",
             (void*)job, jobData, (void*)pfData->data, pfData->count, pfData->dataSize);
    pfData->jobSystem->Log(logBuf);

    // 如果范围足够小，直接执行
    bool shouldSplit = pfData->splitter->ShouldSplit(pfData->count);
    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] shouldSplit=%d (count=%u, threshold=%u)\n",
             shouldSplit, pfData->count, pfData->splitter->threshold);
    pfData->jobSystem->Log(logBuf);

    if (!shouldSplit) {
        // 执行用户函数处理这个范围
        snprintf(logBuf, sizeof(logBuf),
                 "[ParallelForJob] LEAF BEFORE CALLBACK: thread=%lu, data=%p, count=%u\n",
                 (unsigned long)std::hash<std::thread::id>{}(threadId), (void*)pfData->data, pfData->count);
        pfData->jobSystem->Log(logBuf);

        // ============ 关键点：调用用户函数 ============
        snprintf(logBuf, sizeof(logBuf),
                 "[ParallelForJob] ABOUT TO CALL func(): funcPtr=%p\n",
                 (void*)&pfData->func);
        pfData->jobSystem->Log(logBuf);

        pfData->func(pfData->data, pfData->count);

        snprintf(logBuf, sizeof(logBuf),
                 "[ParallelForJob] AFTER func() RETURNED: thread=%lu\n",
                 (unsigned long)std::hash<std::thread::id>{}(threadId));
        pfData->jobSystem->Log(logBuf);
        // ============================================

        snprintf(logBuf, sizeof(logBuf),
                 "[ParallelForJob] LEAF AFTER CALLBACK: thread=%lu, deleting pfData=%p\n",
                 (unsigned long)std::hash<std::thread::id>{}(threadId), (void*)pfData);
        pfData->jobSystem->Log(logBuf);

        delete pfData;
        return;
    }

    // 否则，将范围分成两半
    uint32_t leftCount = pfData->count / 2;
    uint32_t rightCount = pfData->count - leftCount;

    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] SPLIT: leftCount=%u, rightCount=%u\n", leftCount, rightCount);
    pfData->jobSystem->Log(logBuf);

    // 创建左半部分的作业
    T* leftDataPtr = pfData->data;
    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] LEFT: ptr=%p, count=%u\n", (void*)leftDataPtr, leftCount);
    pfData->jobSystem->Log(logBuf);

    auto* leftData = new ParallelForData<T, Func, Splitter>{
        pfData->jobSystem,
        leftDataPtr,
        pfData->dataSize,
        leftCount,
        pfData->func,
        pfData->splitter,
        job
    };

    Job* leftJob = pfData->jobSystem->CreateJob(job, ParallelForJob<T, Func, Splitter>);
    leftJob->data = leftData;
    pfData->jobSystem->RunJob(leftJob);

    // 创建右半部分的作业
    T* rightDataPtr = pfData->data + leftCount * pfData->dataSize;
    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] RIGHT: ptr=%p (offset=%u bytes), count=%u\n",
             (void*)rightDataPtr, leftCount * pfData->dataSize, rightCount);
    pfData->jobSystem->Log(logBuf);

    auto* rightData = new ParallelForData<T, Func, Splitter>{
        pfData->jobSystem,
        rightDataPtr,
        pfData->dataSize,
        rightCount,
        pfData->func,
        pfData->splitter,
        job
    };

    Job* rightJob = pfData->jobSystem->CreateJob(job, ParallelForJob<T, Func, Splitter>);
    rightJob->data = rightData;
    pfData->jobSystem->RunJob(rightJob);

    snprintf(logBuf, sizeof(logBuf),
             "[ParallelForJob] EXIT SPLIT, deleting parent pfData=%p\n", (void*)pfData);
    pfData->jobSystem->Log(logBuf);

    delete pfData;
}

// parallel_for 主函数
template<typename T, typename Func, typename Splitter = CountSplitter>
Job* parallel_for(JobSystem* jobSystem, T* data, uint32_t count, uint32_t elementsize, Func&& func, Splitter& splitter) {
    // 创建根作业
    Job* rootJob = jobSystem->CreateJob([](Job* job, void* data) {
        // 空的根作业，只用于等待所有子作业完成
    });

    // 创建初始的 parallel_for 数据
    auto* pfData = new ParallelForData<T, Func, Splitter>{
        jobSystem,
        data,
        elementsize,
        count,
        std::forward<Func>(func),
        &splitter,  // 现在splitter是引用，&splitter指向外部管理的对象
        rootJob
    };

    // 创建并运行第一个作业
    Job* firstJob = jobSystem->CreateJob(rootJob, ParallelForJob<T, Func, Splitter>);
    firstJob->data = pfData;
    jobSystem->RunJob(firstJob);

    return rootJob;
}
