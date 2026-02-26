#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <fstream>
#include <mutex>
#include "Job.h"
#include "WorkThreadStealQueue.h"
#include "JobAllocator.h"



class JobSystem {
private:
	std::vector<std::thread> workerThreads;
	std::atomic<bool> isRunning;
	int numThreads;

	// 日志相关
	std::ofstream logFile;
	std::mutex logMutex;
	int frameCounter;

public:
#pragma region JobSystem��������
	void Initialize();
	void FrameStart();
	void FrameEnd();
	void ShutDown();
#pragma endregion

#pragma region Job��������
	Job* CreateJob(JobFunction func);
	Job* CreateJob(Job* parent, JobFunction func);
	void RunJob(Job* job); // �о������������Ǻܺã�Run����Job�б�ִ�е�����
	void WaitJob(Job* job);
	void ExecuteJob(Job* job);
	void FinishJob(Job* job);
	void AddContinuation(Job* job, Job* continuation);
	void Log(const char* message);
#pragma endregion


private:
	void WorkerThreadFunction(int threadIndex);
	WorkThreadStealQueue* GetWorkerThreadQueue();
	Job* GetJob();
	bool HasJobCompleted(Job* job) { return job->_unfinishedJob == 0; }
private:
	void Yield() { std::this_thread::yield(); }
	int GenerateRandomNumber(int rangeStart, int rangeEnd) { //����ҿ�
		static bool initialized = false;
		if (!initialized) {
			srand(static_cast<unsigned int>(time(nullptr))); 
			initialized = true;
		}
		return rangeStart + rand() % (rangeEnd - rangeStart);
	} 
};