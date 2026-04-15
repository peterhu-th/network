cmake_minimum_required(VERSION 3.16)
project(radar_processing)

# 配置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# 头文件路径（关联项目core目录和processing自身include）
include_directories(
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 所有库源文件
set(LIB_SOURCES
        src/DenoiseProcessor.cpp
        src/VADProcessor.cpp
        src/FeatureExtractor.cpp
        src/ProcessorFactory.cpp
        src/PipelineManager.cpp
        src/AudioProcessingService.cpp
)

# 编译为可执行文件（用于测试）
add_executable(processing_exec src/main.cpp ${LIB_SOURCES})
target_link_libraries(processing_exec Qt6::Core core)
add_library(processing_lib STATIC ${LIB_SOURCES})
target_link_libraries(processing_lib Qt6::Core core)
