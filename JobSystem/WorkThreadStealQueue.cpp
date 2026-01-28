#include "WorkThreadStealQueue.h"


void WorkThreadStealQueue::Push(Job* job) {
	std::lock_guard<std::mutex> lock(mutex); 

	m_jobs[m_bottom & MASK] = job;
	m_bottom++;
	return;
}

Job* WorkThreadStealQueue::Pop() {
	std::lock_guard<std::mutex> lock(mutex);
	const int jobCount = m_bottom - m_top;
	if (jobCount <= 0) {
		return nullptr;
	}
	m_bottom--;
	return m_jobs[m_bottom & MASK];
}

Job* WorkThreadStealQueue::Steal() {
	std::lock_guard<std::mutex> lock(mutex);
	const int jobCount = m_bottom - m_top;
	if (jobCount <= 0) {
		return nullptr;
	}
	Job* job = m_jobs[m_top & MASK];
	++m_top;
	return job;
}