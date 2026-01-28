#pragma 
#include <mutex>
#include "Job.h"

static const unsigned int MAX_NUMBER_OF_JOBS_PERTTHREAD = 4096u;
static const unsigned int MASK = MAX_NUMBER_OF_JOBS_PERTTHREAD - 1u;

class WorkThreadStealQueue {
private:
	std::mutex mutex;
	Job* m_jobs[MAX_NUMBER_OF_JOBS_PERTTHREAD];
	int m_bottom;
	int m_top;
public:
	void Push(Job* job);
	Job* Pop();
	Job* Steal();
};