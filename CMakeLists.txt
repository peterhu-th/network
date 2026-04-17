cmake_minimum_required(VERSION 3.16)
project(AudioRadarClient VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer Widgets Concurrent)

# modules
add_subdirectory(src/core)
add_subdirectory(src/audio)
add_subdirectory(src/processing)
# add_subdirectory(src/storage)     # TODO: enable after implementation
add_subdirectory(src/network)
add_subdirectory(tests)

# main executable
add_executable(${PROJECT_NAME} src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    core
    audio
    processing_lib
    # storage
    network
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Widgets
    Qt6::Concurrent
)

# copy config
configure_file(config/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
