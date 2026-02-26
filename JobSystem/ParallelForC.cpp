#include "ParallelForC.h"
#include <algorithm>
#include <vector>

// 空的 C 函数，用于 rootJob
static void EmptyJobFunction(Job*, void*) {
    // 什么都不做，只用于等待子 jobs
}

// 工作范围结构
struct WorkRange {
    uint32_t start;
    uint32_t count;
};

// 叶子节点 Job：直接执行回调，无需分割
void ParallelForLeafJobC(Job* job, void* jobData) {
    auto* pfData = static_cast<ParallelForDataC*>(jobData);

    // 直接调用 C 函数指针
    pfData->callback(pfData->data, pfData->count, pfData->userData);

    // 归还到对象池
    ParallelForDataCPool::GetInstance().Free(pfData);
}


Job* parallel_for_c(
    JobSystem* jobSystem,
    void* data,
    uint32_t count,
    uint32_t elementSize,
    ParallelForCCallback callback,
    void* userData,
    CountSplitter& splitter
) {
    // 创建根 job（使用 C 函数，不用 lambda）
    Job* rootJob = jobSystem->CreateJob(EmptyJobFunction);

    // ✅ 动态调整 batch size：避免创建过多 Job
    uint32_t batchSize = splitter.threshold;

    // 如果数据量很小，直接同步执行（避免 Job 创建开销）
    if (count <= batchSize) {
        char* byteData = (char*)data;
        callback(byteData, count, userData);
        jobSystem->RunJob(rootJob);  // 立即完成 rootJob
        return rootJob;
    }

    uint32_t adjustedBatchSize = batchSize;

    // ✅ 预分割：计算所有工作范围（使用调整后的 batch size）
    std::vector<WorkRange> ranges;
    for (uint32_t i = 0; i < count; i += adjustedBatchSize) {
        uint32_t batchCount = std::min(adjustedBatchSize, count - i);
        ranges.push_back({i, batchCount});
    }

    // ✅ 为每个范围创建一个叶子 Job
    char* byteData = (char*)data;
    for (const auto& range : ranges) {
        // 从对象池分配数据
        auto* pfData = ParallelForDataCPool::GetInstance().Allocate();
        pfData->jobSystem = jobSystem;
        pfData->data = byteData + (range.start * elementSize);  // 指向当前批次的起始位置
        pfData->dataSize = elementSize;
        pfData->count = range.count;
        pfData->callback = callback;
        pfData->userData = userData;
        pfData->splitter = splitter;
        pfData->parentJob = rootJob;

        // 创建并运行叶子 Job
        Job* leafJob = jobSystem->CreateJob(rootJob, ParallelForLeafJobC);
        leafJob->data = pfData;
        jobSystem->RunJob(leafJob);
    }

    return rootJob;
}
