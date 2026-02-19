add_executable(test_audio test_audio.cpp)
target_link_libraries(test_audio PRIVATE
    audio
    core
    Qt5::Core
)

add_executable(test_network test_network.cpp)
target_link_libraries(test_network PRIVATE
    network
    core
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)