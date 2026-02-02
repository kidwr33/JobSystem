#include "JobAllocator.h"
#include <cassert>

JobAllocator::~JobAllocator()
{
	if (jobAllocator != nullptr) {
		delete[] jobAllocator;
		jobAllocator = nullptr;
	}
}

void JobAllocator::Initialize(int size)
{
	// 检查 size 是否为 2 的整数次幂
	assert(size > 0 && (size & (size - 1)) == 0 && "Size must be a power of 2");

	this->size = size;
	index = 0;
	jobAllocator = new Job[this->size];
}

void JobAllocator::FrameStart()
{
}

void JobAllocator::FrameEnd()
{
}


Job* JobAllocator::AllocateJob()
{
	index++;
	return &jobAllocator[index & (size - 1)];
}
