#include "WorkThreadStealQueue.h"
#include <atomic>

#define COMPILER_BARRIER std::atomic_signal_fence(std::memory_order_seq_cst)

// macOS/Linux compatibility for Windows Interlocked functions
#if !defined(_WIN32) && !defined(_WIN64)
inline long _InterlockedExchange(volatile long* target, long value) {
    std::atomic<long>* atomic_target = reinterpret_cast<std::atomic<long>*>(const_cast<long*>(target));
    return atomic_target->exchange(value, std::memory_order_seq_cst);
}

inline long _InterlockedCompareExchange(volatile long* destination, long exchange, long comparand) {
    std::atomic<long>* atomic_dest = reinterpret_cast<std::atomic<long>*>(const_cast<long*>(destination));
    atomic_dest->compare_exchange_strong(comparand, exchange, std::memory_order_seq_cst);
    return comparand;
}
#endif

void WorkThreadStealQueue::Push(Job* job) {
	long b = m_bottom;
	m_jobs[b & MASK] = job;

    COMPILER_BARRIER;
	m_bottom = b + 1;
}

Job* WorkThreadStealQueue::Pop() {
    long b = m_bottom - 1;
    _InterlockedExchange(&m_bottom, b);

    long t = m_top;
    if (t <= b)
    {
        // non-empty queue
        Job* job = m_jobs[b & MASK];
        if (t != b)
        {
            // there's still more than one item left in the queue
            return job;
        }

        // this is the last item in the queue
        if (_InterlockedCompareExchange(&m_top, t + 1, t) != t)
        {
            // failed race against steal operation
            job = nullptr;
        }

        m_bottom = t + 1;
        return job;
    }
    else
    {
        // deque was already empty
        m_bottom = t;
        return nullptr;
    }
}

Job* WorkThreadStealQueue::Steal() {
    long t = m_top;

    // ensure that top is always read before bottom.
    // loads will not be reordered with other loads on x86, so a compiler barrier is enough.
    COMPILER_BARRIER;

    long b = m_bottom;
    if (t < b)
    {
        // non-empty queue
        Job* job = m_jobs[t & MASK];

        // the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
        if (_InterlockedCompareExchange(&m_top, t + 1, t) != t)
        {
            // a concurrent steal or pop operation removed an element from the deque in the meantime.
            return nullptr;
        }

        return job;
    }
    else
    {
        // empty queue
        return nullptr;
    }
}