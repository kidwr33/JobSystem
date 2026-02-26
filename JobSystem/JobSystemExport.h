#pragma once

// 动态库导出/导入宏定义
#if defined(_WIN32) || defined(_WIN64)
    #ifdef JOBSYSTEM_BUILD_DLL
        #define JOBSYSTEM_API __declspec(dllexport)
    #else
        #define JOBSYSTEM_API __declspec(dllimport)
    #endif
#else
    // macOS/Linux 使用 visibility 属性
    #ifdef JOBSYSTEM_BUILD_DLL
        #define JOBSYSTEM_API __attribute__((visibility("default")))
    #else
        #define JOBSYSTEM_API
    #endif
#endif

// C 函数导出宏（用于 Unity P/Invoke）
#ifdef __cplusplus
    #define JOBSYSTEM_C_API extern "C" JOBSYSTEM_API
#else
    #define JOBSYSTEM_C_API JOBSYSTEM_API
#endif
