cmake_minimum_required(VERSION 3.16)
project(radar_processing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

include_directories(
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES
        DenoiseProcessor.cpp
        VADProcessor.cpp
        FeatureExtractor.cpp
        ProcessorFactory.cpp
        PipelineManager.cpp
        AudioProcessingService.cpp
)

add_library(processing_lib STATIC ${SOURCES})
target_link_libraries(processing_lib PRIVATE Qt6::Core core)
