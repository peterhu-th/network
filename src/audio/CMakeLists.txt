set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Network Multimedia REQUIRED)

add_library(audio STATIC
        IAudioSource.h
        AudioSourceBase.h AudioSourceBase.cpp
        AudioSourceFactory.h AudioSourceFactory.cpp
        SystemAudioSource.h SystemAudioSource.cpp
)

target_include_directories(audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(audio PUBLIC core Qt6::Network Qt6::Multimedia)