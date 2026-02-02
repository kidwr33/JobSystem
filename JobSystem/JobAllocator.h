#pragma once
#include "Job.h"

class JobAllocator {
private:
	int index;
	int size;
	Job* jobAllocator;
public:
	JobAllocator() : index(0), size(0), jobAllocator(nullptr) {}
	~JobAllocator();

	void Initialize(int size = 0);
	void FrameStart();
	void FrameEnd();
	Job* AllocateJob();
};

