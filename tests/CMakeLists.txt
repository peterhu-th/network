add_executable(test_audio test_audio.cpp)
target_link_libraries(test_audio PRIVATE
    audio
    core
    Qt5::Core
    Qt5::Test
)

add_executable(test_network test_network.cpp)
target_link_libraries(test_network PRIVATE
    network_lib
    core
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::Test
)
