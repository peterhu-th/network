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

add_executable(test_impulse test_impulse.cpp)
target_link_libraries(test_impulse PRIVATE
    processing_lib
    core
    Qt6::Core
)

add_executable(test_processing_main test_processing_main.cpp)
target_link_libraries(test_processing_main PRIVATE
    processing_lib
    core
    Qt6::Core
)
add_test(NAME test_processing_main COMMAND test_processing_main)
