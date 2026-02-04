cmake_minimum_required(VERSION 3.16)
project(radar_processing)

# 配置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 仅依赖Qt Core（无需Multimedia，避免audio模块依赖）
find_package(Qt5 REQUIRED COMPONENTS Core)

# 头文件路径（修正core模块路径）
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# processing模块所有源文件（仅包含自己的文件）
set(SOURCES
        src/main.cpp
        src/DenoiseProcessor.cpp
        src/VADProcessor.cpp
        src/FeatureExtractor.cpp
        src/ProcessorFactory.cpp
        src/PipelineManager.cpp
        src/AudioProcessingService.cpp
)

# 编译可执行文件（用于调试）
add_executable(processing_exec ${SOURCES})
target_link_libraries(processing_exec Qt5::Core)

# 编译静态库（供项目其他模块调用）
add_library(processing_lib STATIC ${SOURCES})
target_link_libraries(processing_lib Qt5::Core)