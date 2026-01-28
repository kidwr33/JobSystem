#pragma once
#include <thread>
#include <atomic>
// ×Ô¶¯¼ÆËãpadding
struct Job;
typedef void (*JobFunction)(Job* job, void* data);


static constexpr size_t CACHE_LINE_SIZE = 64;
static constexpr size_t DATA_SIZE = sizeof(JobFunction) + sizeof(Job*) +
sizeof(std::atomic<int32_t>) +
sizeof(void*);


struct Job {
	JobFunction _func;
	Job* _parent;
	std::atomic<int32_t> _unfinishedJob;
	void* data;
	char padding[DATA_SIZE];
};


