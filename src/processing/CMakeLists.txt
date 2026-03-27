cmake_minimum_required(VERSION 3.16)
project(radar_processing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES
        src/main.cpp
        src/DenoiseProcessor.cpp
        src/VADProcessor.cpp
        src/FeatureExtractor.cpp
        src/ProcessorFactory.cpp
        src/PipelineManager.cpp
        src/AudioProcessingService.cpp
)

add_executable(processing_exec ${SOURCES})
target_link_libraries(processing_exec Qt5::Core)

add_executable(test_impulse_exec src/test_impulse.cpp src/DenoiseProcessor.cpp)
target_link_libraries(test_impulse_exec Qt5::Core)

add_library(processing_lib STATIC ${SOURCES})
target_link_libraries(processing_lib Qt5::Core)