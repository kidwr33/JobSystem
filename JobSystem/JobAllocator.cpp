#include "JobAllocator.h"

Job* AllocateJob() {
	return new Job();
}

void DeleteJob(Job* ptr) {
	delete ptr;
}