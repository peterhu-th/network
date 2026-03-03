cmake_minimum_required(VERSION 3.16)
project(AudioRadarClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Network Sql Widgets)

# modules
add_subdirectory(src/core)
add_subdirectory(src/audio)
add_subdirectory(src/processing)
# add_subdirectory(src/storage)     # TODO: enable after implementation
add_subdirectory(src/network)
add_subdirectory(tests)

# add_subdirectory(src/ui)

# main executable
add_executable(${PROJECT_NAME} src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    core
    audio
    processing_lib
    # storage
    network
    # ui
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::Widgets
)

# copy config
configure_file(config/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
