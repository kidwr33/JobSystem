cmake_minimum_required(VERSION 3.10)
project(JobSystem)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 启用调试符号（Debug模式）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# macOS: 生成独立的 dSYM 文件，避免调试符号不匹配问题
if(APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# 定义源文件（不包含 main.cpp）
set(JOBSYSTEM_SOURCES
    JobSystem/JobSystem.cpp
    JobSystem/JobAllocator.cpp
    JobSystem/WorkThreadStealQueue.cpp
    JobSystem/JobSystemCAPI.cpp
    JobSystem/ParallelForC.cpp
    JobSystem/ParticleUpdateNative.cpp
)

# 添加动态库
add_library(JobSystem SHARED ${JOBSYSTEM_SOURCES})

# 设置导出符号
target_compile_definitions(JobSystem PRIVATE JOBSYSTEM_BUILD_DLL)

# 包含头文件目录
target_include_directories(JobSystem PUBLIC JobSystem)

# 链接 pthread（macOS/Linux 需要）
find_package(Threads REQUIRED)
target_link_libraries(JobSystem PRIVATE Threads::Threads)

# 设置输出目录
set_target_properties(JobSystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# 可选：构建测试可执行文件
option(BUILD_TESTS "Build test executable" ON)
if(BUILD_TESTS)
    add_executable(JobSystemTest JobSystem/main.cpp)
    target_link_libraries(JobSystemTest PRIVATE JobSystem)
    target_include_directories(JobSystemTest PRIVATE JobSystem)
    set_target_properties(JobSystemTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
