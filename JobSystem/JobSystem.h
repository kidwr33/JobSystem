#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include "Job.h"
#include "WorkThreadStealQueue.h"
#include "JobAllocator.h"



class JobSystem {
private:
	std::vector<std::thread> workerThreads;
	std::atomic<bool> isRunning;
	int numThreads;

public:
#pragma region JobSystem生命周期
	void Initialize();
	void FrameStart();
	void FrameEnd();
	void ShutDown();
#pragma endregion

#pragma region Job生命周期
	Job* CreateJob(JobFunction func);
	Job* CreateJob(Job* parent, JobFunction func);
	void RunJob(Job* job); // 感觉这里命名不是很好，Run是让Job有被执行的能力
	void WaitJob(Job* job);
	void ExecuteJob(Job* job);
	void FinishJob(Job* job);
#pragma endregion


private:
	void WorkerThreadFunction(int threadIndex);
	WorkThreadStealQueue* GetWorkerThreadQueue();
	Job* GetJob();
	bool HasJobCompleted(Job* job) { return job->_unfinishedJob == -1; }
private:
	void Yield() { std::this_thread::yield(); }
	int GenerateRandomNumber(int rangeStart, int rangeEnd) { //左闭右开
		static bool initialized = false;
		if (!initialized) {
			srand(static_cast<unsigned int>(time(nullptr))); 
			initialized = true;
		}
		return rangeStart + rand() % (rangeEnd - rangeStart);
	} 
};