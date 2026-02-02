#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include "JobSystem.h"

// 测试Job函数：简单计算任务
void SimpleTask(Job* job, void* data) {

}

// 测试Job函数：嵌套任务
void ParentTask(Job* job, void* data) {
    // 模拟一些工作
    volatile int sum = 0;
    for (int i = 0; i < 5000; i++) {
        sum += i;
    }
}

void ChildTask(Job* job, void* data) {
    // 子任务工作
    volatile int sum = 0;
    for (int i = 0; i < 3000; i++) {
        sum += i;
    }
}

// 帧统计结构
struct FrameStats {
    double frameTime;  // 毫秒
    int jobCount;
};

int main() {
    std::cout << "=== JobSystem Test ===" << std::endl;

    // 初始化JobSystem
    JobSystem jobSystem;
    jobSystem.Initialize();
    std::cout << "JobSystem initialized." << std::endl;

    // 测试参数
    const int TOTAL_FRAMES = 10;
    const int JOBS_PER_FRAME = 4095;
    const int CHILD_JOBS_PER_PARENT = 5;

    std::vector<FrameStats> frameStats;
    frameStats.reserve(TOTAL_FRAMES);

    std::cout << "Running " << TOTAL_FRAMES << " frames with "
              << JOBS_PER_FRAME << " jobs per frame..." << std::endl;
    std::cout << std::endl;

    // 主循环
    for (int frame = 0; frame < TOTAL_FRAMES; frame++) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        jobSystem.FrameStart();

        // 创建测试Job
        std::vector<Job*> jobs;


        // 1. 创建简单任务
     /*   for (int i = 0; i < JOBS_PER_FRAME ; i++) {
            Job* job = jobSystem.CreateJob(SimpleTask);
            jobs.push_back(job);
            jobSystem.RunJob(job);
            jobSystem.WaitJob(job);
        }*/
        
        Job* Root = jobSystem.CreateJob(SimpleTask);
        jobs.push_back(Root);
        for (int i = 0; i < JOBS_PER_FRAME; i++) {
            Job* job = jobSystem.CreateJob(Root, SimpleTask);
            jobs.push_back(job);
            jobSystem.RunJob(job);
        }
        jobSystem.RunJob(Root);
        jobSystem.WaitJob(Root);
      
        //// 2. 创建嵌套任务（父任务+子任务）
        //for (int i = 0; i < JOBS_PER_FRAME / 10; i++) {
        //    Job* parentJob = jobSystem.CreateJob(ParentTask);

        //    // 为父任务创建多个子任务
        //   for (int j = 0; j < CHILD_JOBS_PER_PARENT; j++) {
        //        Job* childJob = jobSystem.CreateJob(parentJob, ChildTask);
        //        jobSystem.RunJob(childJob);
        //    }

        //    jobs.push_back(parentJob);
        //    jobSystem.RunJob(parentJob);
        //}

  

        jobSystem.FrameEnd();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

        // 记录统计
        frameStats.push_back({frameTime, (int)jobs.size()});

        // 每10帧输出一次
        if ((frame + 1) % 10 == 0) {
            std::cout << "Frame " << (frame + 1) << " completed in "
                      << frameTime << " ms" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "=== Frame Statistics ===" << std::endl;

    // 计算统计信息
    double totalTime = 0.0;
    double minTime = frameStats[0].frameTime;
    double maxTime = frameStats[0].frameTime;

    for (const auto& stat : frameStats) {
        totalTime += stat.frameTime;
        minTime = std::min(minTime, stat.frameTime);
        maxTime = std::max(maxTime, stat.frameTime);
    }

    double avgTime = totalTime / TOTAL_FRAMES;

    // 计算标准差
    double variance = 0.0;
    for (const auto& stat : frameStats) {
        variance += (stat.frameTime - avgTime) * (stat.frameTime - avgTime);
    }
    double stdDev = std::sqrt(variance / TOTAL_FRAMES);

    std::cout << "Total frames: " << TOTAL_FRAMES << std::endl;
    std::cout << "Average frame time: " << avgTime << " ms" << std::endl;
    std::cout << "Min frame time: " << minTime << " ms" << std::endl;
    std::cout << "Max frame time: " << maxTime << " ms" << std::endl;
    std::cout << "Std deviation: " << stdDev << " ms" << std::endl;
    std::cout << "Total time: " << totalTime << " ms" << std::endl;
    std::cout << "Average FPS: " << (1000.0 / avgTime) << std::endl;

    // 关闭JobSystem
    std::cout << std::endl;
    std::cout << "Shutting down JobSystem..." << std::endl;
    jobSystem.ShutDown();
    std::cout << "JobSystem shutdown complete." << std::endl;

    return 0;
}
