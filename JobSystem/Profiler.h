#pragma once
#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>
#include <sstream>

// 轻量级性能追踪器 - 输出Chrome Tracing格式
class Profiler {
public:
    static Profiler& Instance() {
        static Profiler instance;
        return instance;
    }

    void BeginSession(const char* filepath = "trace.json") {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            EndSession();
        }
        file_.open(filepath);
        file_ << "{\"traceEvents\":[\n";
        firstEvent_ = true;
    }

    void EndSession() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_ << "\n]}";
            file_.close();
        }
    }

    void WriteEvent(const char* name, const char* cat, char ph, int64_t ts, int64_t dur = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) return;

        if (!firstEvent_) {
            file_ << ",\n";
        }
        firstEvent_ = false;

        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        file_ << "{\"name\":\"" << name << "\","
              << "\"cat\":\"" << cat << "\","
              << "\"ph\":\"" << ph << "\","
              << "\"ts\":" << ts << ","
              << "\"pid\":1,"
              << "\"tid\":" << tid;

        if (ph == 'X' && dur > 0) {
            file_ << ",\"dur\":" << dur;
        }

        file_ << "}";
    }

    int64_t GetTimestamp() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

private:
    Profiler() = default;
    ~Profiler() { EndSession(); }

    std::ofstream file_;
    std::mutex mutex_;
    bool firstEvent_ = true;
};

// RAII作用域追踪
class ScopedTrace {
public:
    ScopedTrace(const char* name, const char* category = "function")
        : name_(name), category_(category) {
        start_ = Profiler::Instance().GetTimestamp();
    }

    ~ScopedTrace() {
        int64_t end = Profiler::Instance().GetTimestamp();
        int64_t duration = end - start_;
        Profiler::Instance().WriteEvent(name_, category_, 'X', start_, duration);
    }

private:
    const char* name_;
    const char* category_;
    int64_t start_;
};

// 便捷宏
#define PROFILE_SESSION_BEGIN(filepath) Profiler::Instance().BeginSession(filepath)
#define PROFILE_SESSION_END() Profiler::Instance().EndSession()
#define PROFILE_SCOPE(name) ScopedTrace _trace##__LINE__(name)
#define PROFILE_FUNCTION() ScopedTrace _trace##__LINE__(__FUNCTION__)
