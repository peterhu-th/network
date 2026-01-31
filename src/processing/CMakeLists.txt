cmake_minimum_required(VERSION 3.16)
project(processing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Core QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core)
endif()

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../core
)

file(GLOB_RECURSE PROCESSING_SOURCES
        ${PROJECT_SOURCE_DIR}/*.cpp
        ${PROJECT_SOURCE_DIR}/*.h
)

add_library(${PROJECT_NAME} SHARED
        ${PROCESSING_SOURCES}
)

if (Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core)
endif()

install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/
        DESTINATION include/processing
        FILES_MATCHING PATTERN "*.h"
)