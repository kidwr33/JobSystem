#pragma once
#include <thread>
#include <atomic>

// Forward declaration
struct Job;
typedef void (*JobFunction)(Job* job, void* data);

// 每个 Job 最多支持的 continuation 数量
static constexpr size_t MAX_JOB_CONTINUATIONS = 10;

// Job 结构体大小：128 字节（两个缓存行）
static constexpr size_t JOB_SIZE = 128;
static constexpr size_t DATA_SIZE = sizeof(JobFunction) + sizeof(Job*) +
sizeof(std::atomic<int32_t>) * 2 +
sizeof(void*) +
sizeof(Job*) * MAX_JOB_CONTINUATIONS;


struct Job {
	JobFunction _func;                               // 8 bytes
	Job* _parent;                                    // 8 bytes
	std::atomic<int32_t> _unfinishedJob;            // 4 bytes
	std::atomic<int32_t> continuationCount;         // 4 bytes
	void* data;                                      // 8 bytes
	Job* continuations[MAX_JOB_CONTINUATIONS];      // 80 bytes
	char padding[JOB_SIZE - DATA_SIZE];             // 16 bytes
};
