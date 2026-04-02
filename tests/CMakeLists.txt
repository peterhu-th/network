add_executable(test_audio test_audio.cpp)
target_link_libraries(test_audio PRIVATE
    audio
    core
    Qt6::Core
)

add_executable(test_network test_network.cpp)
target_link_libraries(test_network PRIVATE
    network
    core
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)