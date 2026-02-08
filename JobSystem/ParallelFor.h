#pragma once
#include "JobSystem.h"
#include <cstdint>
#include <functional>

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
    uint32_t count;
    Func func;
    Splitter splitter;
    Job* parentJob;
};

// parallel_for 作业函数
template<typename T, typename Func, typename Splitter>
void ParallelForJob(Job* job, void* jobData) {
    auto* pfData = static_cast<ParallelForData<T, Func, Splitter>*>(jobData);

    // 如果范围足够小，直接执行
    if (!pfData->splitter.ShouldSplit(pfData->count)) {
        // 执行用户函数处理这个范围
        pfData->func(pfData->data, pfData->count);
        delete pfData;
        return;
    }

    // 否则，将范围分成两半
    uint32_t leftCount = pfData->count / 2;
    uint32_t rightCount = pfData->count - leftCount;

    // 创建左半部分的作业
    auto* leftData = new ParallelForData<T, Func, Splitter>{
        pfData->jobSystem,
        pfData->data,
        leftCount,
        pfData->func,
        pfData->splitter,
        job
    };

    Job* leftJob = pfData->jobSystem->CreateJob(job, ParallelForJob<T, Func, Splitter>);
    leftJob->data = leftData;
    pfData->jobSystem->RunJob(leftJob);

    // 创建右半部分的作业
    auto* rightData = new ParallelForData<T, Func, Splitter>{
        pfData->jobSystem,
        pfData->data + leftCount,
        rightCount,
        pfData->func,
        pfData->splitter,
        job
    };

    Job* rightJob = pfData->jobSystem->CreateJob(job, ParallelForJob<T, Func, Splitter>);
    rightJob->data = rightData;
    pfData->jobSystem->RunJob(rightJob);

    delete pfData;
}

// parallel_for 主函数
template<typename T, typename Func, typename Splitter = CountSplitter>
Job* parallel_for(JobSystem* jobSystem, T* data, uint32_t count, Func&& func, Splitter splitter = Splitter()) {
    // 创建根作业
    Job* rootJob = jobSystem->CreateJob([](Job* job, void* data) {
        // 空的根作业，只用于等待所有子作业完成
    });

    // 创建初始的 parallel_for 数据
    auto* pfData = new ParallelForData<T, Func, Splitter>{
        jobSystem,
        data,
        count,
        std::forward<Func>(func),
        splitter,
        rootJob
    };

    // 创建并运行第一个作业
    Job* firstJob = jobSystem->CreateJob(rootJob, ParallelForJob<T, Func, Splitter>);
    firstJob->data = pfData;
    jobSystem->RunJob(firstJob);

    return rootJob;
}
