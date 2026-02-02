add_executable(test_audio test_audio.cpp)

target_link_libraries(test_audio PRIVATE
        audio
        core
        Qt5::Core
)
