cmake_minimum_required(VERSION 3.16)
project(radar_processing)

# 配置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 依赖Qt5 Core（替换原Qt6配置）
find_package(Qt5 REQUIRED COMPONENTS Core)

# 头文件路径（关联项目core目录和processing自身include）
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 所有源文件
set(SOURCES
        src/main.cpp
        src/DenoiseProcessor.cpp
        src/VADProcessor.cpp
        src/FeatureExtractor.cpp
        src/ProcessorFactory.cpp
        src/PipelineManager.cpp
        src/AudioProcessingService.cpp
)

# 编译为可执行文件（用于测试）
add_executable(processing_exec ${SOURCES})
# 链接Qt5 Core库
target_link_libraries(processing_exec Qt5::Core)

# 编译为静态库（供项目其他模块调用）
add_library(processing_lib STATIC ${SOURCES})
# 链接Qt5 Core库
target_link_libraries(processing_lib Qt5::Core)