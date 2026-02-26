#include "JobSystem.h"
thread_local int* tlthreadIndex = nullptr;
thread_local JobAllocator g_jobAllocator;
std::vector<WorkThreadStealQueue*> g_threadsJobQueue;


#pragma region JobSystem生命周期
void JobSystem::Initialize() {
	numThreads = std::thread::hardware_concurrency();
	g_threadsJobQueue.resize(numThreads);

	// 打开日志文件
	frameCounter = 0;
	logFile.open("JobSystemDebug.txt", std::ios::out | std::ios::trunc);
	if (logFile.is_open()) {
		logFile << "=== JobSystem Debug Log ===\n";
		logFile << "Initialized with " << numThreads << " threads\n";
		logFile << "============================\n\n";
		logFile.flush();
	}

	// First, create all queues before starting threads
	for (int i = 0; i < numThreads; i++)
	{
		g_threadsJobQueue[i] = new WorkThreadStealQueue();
	}

	// Initialize main thread
	tlthreadIndex = new int(0);
	g_jobAllocator.Initialize(MAX_NUMBER_OF_JOBS_PERTTHREAD);

	isRunning = true;

	// Now start worker threads
	for (int i = 0; i < numThreads - 1; i++)
	{
		workerThreads.emplace_back([this, i]() {
			WorkerThreadFunction(i + 1);
		});
	}
}

void JobSystem::FrameStart()
{
}

void JobSystem::FrameEnd()
{
}

void JobSystem::ShutDown()
{
	isRunning = false;

	for (auto& thread : workerThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	workerThreads.clear();

	for (auto* queue : g_threadsJobQueue) {
		if (queue != nullptr) {
			delete queue;
		}
	}
	g_threadsJobQueue.clear();

	delete tlthreadIndex;

}

#pragma endregion



void JobSystem::WorkerThreadFunction(int threadIndex) {
		tlthreadIndex = new int(threadIndex);
		g_jobAllocator.Initialize(MAX_NUMBER_OF_JOBS_PERTTHREAD);
		while (isRunning) {
			Job* job = GetJob();
			if (job)
			{
				ExecuteJob(job);
			}
		}
		delete tlthreadIndex;
		tlthreadIndex = nullptr;
}

WorkThreadStealQueue* JobSystem::GetWorkerThreadQueue() {
	return g_threadsJobQueue[*tlthreadIndex];
}

Job* JobSystem::GetJob() {
	WorkThreadStealQueue* queue = GetWorkerThreadQueue();

	Job* job = queue->Pop();
	if (job == nullptr)
	{
		// this is not a valid job because our own queue is empty, so try stealing from some other queue
		unsigned int randomIndex = GenerateRandomNumber(0, numThreads);
		WorkThreadStealQueue* stealQueue = g_threadsJobQueue[randomIndex];

		// Check if the steal queue is valid and not the same as our queue
		if (stealQueue == nullptr || stealQueue == queue)
		{
			// don't try to steal from ourselves or invalid queue
			Yield();
			return nullptr;
		}

		Job* stolenJob = stealQueue->Steal();
		if (stolenJob == nullptr)
		{
			// we couldn't steal a job from the other queue either, so we just yield our time slice for now
			Yield();
			return nullptr;
		}

		return stolenJob;
	}

	return job;
}
#pragma region Job生命周期
Job* JobSystem::CreateJob(JobFunction func) {
	Job* job = g_jobAllocator.AllocateJob();
	job->_func = func;
	job->_parent = nullptr;
	job->_unfinishedJob = 1;
	job->continuationCount = 0;
	// 初始化 continuations 数组
	for (size_t i = 0; i < MAX_JOB_CONTINUATIONS; i++) {
		job->continuations[i] = nullptr;
	}
	return job;
}

Job* JobSystem::CreateJob(Job* parent, JobFunction func) {
	parent->_unfinishedJob.fetch_add(1, std::memory_order_relaxed); // �̰߳�ȫ����

	Job* job = g_jobAllocator.AllocateJob();
	job->_func = func;
	job->_parent = parent;
	job->_unfinishedJob = 1;
	job->continuationCount = 0;
	// 初始化 continuations 数组
	for (size_t i = 0; i < MAX_JOB_CONTINUATIONS; i++) {
		job->continuations[i] = nullptr;
	}
	return job;
}

void JobSystem::RunJob(Job* job) {
	WorkThreadStealQueue* queue = GetWorkerThreadQueue();
	queue->Push(job);
}

void JobSystem::WaitJob(Job* job) {
	while (!HasJobCompleted(job)) {
		Job* nextJob = GetJob();
		if (nextJob) {
			ExecuteJob(nextJob);
		}
	}
}
void JobSystem::ExecuteJob(Job* job) {
	(job->_func)(job, job->data);
	FinishJob(job);
}

void JobSystem::AddContinuation(Job* job, Job* continuation) {
	// 原子地增加 continuation 计数并获取索引
	const int32_t index = job->continuationCount.fetch_add(1, std::memory_order_relaxed);

	// 检查是否超出最大 continuation 数量
	if (index >= MAX_JOB_CONTINUATIONS) {
		// 超出限制，可以选择抛出异常或记录错误
		// 这里简单处理：减回计数
		job->continuationCount.fetch_sub(1, std::memory_order_relaxed);
		return;
	}

	// 将 continuation 添加到数组中
	job->continuations[index] = continuation;
}

void JobSystem::Log(const char* message) {
	std::lock_guard<std::mutex> lock(logMutex);
	if (logFile.is_open()) {
		logFile << message;
		logFile.flush();
	}
}

/// <summary>
/// </summary>
/// <param name="job"></param>
void JobSystem::FinishJob(Job* job) {
	const int32_t unfinishedJobs = job->_unfinishedJob.fetch_sub(1, std::memory_order_relaxed) - 1;
	if (unfinishedJobs == 0)
	{
		// 触发所有 continuations
		const int32_t count = job->continuationCount.load(std::memory_order_relaxed);
		for (int32_t i = 0; i < count; i++) {
			Job* continuation = job->continuations[i];
			if (continuation != nullptr) {
				RunJob(continuation);
			}
		}

		// 通知父 Job
		if (job->_parent)
		{
			FinishJob(job->_parent);
		}
	}
}
#pragma endregion



