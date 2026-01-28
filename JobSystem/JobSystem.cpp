#include "JobSystem.h"
thread_local int* tlthreadIndex = nullptr;
std::vector<WorkThreadStealQueue*> g_threadsJobQueue;
std::atomic<int> g_jobToDeleteCount{0};
Job** g_jobsToDelete;

#pragma region JobSystem生命周期
void JobSystem::Initialize() {
	numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) numThreads = 4;

	g_threadsJobQueue.resize(numThreads);
	tlthreadIndex = new int(0); 
	g_threadsJobQueue[0] = new WorkThreadStealQueue();
	for (int i = 0; i < numThreads - 1; i++)
	{
		 workerThreads.emplace_back([this, i]() {
			WorkerThreadFunction(i + 1);
			});
		g_threadsJobQueue[i + 1] = new WorkThreadStealQueue();
	}

	if (g_jobsToDelete != nullptr) {
		delete[] g_jobsToDelete;
	}
	size_t size = static_cast<size_t>(MAX_NUMBER_OF_JOBS_PERTTHREAD) * numThreads;
	g_jobsToDelete = new Job*[size](); 

	isRunning = true;

}

void JobSystem::FrameStart()
{
}

void JobSystem::FrameEnd()
{
	int count = g_jobToDeleteCount.load();
	for (int i = 0; i < g_jobToDeleteCount; i++) {
		DeleteJob(g_jobsToDelete[i]);
		g_jobsToDelete[i] = nullptr; //置空防止数据污染
	}
	g_jobToDeleteCount.store(0);
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
	if (g_jobsToDelete != nullptr) {
		delete[] g_jobsToDelete;
		g_jobsToDelete = nullptr;
	}

}

#pragma endregion



void JobSystem::WorkerThreadFunction(int threadIndex) {
		tlthreadIndex = new int(threadIndex);
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
		if (stealQueue == queue)
		{
			// don't try to steal from ourselves
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
	Job* job = AllocateJob();
	job->_func = func;
	job->_parent = nullptr;
	job->_unfinishedJob = 1;
	return job;
}

Job* JobSystem::CreateJob(Job* parent, JobFunction func) {
	parent->_unfinishedJob++; // �̰߳�ȫ����

	Job* job = AllocateJob();
	job->_func = func;
	job->_parent = parent;
	job->_unfinishedJob = 1;
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

/// <summary>
/// </summary>
/// <param name="job"></param>
void JobSystem::FinishJob(Job* job) {
	const int32_t unfinishedJobs = --job->_unfinishedJob;
	if (unfinishedJobs == 0)
	{
		const int32_t index = ++g_jobToDeleteCount;
		g_jobsToDelete[index - 1] = job;

		if (job->_parent)
		{
			FinishJob(job->_parent);
		}

		job->_unfinishedJob--;
	}
}
#pragma endregion



